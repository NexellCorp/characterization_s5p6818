#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/media.h>
#include <linux/v4l2-subdev.h>
#include <linux/v4l2-mediabus.h>
#include <linux/videodev2.h>

#include <ion.h>
#include <linux/ion.h>
#include <linux/nxp_ion.h>
#include <linux/videodev2_nxp_media.h>

#include <nxp-v4l2.h>

#include "common.cpp"

#define MAX_BUFFER_COUNT 4
#define DEFAULT_WIDTH    720
#define DEFAULT_HEIGHT   480

#define DEFAULT_TEST_COUNT  20

#define MAX_BUFFER_COUNT 4

void print_usage(void)
{
    printf( "usage: no options\n"
			" -C test count       (default: %d)\n"
			" -T not supported.\n"
			, DEFAULT_TEST_COUNT
            );
}

static int test_count = DEFAULT_TEST_COUNT;

int main(int argc, char *argv[])
{
    int ion_fd = ion_open();
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;
    int module = 0;
    int clipper_id = nxp_v4l2_clipper0;
    int sensor_id = nxp_v4l2_sensor0;
    int video_id = nxp_v4l2_mlc0_video;
    int format = V4L2_PIX_FMT_YUV420M;
    int opt;

    while (-1 != (opt = getopt(argc, argv, "hC:T"))) {
        switch (opt) {
        case 'h':	print_usage();  exit(0);	break;
        case 'C':   test_count = atoi(optarg);	break;
        case 'T':   							break;
        default:
                    break;
        }
    }

    if (ion_fd < 0) {
        fprintf(stderr, "can't open ion!!!\n");
        return -EINVAL;
    }

    struct V4l2UsageScheme s;
    memset(&s, 0, sizeof(s));

    s.useClipper0 = true;
    s.useDecimator0 = true;
    s.useMlc0Video = true;
    //printf("width %d, height %d, module %d\n", width, height, module);
    CHECK_COMMAND(v4l2_init(&s));
    CHECK_COMMAND(v4l2_set_format(clipper_id, width, height, format));
    CHECK_COMMAND(v4l2_set_crop(clipper_id, 0, 0, width, height));

    CHECK_COMMAND(v4l2_set_format(sensor_id, width, height, V4L2_MBUS_FMT_YUYV8_2X8));
    CHECK_COMMAND(v4l2_set_format(video_id, width, height, format));

	// setting destination position
    CHECK_COMMAND(v4l2_set_crop(video_id, 0, 0, width, height));
    // setting source crop
    CHECK_COMMAND(v4l2_set_crop_with_pad(video_id, 2, 0, 0, width, height)); //psw 20150331

    CHECK_COMMAND(v4l2_set_ctrl(video_id, V4L2_CID_MLC_VID_PRIORITY, 0));
    CHECK_COMMAND(v4l2_set_ctrl(video_id, V4L2_CID_MLC_VID_COLORKEY, 0x0));
    CHECK_COMMAND(v4l2_reqbuf(clipper_id, MAX_BUFFER_COUNT));
    CHECK_COMMAND(v4l2_reqbuf(video_id, MAX_BUFFER_COUNT));
    //printf("alloc video\n");
    struct nxp_vid_buffer bufs[MAX_BUFFER_COUNT];
    CHECK_COMMAND(alloc_buffers(ion_fd, MAX_BUFFER_COUNT, bufs, width, height, format));
    //printf("vid_buf: %p, %p, %p, %p\n", bufs[0].virt[0], bufs[1].virt[0], bufs[2].virt[0], bufs[3].virt[0]);

    int i;
    for (i = 0; i < MAX_BUFFER_COUNT; i++) {
        struct nxp_vid_buffer *buf = &bufs[i];
        //printf("buf plane num: %d\n", buf->plane_num);
        CHECK_COMMAND(v4l2_qbuf(clipper_id, buf->plane_num, i, buf, -1, NULL));
    }

    CHECK_COMMAND(v4l2_streamon(clipper_id));

    int out_index = 0;
    int out_dq_index = 0;
    int out_q_count = 0;
    bool started_out = false;
    int capture_index = 0;
    int count = test_count;

    while (count >= 0) {
        struct nxp_vid_buffer *buf = &bufs[capture_index];
        CHECK_COMMAND(v4l2_dqbuf(clipper_id, buf->plane_num, &capture_index, NULL));
        //printf("====>capture_index: %d\n", capture_index);
        CHECK_COMMAND(v4l2_qbuf(video_id, buf->plane_num, out_index, buf, -1, NULL));

        out_q_count++;
        out_index++;
        out_index %= MAX_BUFFER_COUNT;

        if (!started_out) {
            CHECK_COMMAND(v4l2_streamon(video_id));
            started_out = true;
        }

        if (out_q_count >= 2) {
            CHECK_COMMAND(v4l2_dqbuf(video_id, buf->plane_num, &out_dq_index, NULL));
            out_q_count--;
        }

        CHECK_COMMAND(v4l2_qbuf(clipper_id, buf->plane_num, capture_index, buf, -1, NULL));
        count--;
    }

    CHECK_COMMAND(v4l2_streamoff(video_id));
    CHECK_COMMAND(v4l2_streamoff(clipper_id));

    v4l2_exit();
    close(ion_fd);

	if(count == -1)
	{
		printf("VIP Loopback Test Succeed!!\n");
		return 0;
	}
	else
		printf("VIP Loopback Test Fail!!\n");

	return -1;
}

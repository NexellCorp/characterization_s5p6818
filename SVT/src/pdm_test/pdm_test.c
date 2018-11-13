#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#define	OUT_FILE_NAME			"/tmp/pdm_test.wav"

#define TEST_COUNT				(1)
#define RECORD_TIME				(1)

int   debug   = 0;

#define DBG(x) if (debug) { printf x; }

void print_usage(void)
{
    printf( "usage: no options\n"
            " -r record test   (default: enable)\n"
            " -l loopback test (default: disable)\n"
			" -m mode select   (default: single-1) ex)double-2\n"
			" -C test count    (default: %d)\n"
			" -T recordtime      (default: %d secs)\n"
			" -v vervose       (default: disable)\n"
			, TEST_COUNT
			, RECORD_TIME
            );
}

int main(int argc, char **argv)
{
	FILE *fd;
	int opt, ret = 0;
	char cmdString[1024];
	int record = 1;
	int test_count = TEST_COUNT, index = 0;
	int record_time = RECORD_TIME;

    while (-1 != (opt = getopt(argc, argv, "hrlC:T:v"))) {
	switch (opt) {
        case 'h':	print_usage();  exit(0);	break;
        case 'r':	record = 1;					break;
        case 'l':	record = 0;					break;
        case 'C':   test_count = atoi(optarg);	break;
        case 'T':   record_time = atoi(optarg);	break;
		case 'v':	debug = 1;					break;
        default:
        	break;
		}
	}

	for (index = 0; test_count > index; index++) {
		if (record) {
			DBG(("---------PDM Record TEST---------\n"));

			sprintf(cmdString, "arecord -D default:CARD=PDMRecorder -f S16_LE -r 48000 -c 2 -d %d %s", record_time, OUT_FILE_NAME);
			ret = system(cmdString);
			if (ret != 0) {
				printf("pdm device is not found.\n");
				return -ENODEV;
			}
	
			fd = fopen(OUT_FILE_NAME,"rb");
			if (fd == NULL)	{
				printf("file open error!\n");
				return -EIO;
			}
		}
		else {
			DBG(("-----PDM Record & Playback TEST------\n"));
		
			sprintf( cmdString,
			"arecord -D default:CARD=PDMRecorder -f S16_LE -r 48000 -c 2 -d %d %s &" 
			"aplay -D default:CARD=I2SRT5631 %s"
			, record_time, OUT_FILE_NAME, OUT_FILE_NAME );
			system( cmdString );
	
			ret = system(cmdString);
			if (ret != 0) {
				printf("pdm device is not found.\n");
				return -ENODEV;
			}
		}

		if (ret != 0)
			printf("fail!\n");
		else
			printf("pass!\n");
	} // end loop test_count

	return ret;
}



#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <errno.h>
#include <time.h>
#include <linux/kernel.h>

#define TMU_TEMP_PATH0 "/sys/devices/c0000000.soc/c0096000.tmu-hwmon/temp_label"
#define TMU_TEMP_PATH1 "/sys/devices/c0000000.soc/c0097000.tmu-hwmon/temp_label"
//#define TMU_TEMP_PATH0 "sys/devices/platform/nxp-tmu.0/temp_label"
//#define TMU_TEMP_PATH1 "sys/devices/platform/nxp-tmu.1/temp_label"

#define TMU_LOW_RANGE 30
#define TMU_HIGH_RANGE 60


#define SUPPORT_CH1 1

void print_usage(void)
{
	printf( "usage: options\n"
	);
}
int main(int argc, char **argv)
{
	int fd,fd1, opt;
	char buf[4];
	int temp_0,temp_1, size;
	int ret = -1, temp_dif = 0;
	
    while (-1 != (opt = getopt(argc, argv, "c:hp:"))) {
		switch(opt) {
		case 'h' :      print_usage(); exit(0); break;
		}
	}
	fd = open(TMU_TEMP_PATH0,O_RDONLY);
	if(fd < 0)
	{
		printf("open test_fail TMU ch 0\n");
		return ret;
	}	
#if (SUPPORT_CH1)

	fd1 = open(TMU_TEMP_PATH1,O_RDONLY);
	if(fd1 < 0)
	{
		printf("open test_fail TMU ch 0\n");
		goto test_fail;
	}	
#endif
	size = read(fd,buf,4);
	if(size < 4)
	{
		printf("read temp test_end\n");
		goto test_fail;
	}
	
	temp_0 = atoi(buf);
	close(fd);

	printf("CH 0 : %s, %d \n",buf,temp_0 );
	
#if (SUPPORT_CH1)
	size = read(fd1,buf,4);
	if(size < 4)
	{
		printf("read temp test_end\n");
		goto test_fail;
	}
	temp_1 = atoi(buf);
	printf("CH 1 : %s, %d \n",buf,temp_1 );
	close(fd1);
#endif 
	if(!((temp_0 >= TMU_LOW_RANGE) && (temp_0 <= TMU_HIGH_RANGE)))
		goto test_fail;	
#if (SUPPORT_CH1)
	if(!((temp_1 >= TMU_LOW_RANGE) && (temp_1 <= TMU_HIGH_RANGE)))
		goto test_fail;

	temp_dif = abs(temp_0 - temp_1);

	if(temp_dif > 7)
		goto test_fail;
#endif

	if(fd)
		close(fd);
		
	if(fd1)
		close(fd1);
	printf(" TEST OK \n ");
	return 0;	
test_fail:
	
	
	if(fd)
		close(fd);
		
	if(fd1)
		close(fd1);

	return -1; 
}

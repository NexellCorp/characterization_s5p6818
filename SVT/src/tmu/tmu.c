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
#define TMU_TEMP_PATH "sys/devices/platform/nxp-tmu.0/temp_label"
//#define TMU_TEMP_PATH "sys/bus/platform/drivers/nxp-tmu/c0096000.tmu_0/temp_label"

void print_usage(void)
{
    printf( "usage: options\n"
	);
}

pthread_t m_thread_1;
pthread_t m_thread_2;
pthread_t m_thread_3;
pthread_t m_thread_4;
pthread_t m_thread_5;
pthread_t m_thread_6;
pthread_t m_thread_7;
pthread_t m_thread_8;

void *test_thread(void *data)
{
	while(1);
}

int main(int argc, char **argv)
{
	int fd;
	char buf[4];
	int temp_a,temp_b, size;
	fd = open(TMU_TEMP_PATH,O_RDONLY);
	int ret = -1, cnt = 0;
	if(fd < 0)
	{
		printf("open test_fail TMU\n");
		return ret;
	}
	
	
	
	size = read(fd,buf,4);
	if(size < 4)
	{
		printf("read temp test_end\n");
		return ret;
	}
	
	if(size < 4)
	{
		printf("read temp test_end\n");
		return ret;
	}
	temp_a = atoi(buf);
	printf("%s, %d \n",buf,temp_a );

	system ("echo 1400000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");

	if(pthread_create(&m_thread_1,NULL,test_thread, NULL) < 0)
	{
		printf("test_end Create Thread");
		goto test_end;
	}

	if(pthread_create(&m_thread_2,NULL,test_thread, NULL) < 0)
	{
		printf("test_end Create Thread");
		goto test_end;
	}
	if(pthread_create(&m_thread_3,NULL,test_thread, NULL) < 0)
	{
		printf("test_end Create Thread");
		goto test_end;
	}
	if(pthread_create(&m_thread_4,NULL,test_thread, NULL) < 0)
	{
		printf("test_end Create Thread");
		goto test_end;
	}
	if(pthread_create(&m_thread_5,NULL,test_thread, NULL) < 0)
	{
		printf("test_end Create Thread");
		goto test_end;
	}
	if(pthread_create(&m_thread_6,NULL,test_thread, NULL) < 0)
	{
		printf("test_end Create Thread");
		goto test_end;
	}
	if(pthread_create(&m_thread_7,NULL,test_thread, NULL) < 0)
	{
		printf("test_end Create Thread");
		goto test_end;
	}
	if(pthread_create(&m_thread_8,NULL,test_thread, NULL) < 0)
	{
		printf("test_end Create Thread");
		goto test_end;
	}
	do {
		sleep(1);
			
		if(fd)
			close(fd);
		
		fd = open(TMU_TEMP_PATH,O_RDONLY);
		if(fd < 0)
		{
			printf("open test_end TMU\n");
			goto test_end;
		}
	
		size = read(fd,buf,4);
		close(fd);
		
		if(size < 4)
		{
			printf("read temp test_end\n");
			goto test_end;
		}
		
		temp_b = atoi(buf);
		printf("%s, %d %d\n",buf,temp_b , cnt );
		
		if(temp_b > temp_a)
			ret = 0;
		
		cnt++;
	} while(ret == -1 && cnt <= 3);
	
	pthread_cancel(m_thread_1);	
	pthread_cancel(m_thread_2);	
	pthread_cancel(m_thread_3);	
	pthread_cancel(m_thread_4);	
	pthread_cancel(m_thread_5);	
	pthread_cancel(m_thread_6);	
	pthread_cancel(m_thread_7);	
	pthread_cancel(m_thread_8);	
	
	system ("echo 800000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
		
	return ret;
	
		
test_end:
	
	pthread_cancel(m_thread_1);	
	pthread_cancel(m_thread_2);	
	pthread_cancel(m_thread_3);	
	pthread_cancel(m_thread_4);	
	pthread_cancel(m_thread_5);	
	pthread_cancel(m_thread_6);	
	pthread_cancel(m_thread_7);	
	pthread_cancel(m_thread_8);	
	system ("echo 800000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
	if(fd)
		close(fd);
		
	return -1;
}


#include <stdio.h>
#include <stdlib.h>
#include "asv_type.h"
#include <unistd.h> /* for open/close .. */
#include "Fake_OS.h" /* for OS_GetTickCount */

int main( void )
{
	ASV_TEST_MODULE *test = Get3DTestModule();	
	ASV_RESULT res;
	unsigned int count = 10;
	unsigned int test_start_time, test_end_time;	
	
	test_start_time = (unsigned int)OS_GetTickCount();	  

	res = test->run();
	if( res < 0 )
		return res;

	while(count-->0)
	{
		usleep(7000000);	//	sleep 1sec
		res = test->status();
		if( res != ASV_RES_TESTING)
		{
			printf( "test done\n");
			break;
		}
	}
	res = test->stop();
	
	test_end_time = (unsigned int)OS_GetTickCount();	
	
	printf( "test end - time(%d), result(%d)\n", test_end_time-test_start_time, test->status());
	if(test->status() == ASV_RES_ERR) 
	{
		return -1; //fail
	}
	
	return 0;
}


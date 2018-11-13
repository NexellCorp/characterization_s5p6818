#include <stdio.h>
#if 1
#include <sys/ioctl.h>      //  _IOW
#include <fcntl.h>          //  open
#include <unistd.h>         //  close
//#include <linux/i2c.h>
//#include <linux/i2c-dev.h>
#endif
#include <pthread.h>
#include "NXP_MP2TSCaptureMgr.h"

static void mp2ts_chsel_func(U8);
//static void start_stream_capture(const char *fileName);
//static void stop_stream_capture(void);


#define CLEAR_STDIN while(getc(stdin) != '\n')

#ifdef __cplusplus
extern "C"
{
extern int      MxL603_main(void);
extern void     LGBB_TestMenu(void);
}
#endif

int main( int argc, char *argv[] )
{
 int key;
    while(1)
    {
        printf("============== [Select Capture Channel] ==================\n");
        printf("\t0: CH 0\n");
        printf("\t1: CH 1\n");
        printf("\t2: Core\n");
        printf("\t3: MxL603 Init\n");
        printf("\t4: LGDT3306 Init\n");
        printf("\tq or Q: Quit\n");
        printf("==========================================================\n");

        key = getc(stdin);
        CLEAR_STDIN;
        switch( key )
        {
            case '0':
                printf("[MP2TS] Capture ch0.\n");
                mp2ts_chsel_func( 0 );
                break;
            case '1':
                printf("[MP2TS] Capture ch1.\n");
                mp2ts_chsel_func( 1 );
                break;
            case '2':
                printf("[MP2TS] Core.\n");
                mp2ts_chsel_func( 2 );
                break;
			case '3':
				printf("[MP2TS] MxL603 Init.\n");
				//MxL603_main();
				break;
            case '4':
                printf("[MP2TS] LGDT3306 Init.\n");
                //LGBB_TestMenu( );
                break;

            case 'q':
            case 'Q':
                goto ATSC_EXIT;

            default:
                printf("[%c]\n", key);
        }
    }
ATSC_EXIT:
    printf("ATSC EXIT\n");
    return 0;    
}

static void mp2ts_chsel_func(U8 ch_num)
{
    int key;
//    unsigned int ch_num;
//    IOCTL_ATSC_SIGNAL_INFO sig_info;

    NXP_MP2TSCaptureMgr *pMgr = new NXP_MP2TSCaptureMgr;

    //    Device Driver Open
    if( !pMgr->Open() ){
        printf( "Cannot Open Device Driver\n" );
        return ;
    }

    while(1)
    {
        printf("===============================================\n");
        printf("\t0: MP2TS Power ON\n");
        printf("\t1: MP2TS Power OFF\n");
        printf("\t2: MP2TS Start TS\n");
        printf("\t3: MP2TS Stop TS\n");
        printf("\t4: MP2TS Buffer Alloc\n");
        printf("\t5: MP2TS Buffer Dealloc\n");
        printf("\t6: MP2TS Set Config\n");
        printf("\t7: MP2TS Get Config\n");
        printf("\t8: MP2TS Set Param\n");
        printf("\t9: MP2TS Get Lockstatus\n");
        printf("\ta: MP2TS Read Buffer\n");
        printf("\tb: MP2TS Write Buffer\n");
        printf("\tc: MP2TS Detect PID\n");
        printf("\td: MP2TS Decry Test\n");
        printf("\tq or Q: Quit\n");
        printf("===============================================\n");
           
        key = getc(stdin);
        CLEAR_STDIN;

        switch( key )
        {
            case '0':
                printf("[MP2TS Power ON]\n");
                pMgr->PowerOn(ch_num);
                break;

            case '1':    
                printf("[MP2TS Power OFF]\n");
                pMgr->PowerOff(ch_num);
                break;

            case '2':
                printf("[MP2TS Run TS]\n");
                pMgr->Run(ch_num);
                //start_stream_capture(NULL);
                break;

            case '3':
                printf("[MP2TS Stop TS]\n");
                //stop_stream_capture();
                pMgr->Stop(ch_num);
                break;

            case '4':
                printf("[MP2TS Buffer Alloc]\n");
                pMgr->Alloc(ch_num);
                break;

            case '5':
                printf("[MP2TS Buffer Dealloc]\n");
                pMgr->Dealloc(ch_num);
                break;

            case '6':
                printf("[MP2TS Set Config]\n");
                pMgr->SetConfig(ch_num);
                break;

            case '7':
                printf("[MP2TS Get Config]\n");
                pMgr->GetConfig(ch_num);
                break;

            case '8':
                printf("[MP2TS Set Param]\n");
                pMgr->SetParam(ch_num);
                break;

            case '9':
                printf("[MP2TS Get Lockstatus]\n");
                pMgr->GetChannelStatus();
                break;

            case 'a':
            case 'A':
                printf("[MP2TS Read Buffer]\n");
                pMgr->ReadBuff(ch_num);
                break;

            case 'b':
            case 'B':
                printf("[MP2TS Write Buffer]\n");
                pMgr->WriteBuff(ch_num);
                break;

            case 'c':
            case 'C':
                printf("[MP2TS Detect PID]\n");
                pMgr->DetectPID(ch_num);
                break;

            case 'd':
            case 'D':
                printf("[MP2TS Decry Test]\n");
                pMgr->CASDecryTest(ch_num);
                break;

            case 'q':
            case 'Q':
                goto ATSC_SUB_EXIT;

            default:
                printf("[%c]\n", key);
        }
        printf("\n");
    }

ATSC_SUB_EXIT:
    if( pMgr )
    {
        delete pMgr;
    }
    printf("ATSC SUB EXIT\n");
    
    return;
}


//
//    Stream capture
//
#if 0
static int isCapture = 0;
static int exitCapThread = 1;
pthread_t  g_hCapThread;

void *capture_thread( void *arg )
{
    int readSize;
    unsigned char *pBuf;
//    RAON_MTV350StreamReader *pReader = new RAON_MTV350StreamReader();
//    pReader->Start();
    while( !exitCapThread )
    {
        pBuf = NULL;
//        readSize = pReader->Read( &pBuf );
        printf("Read Size = %d, 0x%02x\n", readSize, pBuf[0]);
    }
//    pReader->Stop();
//    delete pReader;
    return (void*)0xdeaddead;
}

static void start_stream_capture(const char *fileName)
{
    if( !isCapture )
    {
        exitCapThread = 0;
        if( pthread_create( &g_hCapThread, NULL, capture_thread, (void*)fileName ) != 0 )
        {
            return;
        }
        isCapture = 1;
    }
}

static void stop_stream_capture(void)
{
    if( isCapture )
    {
        exitCapThread = 1;
        pthread_join( g_hCapThread, NULL );
        isCapture = 0;
    }
}
#endif


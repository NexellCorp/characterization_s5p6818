#ifndef __NXP_MP2TSCaptureMgr_h__
#define __NXP_MP2TSCaptureMgr_h__

#include "mtv_ioctl.h"

#define CFG_MPEGTS_IDMA_MODE	(0)


#define __ALIGN_MASK(x, mask)   (((x) + (mask)) & ~(mask))
#define __ALIGN(x, a)           __ALIGN_MASK(x, (typeof(x))(a) - 1)
#define ALLOC_ALIGN(size)       __ALIGN(size, 16)

#define NX_NO_PID           (0x1FFF)

#define NX_CAP_VALID_PID    (1 << 13)

#define NX_CSA_TYPE_PID     (1 << 15)
#define NX_AES_TYPE_PID     (0 << 15)

#define NX_AES_EVEN_PID     (0 << 14)
#define NX_AES_ODD_PID      (1 << 14)

#define NX_CAS_INTR_ENB     (1 << 13)

#define TS_PARAM_CAS_SIZE   (4 * 8) // 32byte

#define TS_CAPx_PID_MAX     (128)   // Unit: 2byte, 128 fileld
#define TS_CORE_PID_MAX     (16)    // Unit: 4byte, 16 fileld

#define TS_PACKET_SIZE      (188)   // Fixed, (CFG_MPEGTS_WORDCNT*4*16*8)
#if (CFG_MPEGTS_IDMA_MODE == 1)
#define TS_PACKET_NUM       (256)
#define TS_PAGE_NUM         (5)
#else
#define TS_PACKET_NUM       (80)	// Fixed, Maximum 80
#define TS_PAGE_NUM         (36)	// Variable
#endif
#define TS_PAGE_SIZE        (TS_PACKET_NUM * TS_PACKET_SIZE)
#define TS_BUF_TOTAL_SIZE   (TS_PAGE_NUM * TS_PAGE_SIZE)


#if 0
typedef enum {
    NXP_MP2TS_ID_CAP0,
    NXP_MP2TS_ID_CAP1,
    NXP_MP2TS_ID_CORE,
    NXP_MP2TS_ID_MAX
} NXP_MP2TS_ID;
#else

enum {
    NXP_MP2TS_ID_CAP0,
    NXP_MP2TS_ID_CAP1,
    NXP_MP2TS_ID_CORE,
    NXP_MP2TS_ID_MAX
};
#endif


enum {
    NXP_MP2TS_PARAM_TYPE_PID,
//    NXP_MP2TS_PARAM_TYPE_AES,   /* Core only        */
//    NXP_MP2TS_PARAM_TYPE_CSA,   /* Core only        */
    NXP_MP2TS_PARAM_TYPE_CAS,   /* Core only        */
    NXP_MP2TS_PARAM_TYPE_BUF,   /* Read Buffer only */
};

struct ts_op_mode {
    unsigned char   ch_num;
    unsigned char   tx_mode;            /* 0: rx mod, 1: tx mode    */
};

struct mp2ts_read_info {
    unsigned char   ch_num;
    unsigned char  *buf;
    unsigned int    count;
};

struct ts_config_desc {
    unsigned char       ch_num;
    union {
        unsigned char   data;
        struct {
        unsigned char   clock_pol   : 1;    /* 0: Invert, 1: Bypass */
        unsigned char   valid_pol   : 1;    /* 0: Active Low, 1: Active High */
        unsigned char   sync_pol    : 1;    /* 0: Active Low, 1: Active High */
        unsigned char   err_pol     : 1;    /* 0: Active Low, 1: Active High */
        unsigned char   data_width1 : 1;
        unsigned char   bypass_enb  : 1;
        unsigned char   xfer_mode   : 1;    /* Get status only  */
        unsigned char   xfer_clk_pol: 1;
        unsigned char   encry_on    : 1;    /* Core only */
        } bits;
    } un;
};


struct ts_param_info {
    union {
        unsigned int    data;
        struct {
        unsigned int    index       : 7;    /* CSA type : (CSA_IDX << 1) | (0 or 1) */
        unsigned int    type        : 2;    /* 0 : PID,     1 : CAS    */
        unsigned int    ch_num      : 2;    /* 0 : Ch0,     1 : Ch1,    2 : Core    */
        unsigned int    reserved    : 21;
        } bits;
    } un;
};

struct ts_param_descr {
    struct ts_param_info    info;
    void                   *buf;
    int                     buf_size;
    int                     wait_time;      // Uint : 10ms
    int                     read_count;     // for debug
    int                     ret_value;
};

struct ts_buf_init_info {
    unsigned char   ch_num;
    unsigned int    packet_size;
    unsigned int    packet_num;
    unsigned int    page_size;
    unsigned int    page_num;
};

struct ts_ch_info {
    bool    m_bPowerOn;
    bool    m_bStarted;
};


//------------------------------------------------------------------------------
//
//  Return values
//
#define ETS_NOERROR     0
#define ETS_WRITEBUF    1   /* copy_to_user or copy_from_user   */
#define ETS_READBUF     2   /* memory alloc */
#define ETS_FAULT       3   /* copy_to_user or copy_from_user   */
#define ETS_ALLOC       4   /* memory alloc */
#define ETS_RUNNING     5
#define ETS_TYPE        6
#define ETS_TIMEOUT     7


//------------------------------------------------------------------------------
//
//    NXP_MP2TSCaptureMgr
//
class NXP_MP2TSCaptureMgr
{
public:
    NXP_MP2TSCaptureMgr();
    virtual ~NXP_MP2TSCaptureMgr();

    //    Device Driver Open/Close
    bool    Open();
    bool    Close();

    //    Device Power On/Off
    bool    PowerOn(U8 ch_num);
    bool    PowerOff(U8 ch_num);

    //    Channel Select or Set
    bool    ScanAll();                      //    Scan all frequency channel
    bool    ScanSingle( int chNum );        //    Scan Single Channel
    bool    SetChannel( int chNum );        //    Select Channel

    //    Data Streaming Start/Stop
    bool    Run(U8 ch_num);
    bool    Stop(U8 ch_num);

    //    Driver Buffer Alloc/ Dealloc
    bool    Alloc(U8 ch_num);
    bool    Dealloc(U8 ch_num);

    bool    SetConfig(U8 ch_num);
    bool    GetConfig(U8 ch_num);

    bool    SetParam(U8 ch_num);
    bool    GetParam(U8 ch_num);

    bool    GetChannelStatus();             //    
    bool    GetSignalStatus(IOCTL_ISDBT_SIGNAL_INFO &signalInf);    //

    bool    ReadBuff(U8 ch_num);
    bool    WriteBuff(U8 ch_num);

    bool    DetectPID(U8 ch_num);
    bool    CASDecryTest(U8 ch_num);


private:
#if 0
//    int     m_AreaIndex;
    bool    m_bOpened;
    bool    m_bPowerOn;
//    bool    m_bLocked;
    bool    m_bStarted;

    //    Device Driver
    int     m_hDrv;
//    int     m_CurChNum;
#else

	struct ts_ch_info	m_ch_info[NXP_MP2TS_ID_MAX];

	bool	m_bOpened;
	int 	m_hDrv;
#endif
};

#endif    //    __NXP_MP2TSCaptureMgr_h__

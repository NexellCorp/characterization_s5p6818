#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>      //  _IOW
#include <fcntl.h>          //  open
#include <unistd.h>         //  close

#include "NXP_MP2TSCaptureMgr.h"
#include "DbgMsg.h"

#define CFG_LGDT3305    (1)
#define CFG_SMS3230     (0)

#define CFG_BYPASS_DUMP (0)

#define CFG_ATSC_TEST   (1)
#define CFG_ISDBT_TEST  (0)

#define CFG_AES_TEST    (0)
#define CFG_CSA_TEST    (1)


#define DTAG            "[LG3305|ChannelMgr] "
#define DBG_INFO        1
#define DBG_DBG         1
#define DBG_VBS         0
#define DBG_FUNC        0

#define CURRENT_NEXT_INDICATOR_BYTE_OFFSET  (5)
#define CURRENT_NEXT_INDICATOR_BIT_OFFSET   (7)
#define CURRENT_NEXT_INDICATOR_BITS_MASK    (0x01)

#define PAT_SECTION_COUNT_BYTE_OFFSET       (8)
#define PAT_SECTION_COUNT_BIT_OFFSET        (0)
#define PAT_SECTION_COUNT_BITS_MASK         (0xFF)

#define PSI_ID_PAT                          (0x00)
#define PSI_ID_PMT                          (0x02)
#define PSI_ID_CAT                          (0x01)

unsigned short PIDs[] = {
    0x0c11, /* MBC */
    0x0011,
    0x0014,

    0x0411, /* KBS */
    0x0021,
    0x0024,
    0x0034,

    0x0001, /* SBS */
    0x0011,
    0x0014,
};


struct PAT_SECTION {
    unsigned char   program_num1;
    unsigned char   program_num0;

    unsigned char   program_map_PID1    : 5;
    unsigned char   reserved            : 3;

    unsigned char   program_map_PID0;
};

// Big enddian
struct TS_PACKET {
    unsigned char   sync_byte;

    unsigned char   PID1                            : 5;
    unsigned char   transport_priority              : 1;
    unsigned char   payload_unit_start_indicator    : 1;
    unsigned char   transport_error_indicator       : 1;

    unsigned char   PID0                            : 8;

    unsigned char   continuity_counter              : 4;
    unsigned char   adaptation_field_control        : 2;
    unsigned char   transport_scrambling_control    : 2;
    unsigned char   data_byte                       : 8;
};

struct adaptation_filed {
    unsigned char   adaptation_field_length             : 8;

    unsigned char   adaptation_field_extension_flag     : 1;
    unsigned char   transport_private_data_flag         : 1;
    unsigned char   splicing_point_flag                 : 1;
    unsigned char   OPCR_flag                           : 1;
    unsigned char   PCR_flag                            : 1;
    unsigned char   elementary_stream_priority_indicator    : 1;
    unsigned char   random_access_indicator             : 1;
    unsigned char   discontinuity_indicator             : 1;

    unsigned char   program_clock_reference_base4       : 8;
    unsigned char   program_clock_reference_base3       : 8;
    unsigned char   program_clock_reference_base2       : 8;
    unsigned char   program_clock_reference_base1       : 8;

    unsigned char   program_clock_reference_extension1  : 1;
    unsigned char   reserved                            : 6;
    unsigned char   program_clock_reference_base0       : 1;

    unsigned char   program_clock_reference_extension0  : 8;


    unsigned char   splice_countdown                    : 8;
    unsigned char   stuffing_bytes                      : 8;
};

struct PAT {
    unsigned char   table_id                    : 8;

    unsigned char   section_length1             : 4;
    unsigned char   reserved0                   : 2;
//    unsigned char   '0'                         : 1;
    unsigned char   section_syntax_indicator    : 1;

    unsigned char   section_length0             : 8;

    unsigned char   transport_stream_id1        : 8;
    unsigned char   transport_stream_id0        : 8;

    unsigned char   current_next_indicator      : 1;
    unsigned char   version_number              : 5;
    unsigned char   reserved1                   : 2;

    unsigned char   section_number              : 8;
    unsigned char   last_section_number         : 8;
};

struct CAT {
    unsigned char   table_id                    : 8;

    unsigned char   section_length1             : 4;
    unsigned char   reserved0                   : 2;
//    unsigned char   '0'                         : 1;
    unsigned char   section_syntax_indicator    : 1;

    unsigned char   reserved1                   : 8;
    unsigned char   reserved2                   : 8;

    unsigned char   current_next_indicator      : 1;
    unsigned char   version_number              : 5;
    unsigned char   reserved3                   : 2;

    unsigned char   section_number              : 8;
    unsigned char   last_section_number         : 8;
};

struct PMT {
    unsigned char   table_id                    : 8;

    unsigned char   section_length1             : 4;
    unsigned char   reserved0                   : 2;
//    unsigned char   '0'                         : 1;
    unsigned char   section_syntax_indicator    : 1;

    unsigned char   section_length0             : 8;

    unsigned char   program_number1             : 8;
    unsigned char   program_number0             : 8;

    unsigned char   current_next_indicator      : 1;
    unsigned char   version_number              : 5;
    unsigned char   reserved1                   : 2;

    unsigned char   section_number              : 8;
    unsigned char   last_section_number         : 8;

    unsigned char   PCR_PID1                    : 5;
    unsigned char   reserved2                   : 3;
    unsigned char   PCR_PID0                    : 8;

    unsigned char   program_info_length1        : 4;
    unsigned char   reserved3                   : 4;
    unsigned char   program_info_length0        : 8;

};

//static const char *isdbt_area_str[] = {"Japan", "Latin America"};

//    ISDBT_AREA_JAPAN or ISDBT_AREA_LATIN_AMERICA
#if 0
NXP_MP2TSCaptureMgr::NXP_MP2TSCaptureMgr( int areaIndex )
    : m_AreaIndex( areaIndex )
    , m_bOpened( false )
    , m_bPowerOn( false )
    , m_bLocked( false )
    , m_bStarted( false )
    , m_hDrv( 0 )
{
}
#else

NXP_MP2TSCaptureMgr::NXP_MP2TSCaptureMgr()
    : m_bOpened( false )
//    , m_bPowerOn( false )
//    , m_bLocked( false )
//    , m_bStarted( false )
    , m_hDrv( 0 )
{
    U8 ch_num;

    for(ch_num = 0; ch_num < NXP_MP2TS_ID_MAX; ch_num++)
    {
        m_ch_info[ch_num].m_bPowerOn = 0;
        m_ch_info[ch_num].m_bStarted = 0;
    }
}
#endif

NXP_MP2TSCaptureMgr::~NXP_MP2TSCaptureMgr()
{
    if(m_bOpened)
        Close();
}

//    Device Driver Open
bool NXP_MP2TSCaptureMgr::Open()
{
    bool ret = false;

    FUNC_IN;
    if( !m_bOpened )
    {
        char devName[32];
        sprintf(devName, "/dev/%s", MP2TS_DEV_NAME );
        m_hDrv = open(devName, O_RDWR);
        if( m_hDrv < 0 )
        {
            m_hDrv = 0;
            goto exit_Open;
        }
        m_bOpened = true;
    }

    ret = true;
    FUNC_OUT;

exit_Open:
    return ret;
}

//    Device Driver Close
bool NXP_MP2TSCaptureMgr::Close()
{
    U8 ch_num;

    FUNC_IN;
    if( m_bOpened )
    {
        for(ch_num = 0; ch_num < NXP_MP2TS_ID_MAX; ch_num++)
        {
            Stop(ch_num);
        }
        for(ch_num = 0; ch_num < NXP_MP2TS_ID_MAX; ch_num++)
        {
            PowerOff(ch_num);
        }

        if( m_hDrv )
        {
            close( m_hDrv );
            m_hDrv = 0;
        }
        m_bOpened = false;
    }
    FUNC_IN;
    return true;
}

bool NXP_MP2TSCaptureMgr::PowerOn(U8 ch_num)
{
    bool ret = false;

    FUNC_IN;

    if (m_ch_info[ch_num].m_bPowerOn == true)
        goto exit_PowerOn;

    if(ioctl(m_hDrv, IOCTL_MPEGTS_POWER_ON, &ch_num) < 0)
    {
        ErrMsg("IOCTL_MPEGTS_POWER_ON failed: %d\n", ret);
        goto exit_PowerOn;
    }
    m_ch_info[ch_num].m_bPowerOn = true;

    ret = true;

exit_PowerOn:
    FUNC_OUT;

    return ret;
}

bool NXP_MP2TSCaptureMgr::PowerOff(U8 ch_num)
{
    bool ret = false;

    FUNC_IN;

    if (m_ch_info[ch_num].m_bPowerOn == false)
        goto exit_PowerOff;

//        Stop(ch_num);
    if(ioctl(m_hDrv, IOCTL_MPEGTS_POWER_OFF, &ch_num) < 0)
    {
        ErrMsg("IOCTL_MPEGTS_POWER_OFF failed\n");
        goto exit_PowerOff;
    }
    m_ch_info[ch_num].m_bPowerOn = false;

    ret = true;

exit_PowerOff:
    FUNC_OUT;

    return ret;
}

//    Start Data Streaming
bool NXP_MP2TSCaptureMgr::Run(U8 ch_num)
{
    struct ts_op_mode   ts_op;
    bool ret = false;

    FUNC_IN;

    if (m_ch_info[ch_num].m_bStarted == true)
        goto exit_Run;

    ts_op.ch_num    = ch_num;
    ts_op.tx_mode   = false;
//    ts_op.tx_mode   = true;

    if(ioctl(m_hDrv, IOCTL_MPEGTS_RUN, &ts_op) != 0)
    {
        ErrMsg("IOCTL_MPEGTS_CAPTURE_START failed\n");
        goto exit_Run;
    }
    m_ch_info[ch_num].m_bStarted = true;

    ret = true;

exit_Run:
    FUNC_OUT;

    return ret;
}

//    Stop Data Streaming
bool NXP_MP2TSCaptureMgr::Stop(U8 ch_num)
{
    bool ret = false;

    FUNC_IN;

    if (m_ch_info[ch_num].m_bStarted == false)
        goto exit_Stop;

    if( ioctl(m_hDrv, IOCTL_MPEGTS_STOP, &ch_num) < 0 )
    {
        ErrMsg("IOCTL_MPEGTS_CAPTURE_STOP failed\n");
        goto exit_Stop;
    }
    m_ch_info[ch_num].m_bStarted = false;

    ret = true;

exit_Stop:
    FUNC_OUT;

    return ret;
}

bool NXP_MP2TSCaptureMgr::Alloc(U8 ch_num)
{
    struct ts_buf_init_info buf_info;
    bool ret = false;

    FUNC_IN;

    if ((m_ch_info[ch_num].m_bPowerOn == false) || (m_ch_info[ch_num].m_bStarted == true))
        goto exit_Alloc;

    buf_info.ch_num         = ch_num;
    buf_info.packet_size    = TS_PACKET_SIZE;
    buf_info.packet_num     = TS_PACKET_NUM;
    buf_info.page_size      = (TS_PACKET_SIZE * TS_PACKET_NUM);
    buf_info.page_num       = TS_PAGE_NUM;

    if( ioctl(m_hDrv, IOCTL_MPEGTS_DO_ALLOC, &buf_info) < 0 )
    {
        ErrMsg("IOCTL_MPEGTS_DO_ALLOC failed\n");
        goto exit_Alloc;
    }

    ret = true;

exit_Alloc:
    FUNC_OUT;

    return ret;
}

bool NXP_MP2TSCaptureMgr::Dealloc(U8 ch_num)
{
    bool ret = false;

    FUNC_IN;

    if ((m_ch_info[ch_num].m_bPowerOn == false) || (m_ch_info[ch_num].m_bStarted == true))
        goto exit_Dealloc;

    if( ioctl(m_hDrv, IOCTL_MPEGTS_DO_DEALLOC, &ch_num) < 0 )
    {
        ErrMsg("IOCTL_MPEGTS_DO_DEALLOC failed\n");
        goto exit_Dealloc;
    }

    ret = true;

exit_Dealloc:
    FUNC_OUT;

    return ret;
}

bool NXP_MP2TSCaptureMgr::SetConfig(U8 ch_num)
{
    struct ts_config_desc   config_descr;
    bool ret = false;

    FUNC_IN;

    if (m_ch_info[ch_num].m_bPowerOn == false)
        goto exit_SetConfig;

    config_descr.ch_num                 = ch_num;

//ATSC : LGDT3305
#if (CFG_LGDT3305 == 1)
    config_descr.un.bits.clock_pol      = 1;
    config_descr.un.bits.valid_pol      = 1;
    config_descr.un.bits.sync_pol       = 1;
//    config_descr.un.bits.err_pol        = 1;    // Sharp
    config_descr.un.bits.err_pol        = 0;
    config_descr.un.bits.data_width1    = 0;
#if (CFG_BYPASS_DUMP == 1)
    config_descr.un.bits.bypass_enb     = 1;
#else
    config_descr.un.bits.bypass_enb     = 0;
#endif
//    config_descr.un.bits.xfer_mode      = 0;    /* Ch1 Only     */
    config_descr.un.bits.xfer_mode      = 1;    /* Ch1 Only     */
    config_descr.un.bits.encry_on       = 0;    /* TScore Only  */
#endif

//ISDB-T : sms3230
#if (CFG_SMS3230 == 1)
    config_descr.un.bits.clock_pol      = 1;
    config_descr.un.bits.valid_pol      = 0;
    config_descr.un.bits.sync_pol       = 0;
    config_descr.un.bits.err_pol        = 1;
    config_descr.un.bits.data_width1    = 1;
#if (CFG_BYPASS_DUMP == 1)
    config_descr.un.bits.bypass_enb     = 1;
#else
    config_descr.un.bits.bypass_enb     = 0;
#endif
//    config_descr.un.bits.xfer_mode      = 0;    /* Ch1 Only     */
    config_descr.un.bits.xfer_mode      = 1;    /* Ch1 Only     */
    config_descr.un.bits.encry_on       = 0;    /* TScore Only  */
#endif

    if( ioctl(m_hDrv, IOCTL_MPEGTS_SET_CONFIG, &config_descr) < 0 )
    {
        ErrMsg("IOCTL_MPEGTS_SET_CONFIG failed\n");
        goto exit_SetConfig;
    }

    ret = true;

exit_SetConfig:

    FUNC_OUT;

    return ret;
}

bool NXP_MP2TSCaptureMgr::GetConfig(U8 ch_num)
{
    struct ts_config_desc   config_descr;
    bool ret = false;

    FUNC_IN;

    if (m_ch_info[ch_num].m_bPowerOn == false)
        goto exit_GetConfig;

    config_descr.ch_num                 = ch_num;
#if 0
    config_descr.un.bits.clock_pol      = 0;
    config_descr.un.bits.valid_pol      = 0;
    config_descr.un.bits.sync_pol       = 0;
    config_descr.un.bits.err_pol        = 0;
    config_descr.un.bits.data_width1    = 0;
    config_descr.un.bits.xfer_mode      = 0;
    config_descr.un.bits.encry_on       = 0;
#else

    config_descr.un.data                = 0;
#endif
    if( ioctl(m_hDrv, IOCTL_MPEGTS_GET_CONFIG, &config_descr) < 0 )
    {
        ErrMsg("IOCTL_MPEGTS_GET_CONFIG failed\n");
        goto exit_GetConfig;
    }

    printf("config_descr = 0x%08x\n", config_descr.un.data);

    ret = true;

exit_GetConfig:
    FUNC_OUT;

    return ret;
}


// ATSC
#if (CFG_ATSC_TEST == 1)
static U16 NX_CAP_PIDs[] = {
    NX_CAP_VALID_PID | 0x0000,  /* PAT  */
    NX_CAP_VALID_PID | 0x0001,  /* CAT  */

    NX_CAP_VALID_PID | 0x0c11,  /* MBC */
    NX_CAP_VALID_PID | 0x0011,
    NX_CAP_VALID_PID | 0x0014,

    NX_CAP_VALID_PID | 0x0411,  /* KBS */
    NX_CAP_VALID_PID | 0x0021,
    NX_CAP_VALID_PID | 0x0024,
    NX_CAP_VALID_PID | 0x0034,

    NX_CAP_VALID_PID | 0x0001,  /* SBS */
    NX_CAP_VALID_PID | 0x0011,
    NX_CAP_VALID_PID | 0x0014,
};
#endif

// ISDB-T
#if (CFG_ISDBT_TEST == 1)
static U16 NX_CAP_PIDs[] = {
    NX_CAP_VALID_PID | 0x0000,  /* PAT  */

    NX_CAP_VALID_PID | 0x0010,  /* NIT */
    NX_CAP_VALID_PID | 0x0011,  /* SDT */
    NX_CAP_VALID_PID | 0x0012,

    NX_CAP_VALID_PID | 0x0014,  /* TDT/TOT */
    NX_CAP_VALID_PID | 0x0020,
    NX_CAP_VALID_PID | 0x0024,
    NX_CAP_VALID_PID | 0x0027,
    NX_CAP_VALID_PID | 0x006B,
    NX_CAP_VALID_PID | 0x00C9,
    NX_CAP_VALID_PID | 0x00CA,
    NX_CAP_VALID_PID | 0x0100,
    NX_CAP_VALID_PID | 0x0101,
    NX_CAP_VALID_PID | 0x0102,
    NX_CAP_VALID_PID | 0x0103,
    NX_CAP_VALID_PID | 0x0111,
    NX_CAP_VALID_PID | 0x0112,
    NX_CAP_VALID_PID | 0x01FF,
    NX_CAP_VALID_PID | 0x0200,
    NX_CAP_VALID_PID | 0x03E8,
    NX_CAP_VALID_PID | 0x1FC8,
    NX_CAP_VALID_PID | 0x1FFF,
};
#endif

#if (CFG_AES_TEST == 1)
static U16 NX_AES_PIDs[] = {
    NX_AES_TYPE_PID | 0x0014,                       /* AES Audio        */  /* Cleared      */
    NX_AES_TYPE_PID | 0x0011 | NX_AES_EVEN_PID,     /* AES Video, Even  */  /* Scrambled    */
    NX_AES_TYPE_PID | 0x0011 | NX_AES_ODD_PID,      /* AES Video, Odd   */  /* Scrambled    */
    NX_AES_TYPE_PID | 0x0001,                       /* CAT  */
    NX_AES_TYPE_PID | 0x0000,                       /* PAT  */
    NX_AES_TYPE_PID | 0x1ffb,
    NX_AES_TYPE_PID | 0x0088,
    NX_AES_TYPE_PID | 0x0020,
    NX_AES_TYPE_PID | 0x0002,
    NX_AES_TYPE_PID | NX_NO_PID,
};

static U32 NX_AES_KEYs[] = {

    0x00000000,     /* AES Audio    */
    0x00000000,     /* AES Audio    */
    0x00000000,     /* AES Audio    */
    0x00000000,     /* AES Audio    */
    0x00000000,     /* AES Audio    */
    0x00000000,     /* AES Audio    */
    0x00000000,     /* AES Audio    */
    0x00000000,     /* AES Audio    */

#if 1
    0x84be2300,     /* AES Video, Even CW   */
    0xaed66ce1,     /* AES Video, Even CW   */
    0xf1499000,     /* AES Video, Even CW   */
    0xebe9bbf1,     /* AES Video, Even CW   */
    0x00000000,     /* AES Video, Even IV    */
    0x00000000,     /* AES Video, Even IV    */
    0x00000000,     /* AES Video, Even IV    */
    0x00000000,     /* AES Video, Even IV    */

    0x3cdba6b3,     /* AES Video, Odd CW    */
    0x993e0c87,     /* AES Video, Odd CW    */
    0x1c0d5e24,     /* AES Video, Odd CW    */
    0xde47b706,     /* AES Video, Odd CW    */
    0x00000000,     /* AES Video, Odd IV    */
    0x00000000,     /* AES Video, Odd IV    */
    0x00000000,     /* AES Video, Odd IV    */
    0x00000000,     /* AES Video, Odd IV    */
#else

    0x0023be84,     /* AES Video, Even CW   */
    0xe16cd6ae,     /* AES Video, Even CW   */
    0x009049f1,     /* AES Video, Even CW   */
    0xf1bbe9eb,     /* AES Video, Even CW   */
    0x00000000,     /* AES Video, Even IV    */
    0x00000000,     /* AES Video, Even IV    */
    0x00000000,     /* AES Video, Even IV    */
    0x00000000,     /* AES Video, Even IV    */

    0xb3a6db3c,     /* AES Video, Odd CW    */
    0x870c3e99,     /* AES Video, Odd CW    */
    0x245e0d1c,     /* AES Video, Odd CW    */
    0x06b747de,     /* AES Video, Odd CW    */
    0x00000000,     /* AES Video, Odd IV    */
    0x00000000,     /* AES Video, Odd IV    */
    0x00000000,     /* AES Video, Odd IV    */
    0x00000000,     /* AES Video, Odd IV    */
#endif
};
#endif  // #if (CFG_AES_TEST == 1)

#if (CFG_CSA_TEST == 1)
static U16 NX_CSA_PIDs[] = {
    NX_CSA_TYPE_PID | 0x0011,
    NX_CSA_TYPE_PID | 0x0010,
    NX_CSA_TYPE_PID | 0x0101,       /* CSA Audio        */  /* Scrambled    */
    NX_CSA_TYPE_PID | 0x0102,       /* CSA Video        */  /* Scrambled    */
    NX_CSA_TYPE_PID | 0x0000,       /* PAT  */
    NX_CSA_TYPE_PID | 0x0100,
    NX_CSA_TYPE_PID | 0x0001,       /* CAT  */
    NX_CSA_TYPE_PID | 0x0014,
    NX_CSA_TYPE_PID | 0x0030,
    NX_CSA_TYPE_PID | NX_NO_PID,
};

static U32 NX_CSA_KEYs[] = {

    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,

#if 1
    0x7a9501e4,     /* CSA Audio & Video Odd CW     */
    0xe8063ca6,     /* CSA Audio & Video Odd CW     */
    0x269e3b4d,     /* CSA Audio & Video Odd CW     */
    0x66601aec,     /* CSA Audio & Video Odd CW     */
    0x7a9501e4,     /* CSA Audio & Video Even CW    */
    0xe8063ca6,     /* CSA Audio & Video Even CW    */
    0x269e3b4d,     /* CSA Audio & Video Even CW    */
    0x66601aec,     /* CSA Audio & Video Even CW    */
#else

    0xe401957a,     /* CSA Audio & Video Odd CW     */
    0xa63c06e8,     /* CSA Audio & Video Odd CW     */
    0x4d3b9e26,     /* CSA Audio & Video Odd CW     */
    0xec1a6066,     /* CSA Audio & Video Odd CW     */
    0xe401957a,     /* CSA Audio & Video Even CW    */
    0xa63c06e8,     /* CSA Audio & Video Even CW    */
    0x4d3b9e26,     /* CSA Audio & Video Even CW    */
    0xec1a6066,     /* CSA Audio & Video Even CW    */
#endif
};
#endif  //#if (CFG_CSA_TEST == 1)

bool NXP_MP2TSCaptureMgr::SetParam(U8 ch_num)
{
    struct ts_param_descr   param_descr;
    bool    ret = false;

    FUNC_IN;

    if (m_ch_info[ch_num].m_bPowerOn == false)
        goto exit_SetParam;

    if (ch_num == NXP_MP2TS_ID_CORE)
    {
#if (CFG_AES_TEST == 1)
        // set AES PIDs & KEYs
        param_descr.info.un.bits.ch_num = ch_num;
        param_descr.info.un.bits.type   = NXP_MP2TS_PARAM_TYPE_PID;
        param_descr.buf                 = (void *)NX_AES_PIDs;
        param_descr.buf_size            = sizeof(NX_AES_PIDs);

        if( ioctl(m_hDrv, IOCTL_MPEGTS_SET_PARAM, &param_descr) < 0 )
        {
            ErrMsg("IOCTL_MPEGTS_SET_PARAM : NX_AES_PIDs failed\n");
            goto exit_SetParam;
        }

        param_descr.info.un.bits.ch_num = ch_num;
        param_descr.info.un.bits.type   = NXP_MP2TS_PARAM_TYPE_CAS;
        param_descr.buf                 = (void *)NX_AES_KEYs;
        param_descr.buf_size            = sizeof(NX_AES_KEYs);

        if( ioctl(m_hDrv, IOCTL_MPEGTS_SET_PARAM, &param_descr) < 0 )
        {
            ErrMsg("IOCTL_MPEGTS_SET_PARAM : NX_AES_KEYs failed\n");
            goto exit_SetParam;
        }
#endif  //#if (CFG_AES_TEST == 1)

#if (CFG_CSA_TEST == 1)
        // set CSA PIDs & KEYs
        param_descr.info.un.bits.ch_num = ch_num;
        param_descr.info.un.bits.type   = NXP_MP2TS_PARAM_TYPE_PID;
        param_descr.buf                 = (void *)NX_CSA_PIDs;
        param_descr.buf_size            = sizeof(NX_CSA_PIDs);

        if( ioctl(m_hDrv, IOCTL_MPEGTS_SET_PARAM, &param_descr) < 0 )
        {
            ErrMsg("IOCTL_MPEGTS_SET_PARAM : NX_CSA_PIDs failed\n");
            goto exit_SetParam;
        }

        param_descr.info.un.bits.ch_num = ch_num;
        param_descr.info.un.bits.type   = NXP_MP2TS_PARAM_TYPE_CAS;
        param_descr.buf                 = (void *)NX_CSA_KEYs;
        param_descr.buf_size            = sizeof(NX_CSA_KEYs);

        if( ioctl(m_hDrv, IOCTL_MPEGTS_SET_PARAM, &param_descr) < 0 )
        {
            ErrMsg("IOCTL_MPEGTS_SET_PARAM : NX_AES_KEYs failed\n");
            goto exit_SetParam;
        }
#endif  //#if (CFG_CSA_TEST == 1)
    }
    else
    {
#if (CFG_BYPASS_DUMP == 1)
        void *clear_buf = malloc(64);

        if (clear_buf)
        {
            memset( clear_buf, 0x00, sizeof(clear_buf) );

            // set PIDs
            param_descr.info.un.bits.ch_num = ch_num;
            param_descr.info.un.bits.type   = NXP_MP2TS_PARAM_TYPE_PID;
            param_descr.buf                 = (void *)clear_buf;
            param_descr.buf_size            = sizeof(clear_buf);

            free(clear_buf);
        }
#else

        // set PIDs
        param_descr.info.un.bits.ch_num = ch_num;
        param_descr.info.un.bits.type   = NXP_MP2TS_PARAM_TYPE_PID;
        param_descr.buf                 = (void *)NX_CAP_PIDs;
        param_descr.buf_size            = sizeof(NX_CAP_PIDs);
#endif
        if( ioctl(m_hDrv, IOCTL_MPEGTS_SET_PARAM, &param_descr) < 0 )
        {
            ErrMsg("IOCTL_MPEGTS_SET_PARAM : NX_CAP_PIDs failed\n");
            goto exit_SetParam;
        }
    }

    ret = true;

exit_SetParam:
    FUNC_OUT;

    return ret;
}

bool NXP_MP2TSCaptureMgr::GetParam(U8 ch_num)
{
    struct ts_param_descr   param_descr;
    bool    ret = false;

    FUNC_IN;

    if (m_ch_info[ch_num].m_bPowerOn == false)
        goto exit_GetParam;

//    malloc(2048);    

    param_descr.info.un.bits.ch_num = ch_num;
    param_descr.info.un.bits.type   = NXP_MP2TS_PARAM_TYPE_PID;
//    param_descr.buf                     = ;
    param_descr.buf_size            = TS_PARAM_CAS_SIZE * 2;

    if( ioctl(m_hDrv, IOCTL_MPEGTS_GET_PARAM, &param_descr) < 0 )
    {
        ErrMsg("IOCTL_MPEGTS_GET_PARAM failed\n");
        goto exit_GetParam;
    }

    ret = true;

exit_GetParam:
    FUNC_OUT;

    return ret;
}

//    Current
bool NXP_MP2TSCaptureMgr::GetChannelStatus()
{
    unsigned int lock_mask;

    if(ioctl(m_hDrv, IOCTL_MPEGTS_GET_LOCK_STATUS, &lock_mask) == 0)
    {
        DbgMsg(DBG_DBG, "lock_mask = %d\n", lock_mask);            
    }
    else
    {
        ErrMsg("IOCTL_MPEGTS_GET_LOCK_STATUS failed\n");
    }

    return true;
}


//    Read Buffer
#define READ_BUFF_TEST  (1)
bool NXP_MP2TSCaptureMgr::ReadBuff(U8 ch_num)
{
    struct ts_param_descr   param_descr;
    bool    ret = false;
    int     idx, count, alloc_size, read_size, read_count;
    void    *buf_addr[TS_PAGE_NUM];
#if (READ_BUFF_TEST == 1)
    FILE    *fp;
    int     wSize = 0;
#endif

    FUNC_IN;

    if ((m_ch_info[ch_num].m_bPowerOn == false) || (m_ch_info[ch_num].m_bStarted == false))
        goto exit_ReadBuff;

#if (READ_BUFF_TEST == 1)
    fp = fopen( "/tmp/mp2ts_read.ts", "wb" );
    if( fp == 0 )
    {
        ErrMsg("ReadBuff : file open failed\n");

        goto exit_ReadBuff; // file open failed !!!
    }

    read_size   = (50 * 0x100000);
    alloc_size  = (TS_PAGE_SIZE * 1);
    read_count  = (read_size / alloc_size);
DbgMsg(DBG_DBG, "ReadBuff : read count = %d\n", read_count);

    for (count = 0; count < TS_PAGE_NUM; count++)
    {
        buf_addr[count] = malloc(alloc_size);
    }

    param_descr.info.un.bits.ch_num = ch_num;
    param_descr.info.un.bits.type   = NXP_MP2TS_PARAM_TYPE_BUF;
    param_descr.buf_size            = alloc_size;
    param_descr.read_count          = read_count;
    param_descr.wait_time           = 0;

    idx = 0;
    param_descr.buf = (void *)buf_addr[idx];
    for (count = 0; count < read_count; )
    {
        if(ioctl(m_hDrv, IOCTL_MPEGTS_READ_BUF, &param_descr) < 0)
        {
//            ErrMsg("IOCTL_MPEGTS_READ_BUF failed\n");
            continue;
        }

#if 0
        write( fp, (unsigned char*)(buf_addr[idx]), alloc_size );
#else
        wSize = fwrite( (unsigned char*)(buf_addr[idx]), 1, alloc_size, fp);
        if( wSize != alloc_size )
        {
            printf("!!!! Error !!!!(wSize = %d, alloc_size = %d\n", 
                wSize, alloc_size);
        }
#endif

        count++;
        idx = (idx + 1) % TS_PAGE_NUM;
        param_descr.buf = (void *)buf_addr[idx];
    }


for (count = 0; count < 20; count++)
{
DbgMsg(DBG_DBG, "ReadBuff : file close\n");
}
#else

    buf_addr[0] = malloc(alloc_size);

    param_descr.info.un.bits.ch_num = ch_num;
    param_descr.info.un.bits.type   = NXP_MP2TS_PARAM_TYPE_BUF;
    param_descr.buf                 = (void *)buf_addr[0];
    param_descr.buf_size            = alloc_size;
    param_descr.wait_time           = 0;
//    param_descr.wait_time           = 2;    // Uint : 10ms => 20ms
    param_descr.read_count          = 200;

    if(ioctl(m_hDrv, IOCTL_MPEGTS_READ_BUF, &param_descr) < 0)
    {
        ErrMsg("IOCTL_MPEGTS_READ_BUF failed\n");
        goto exit_ReadBuff;
    }
#endif

    ret = true;

exit_ReadBuff:
    FUNC_OUT;

#if (READ_BUFF_TEST == 1)
    if( fp )
    {
        fclose( fp );
    }
    
    for (count = 0; count < TS_PAGE_NUM; count++)
    {
        free(buf_addr[count]);
    }
#else
    free(buf_addr[0]);
#endif
    return ret;
}

//    Write Buffer
#define WRITE_BUFF_TEST (1)
bool NXP_MP2TSCaptureMgr::WriteBuff(U8 ch_num)
{
    struct ts_param_descr   param_descr;
    bool    ret = false;
    int     idx, count, alloc_size, read_size, read_count;
    void    *buf_addr[TS_PAGE_NUM];
#if (WRITE_BUFF_TEST == 1)
    FILE    *fp;
    int     wSize = 0;
#endif

    FUNC_IN;

//    if ((m_ch_info[ch_num].m_bPowerOn == false) || (m_ch_info[ch_num].m_bStarted == false))
    if (m_ch_info[ch_num].m_bPowerOn == false)
        goto exit_WriteBuff;

#if (WRITE_BUFF_TEST == 1)
    //fp = fopen( "/tmp/mp2ts_dump.ts", "rb" );
    fp = fopen( "/mnt/mmc0/mpegts/mp2ts_dump.ts", "rb" );
    if( fp == 0 )
    {
        ErrMsg("WriteBuff : file open failed\n");

        goto exit_WriteBuff;    // file open failed !!!
    }

    read_size   = (1 * 0x100000);
    alloc_size  = (TS_PAGE_SIZE * 1);
    read_count  = (read_size / alloc_size);
DbgMsg(DBG_DBG, "WriteBuff : write count = %d\n", read_count);

    for (count = 0; count < TS_PAGE_NUM; count++)
    {
        buf_addr[count] = malloc(alloc_size);
    }

    param_descr.info.un.bits.ch_num = ch_num;
    param_descr.info.un.bits.type   = NXP_MP2TS_PARAM_TYPE_BUF;
    param_descr.buf_size            = alloc_size;
    param_descr.wait_time           = 0;
//    param_descr.wait_time           = 2;    // Uint : 10ms => 20ms
    param_descr.read_count          = read_count;

    idx = 0;
    param_descr.buf = (void *)buf_addr[idx];
    for (count = 0; count < read_count; )
    {
#if 0
        read( fp, (unsigned char*)(buf_addr[idx]), alloc_size );
#else
        wSize = fread( (unsigned char*)(buf_addr[idx]), 1, alloc_size, fp);
        printf("wSize = %d  %d  %d %p : %x \n", wSize , alloc_size,idx, &buf_addr[idx],buf_addr[idx]);
        if( wSize != alloc_size )
        {
            printf("!!!! Error !!!!(wSize = %d, alloc_size = %d\n", 
                wSize, alloc_size);
        }
#endif

        if(ioctl(m_hDrv, IOCTL_MPEGTS_WRITE_BUF, &param_descr) < 0)
        {
            ErrMsg("IOCTL_MPEGTS_WRITE_BUF failed\n");
            //continue;
        }
        else
       	{
       		printf("TR OK\n");
       	}

        count++;
        printf("COUNT : %d \n",count);
        idx = (idx + 1) % TS_PAGE_NUM;
        param_descr.buf = (void *)buf_addr[idx];
    }


for (count = 0; count < 20; count++)
{
DbgMsg(DBG_DBG, "WriteBuff : file close\n");
}
#endif

    ret = true;

exit_WriteBuff:
    FUNC_OUT;

    if( fp )
    {
        fclose( fp );
    }

#if (WRITE_BUFF_TEST == 1)
    for (count = 0; count < TS_PAGE_NUM; count++)
    {
        free(buf_addr[count]);
    }
#endif
    return ret;
}


bool NXP_MP2TSCaptureMgr::CASDecryTest(U8 ch_num)
{
    struct ts_param_descr   param_descr;
    bool    ret = false;
    int     count, alloc_size, read_size, read_count;
    void    *stream_buf = NULL;
    void    *temp_buf   = NULL;
    FILE    *rFp, *wFp;
    int     wSize = 0;

    FUNC_IN;

//    if ((m_ch_info[ch_num].m_bPowerOn == false) || (m_ch_info[ch_num].m_bStarted == false))
    if (m_ch_info[ch_num].m_bPowerOn == false)
        goto exit_CASTest;

#if (CFG_AES_TEST == 1)
    rFp = fopen( "/tmp/aes_cbc_adapt_stream.ts", "rb" );
#endif
#if (CFG_CSA_TEST == 1)
    rFp = fopen( "/tmp/SA_dvbcsa_cas_stream.ts", "rb" );
#endif
    if( rFp == 0 )
    {
        ErrMsg("CASTest : Read file open failed\n");
        goto exit_CASTest;    // file open failed !!!
    }

#if (CFG_AES_TEST == 1)
    wFp = fopen( "/tmp/aes_cbc_adapt_clear.ts", "wb" );
#endif
#if (CFG_CSA_TEST == 1)
    wFp = fopen( "/tmp/SA_dvbcsa_cas_clear.ts", "wb" );
#endif
    if( wFp == 0 )
    {
        ErrMsg("CASTest : Write file open failed\n");
        goto exit_CASTest;    // file open failed !!!
    }

    read_size   = (50 * 0x100000);
//    read_size   = (10 * 0x100000);
    alloc_size  = (TS_PACKET_SIZE * TS_PACKET_NUM);
    read_count  = (read_size / alloc_size);
//read_count  = 20;

    stream_buf  = malloc(alloc_size);
    temp_buf    = malloc(alloc_size);

    param_descr.info.un.bits.ch_num = ch_num;
    param_descr.info.un.bits.type   = NXP_MP2TS_PARAM_TYPE_BUF;
    param_descr.buf_size            = alloc_size;
    param_descr.read_count          = read_count;
    param_descr.ret_value           = 0;

    param_descr.buf = (void *)temp_buf;
    for (count = 0; count < read_count; )
    {
        /* Read encry contents  */
        wSize = fread( (unsigned char *)(stream_buf), 1, alloc_size, rFp);
        if( wSize != alloc_size )
        {
            printf("!!!! Error !!!!(wSize = %d, alloc_size = %d\n", 
                wSize, alloc_size);
        }

retry_push:
        memcpy(temp_buf, stream_buf, alloc_size);

        /* Push encry contents,  Get decry data */
        if(ioctl(m_hDrv, IOCTL_MPEGTS_DECRY_TEST, &param_descr) < 0)
        {
            ErrMsg("IOCTL_MPEGTS_DECRY_TEST failed\n");
            goto retry_push;
        }

//printf("$$$ ret_value = 0x%08x\n", param_descr.ret_value);
        if( (param_descr.ret_value & (1<<ETS_READBUF)) == 0 )
        {
            /* Write to FILE */
            wSize = fwrite( (unsigned char *)(temp_buf), 1, alloc_size, wFp);
            if( wSize != alloc_size )
            {
                printf("!!!! Error !!!!(rSize = %d, alloc_size = %d\n", 
                    wSize, alloc_size);
            }
        }

        if( param_descr.ret_value & (1<<ETS_WRITEBUF) )
        {
//            ErrMsg("IOCTL_MPEGTS_DECRY_TEST : WriteBuf failed\n");
            goto retry_push;
        }

        count++;
    }

    ret = true;

exit_CASTest:
    FUNC_OUT;

    if( rFp )
        fclose( rFp );
    if( wFp )
        fclose( wFp );
    if( stream_buf )
        free(stream_buf);
    if( temp_buf )
        free(temp_buf);

    return ret;
}

bool NXP_MP2TSCaptureMgr::DetectPID(U8 ch_num)
{
    struct ts_param_descr   param_descr;
    struct TS_PACKET       *ts_packet;
    U32     temp, count;
    U32    *buf_addr;

    FUNC_IN;

    buf_addr = (U32 *)malloc(TS_PACKET_SIZE);

    param_descr.info.un.bits.ch_num = ch_num;
    param_descr.info.un.bits.type   = NXP_MP2TS_PARAM_TYPE_BUF;
    param_descr.buf                 = (void *)buf_addr;
    param_descr.buf_size            = TS_PACKET_SIZE;
    param_descr.read_count          = 1;

    for (count = 0; count < 1; )
    {
        if(ioctl(m_hDrv, IOCTL_MPEGTS_READ_BUF, &param_descr) < 0)
        {
            ErrMsg("IOCTL_MPEGTS_READ_BUF failed\n");
            continue;
        }

#if 0
        if (param_descr.ret_value < 0)
        {
            ErrMsg("Read Buff error\n");
            continue;
        }
#endif

        count++;
    }

    free(buf_addr);


    ts_packet = (struct TS_PACKET *)buf_addr;

    DbgMsg(DBG_DBG, "ts_packet size = %d\n", sizeof(struct TS_PACKET) );
    DbgMsg(DBG_DBG, "Sync byte  = %02x\n", ts_packet->sync_byte);
    DbgMsg(DBG_DBG, "PID        = %04x\n", (ts_packet->PID1 << 8) | ts_packet->PID0);

#if 0
    if (ts_packet->adaptation_field_control == 1)
        DbgMsg(DBG_DBG, "data_byte_0 = %d\n", ts_packet->data_byte);
    if (ts_packet->adaptation_field_control == 3)
    {
        temp = 184 - ts_packet->data_byte;
        DbgMsg(DBG_DBG, "data_byte_1 = %d\n", temp);
    }
#else

    if (ts_packet->adaptation_field_control & 1)
    {
        temp = 184 - ts_packet->data_byte;
        DbgMsg(DBG_DBG, "data_byte  = %d\n", temp);
    }
#endif

    temp = *(buf_addr + CURRENT_NEXT_INDICATOR_BYTE_OFFSET) & (CURRENT_NEXT_INDICATOR_BITS_MASK << CURRENT_NEXT_INDICATOR_BIT_OFFSET);
    DbgMsg(DBG_DBG, "CURRENT_NEXT_INDICATOR : %d\n", temp);

    temp = *(buf_addr + PAT_SECTION_COUNT_BYTE_OFFSET) & (PAT_SECTION_COUNT_BITS_MASK << PAT_SECTION_COUNT_BIT_OFFSET);
    DbgMsg(DBG_DBG, "PAT_SECTION_COUNT : %d\n", temp);

    for (count = 0; count < (TS_PACKET_SIZE >> 4); count++)
    {
        DbgMsg(DBG_DBG, "[%02x] : %08x, %08x, %08x, %08x\n",
            count, *(buf_addr + 0), *(buf_addr + 1), *(buf_addr + 2), *(buf_addr + 3) );
        buf_addr += 4;
    }

    if (TS_PACKET_SIZE % (1 << 4) )
    {
        DbgMsg(DBG_DBG, "[%02x] : %08x, %08x, %08x, %08x\n",
            count, *(buf_addr + 0), *(buf_addr + 1), *(buf_addr + 2), *(buf_addr + 3) );
    }

    FUNC_OUT;

    return true;
}


#include "StdAfx.h"
#include "ASVTest.h"
#include "Utils.h"

#define	RETRY_COUNT				1
#define	LVCC_RETRY_COUNT		3
#define	SCAN_RETRY_COUNT		3

#define	HARDWARE_BOOT_DELAY		15000
#define COMMAND_RESPONSE_DELAY	15000

#define	HARDWARE_RESET_TIME		1000

#define	NUM_VPU_FREQ_TABLE	3
#define	NUM_3D_FREQ_TABLE	3
const unsigned int gVpuFreqTable[NUM_VPU_FREQ_TABLE] =
{
	500000000,
	400000000,
	330000000,
};

const unsigned int g3DFreqTable[NUM_3D_FREQ_TABLE] =
{
	500000000,
	400000000,
	330000000,
};


static double MP8845_VTable[] = 
{
	0.6000,	0.6067,	0.6134,	0.6201,	0.6268,	0.6335,	0.6401,	0.6468,
	0.6535,	0.6602,	0.6669,	0.6736,	0.6803,	0.6870,	0.6937,	0.7004,
	0.7070,	0.7137,	0.7204,	0.7271,	0.7338,	0.7405,	0.7472,	0.7539,
	0.7606,	0.7673,	0.7739,	0.7806,	0.7873,	0.7940,	0.8007,	0.8074,
	0.8141,	0.8208,	0.8275,	0.8342,	0.8408,	0.8475,	0.8542,	0.8609,
	0.8676,	0.8743,	0.8810,	0.8877,	0.8944,	0.9011,	0.9077,	0.9144,
	0.9211,	0.9278,	0.9345,	0.9412,	0.9479,	0.9546,	0.9613,	0.9680,
	0.9746,	0.9813,	0.9880,	0.9947,	1.0014,	1.0081,	1.0148,	1.0215,
	1.0282,	1.0349,	1.0415,	1.0482,	1.0549,	1.0616,	1.0683,	1.0750,
	1.0817,	1.0884,	1.0951,	1.1018,	1.1084,	1.1151,	1.1218,	1.1285,
	1.1352,	1.1419,	1.1486,	1.1553,	1.1620,	1.1687,	1.1753,	1.1820,
	1.1887,	1.1954,	1.2021,	1.2088,	1.2155,	1.2222,	1.2289,	1.2356,
	1.2422,	1.2489,	1.2556,	1.2623,	1.2690,	1.2757,	1.2824,	1.2891,
	1.2958,	1.3025,	1.3091,	1.3158,	1.3225,	1.3292,	1.3359,	1.3426,
	1.3493,	1.3560,	1.3627,	1.3694,	1.3760,	1.3827,	1.3894,	1.3961,
	1.4028,	1.4095,	1.4162,	1.4229,	1.4296,	1.4363,	1.4429,	1.4496,
};

static const int MP8848_MAX_VTABLE = 128;


double FindNearlestVoltage( double inVoltage )
{
	if( inVoltage > MP8845_VTable[MP8848_MAX_VTABLE-1] )
	{
		return MP8845_VTable[MP8848_MAX_VTABLE-1];
	}
	if( inVoltage < MP8845_VTable[0] )
	{
		return MP8845_VTable[0];
	}

	for( int i=0 ; i<MP8848_MAX_VTABLE-1 ; i++ )
	{
		if( (MP8845_VTable[i] <= inVoltage) && (inVoltage <= MP8845_VTable[i+1]) )
		{
			if( (inVoltage-MP8845_VTable[i]) <= ( MP8845_VTable[i+1]-inVoltage) )
			{
				return MP8845_VTable[i];
			}
			else
			{
				return MP8845_VTable[i+1];
			}
		}
	}
	return -1.;
}

CASVTest::CASVTest()
	: m_pcbArg(NULL)
	, m_cbEventFunc(NULL)
	, m_cbMessageFunc(NULL)

	, m_bConfig(false)
	, m_pCom(NULL)

	, m_TestModuleId(ASV_MODULE_ID::ASVM_CPU)
	, m_Frequency( 400000000 )
	, m_MinVolt( 0.5 )
	, m_MaxVolt( 1.5 )
	, m_Timeout( 15 )	//	sec

	, m_bChipIdMode(false)

	, m_RxMsgLen(0)
{
	m_hRxSem = CreateSemaphore( NULL, 0, 4096, NULL );
	InitializeCriticalSection( &m_CritRxMsg );
}

CASVTest::~CASVTest()
{
	CloseHandle(m_hRxSem);
	DeleteCriticalSection( &m_CritRxMsg );
}

void CASVTest::RegisterCallback( void *pArg,
					void (*cbEvent)(void *pArg, ASVT_EVT_TYPE evtCode, void *evtData),
					void (*cbMsg)(void *pArg, char *str, int32_t len) )
{
	m_pcbArg = pArg;
	m_cbEventFunc = cbEvent;
	m_cbMessageFunc = cbMsg;
}

void CASVTest::SetTestConfig( ASV_TEST_CONFIG *config, CComPort *pCom )
{
	if( config )
		m_TestConfig = *config;
	if( pCom )
	{
		m_pCom = pCom;
		m_bConfig = true;
		if( m_pCom )
			m_pCom->SetRxCallback(this, CASVTest::RxComRxCallbackStub );
	}
}

bool CASVTest::Start( bool bChipIdMode )
{
	if( !m_bConfig )
	{
		return false;
	}
	m_bChipIdMode = bChipIdMode;
	m_bThreadExit = false;
	_beginthread( (void (*)(void *))FindLVCCThreadStub, 0, this);
	return true;
}

void CASVTest::Stop()
{
	m_bThreadExit = true;
	Sleep( 30 );
}

bool CASVTest::Scan(unsigned int ecid[4])
{
	if( !HardwareReset() )
	{
		if( m_cbEventFunc )
			m_cbEventFunc( m_pcbArg, ASVT_EVT_ERR, NULL );
		//MessageBox(NULL, TEXT("Scan : Hardware Reset Failed!!!"), TEXT("ERROR!!!"), MB_OK );
		DbgLogPrint(1, "Scan : Hardware Reset Failed!!!" );
		return false;
	}

	int tmu0;
	GetTMUInformation( &tmu0, 0 );

	char cmdBuf[MAX_CMD_STR];
	//	Set Target Voltage
	memset( cmdBuf, 0, sizeof(cmdBuf) );
	ASV_PARAM param = {0};
	MakeCommandString( cmdBuf, sizeof(cmdBuf), ASVC_GET_ECID, ASVM_CPU, param );

	//	Send Command
	if( m_pCom )
	{
		m_pCom->WriteData(cmdBuf, strlen(cmdBuf));
	}

	if( GetECIDCmdResponse(ecid) )
	{
		if( m_cbEventFunc )
		{
			m_cbEventFunc( m_pcbArg, ASVT_EVT_ECID, ecid );
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool CASVTest::TestLowestVolt( ASV_MODULE_ID module, unsigned int freq, float typical, float target )
{
	DbgLogPrint(1, "\n[%s] Start Voltage %f (lowest).\n", ASVModuleIDToString(module), target );
	//	Target 전압이 Typical 전압 보다 높으므로
	if( target > typical )
	{
		if( !SetVoltage(module, target) )
		{
			HardwareReset();
			return true;
		}
		else
		{
			if( m_bThreadExit )
				return false;
			if( !SetFrequency(module, freq) )
			{
				HardwareReset();
			}
			else
			{
				if( !StartTest( module, LVCC_RETRY_COUNT ) )
				{
					HardwareReset();
					return true;
				}
				else
				{
					DbgLogPrint(1, "[Warnning] Lowest Voltage Too High\n");
					return false;
				}
			}
		}
	}
	else
	{
		// Target 전압이 Typical전압보다 낮기 때문에 주파수를 먼저 바꾸고 전압을 바꾼다.
		if( !SetFrequency(module, freq) )
		{
			HardwareReset();
			return true;
		}
		else
		{
			if( m_bThreadExit )
				return false;
			if( !SetVoltage(module, target) )
			{
				HardwareReset();
				return true;
			}
			else
			{
				if( !StartTest( module, LVCC_RETRY_COUNT ) )
				{
					HardwareReset();
					return true;
				}
				else
				{
					DbgLogPrint(1, "[Warnning] Lowest Voltage Too High\n");
					return false;
				}
			}
		}
	}
	return false;
}


//
//	여기서는 System이 반드시 살아 있어야 한다.
//	만약 여기서 Test가 실패하면 Voltage Range 설정이 잘못된 것이다.
//
bool CASVTest::TestHighestVolt( ASV_MODULE_ID module, unsigned int freq, float typical, float target )
{
	DbgLogPrint(1, "\n[%s] Start Voltage %f (highest).\n", ASVModuleIDToString(module), target );
	//	Typical 이 Target보다 높은 경우 Frequency 먼저 바꾸고 전압을 바꾼다.
	if( typical > target )
	{
		if( !SetFrequency(module, freq) || !SetVoltage(module, target) || !StartTest( module, RETRY_COUNT ) )
		{
			DbgLogPrint(1, "[Warnning] Hightest Voltage Too Low\n");
			HardwareReset();
			return false;
		}
	}
	else
	{
		//	Typical 이 Target보다 낮은 경우 전압을 먼저 바꾸고 Frequency를 바꾼다.
		if( !SetVoltage(module, target) || !SetFrequency(module, freq) || !StartTest( module, RETRY_COUNT ) )
		{
			DbgLogPrint(1, "[Warnning] Hightest Voltage Too Low\n");
			HardwareReset();
			return false;
		}
	}
	return true;
}

bool CASVTest::TestTypicalVolt( ASV_MODULE_ID module, unsigned int freq, float typical )
{
	DbgLogPrint(1, "\n[%s] Start Voltage %f (typical).\n", ASVModuleIDToString(module), typical );
	if( module == ASVM_CPU && m_TestConfig.enableFixedCore )
	{
		DbgLogPrint(1, "Fixed Core Voltage Mode %f volt.\n", m_TestConfig.voltFixedCore );
		if( !SetVoltage(ASVM_VPU, m_TestConfig.voltFixedCore) )
		{
			HardwareReset();
			return false;
		}
	}
	if( module == ASVM_CPU && m_TestConfig.enableFixedSys )
	{
		DbgLogPrint(1, "Fixed Sys Voltage Mode %f volt.\n", m_TestConfig.voltFixedSys );
		if( !SetVoltage(ASVM_LDO_SYS, m_TestConfig.voltFixedSys) )
		{
			HardwareReset();
			return false;
		}
	}

	if( !SetFrequency(module, freq) || !SetVoltage(module, typical) || !StartTest( module, RETRY_COUNT ) )
	{
		DbgLogPrint(1, "[Fatal]  Typical Voltage Test Failed!!\n");
		HardwareReset();
		return false;
	}
	return true;
}


#if 0	// Version 1.0.3
float CASVTest::FastTestLoop( ASV_MODULE_ID module, unsigned int frequency )
{
	float lowVolt, highVolt, step, curVolt, bootup, lvcc = -1;
	int tryCount = 0;
	bool lastSuccess = false;
	float prevVolt;

	if( module == ASVM_CPU )
	{
		bootup  = m_TestConfig.armBootUp;
		lowVolt  = m_TestConfig.armVoltStart;
		highVolt = m_TestConfig.armVoltEnd;
		step     = m_TestConfig.armVoltStep;
	}
	else
	{
		bootup  = m_TestConfig.coreTypical;
		lowVolt  = m_TestConfig.coreVoltStart;
		highVolt = m_TestConfig.coreVoltEnd;
		step     = m_TestConfig.coreVoltStep;
	}

	//
	DbgLogPrint(1, "================ Start %s %dMHz LVCC =====================\n", ASVModuleIDToStringSimple(module), frequency/1000000);

	if( !TestLowestVolt( module, frequency, bootup, lowVolt ) )
		return lowVolt;

	if( m_bThreadExit )
		return lvcc;

	if( !TestHighestVolt( module, frequency, bootup, highVolt ) )
	{
#if 0

		//	전압이 너무 높을 경우 Typical을 최저 전압으로 설정해야 한다.
		if( !TestTypicalVolt( module, frequency, bootup ) )
		{
			return lvcc;
		}
		else
		{
			highVolt = lvcc = typical;
		}
#else
		if( module == ASVM_CPU )
		{
			float startVolt = m_TestConfig.armFaultStart;
			for( ; startVolt <= (m_TestConfig.armFaultEnd+step/2) ; )
			{
				if( m_bThreadExit)
				{
					return lvcc;
				}
				if( startVolt > bootup )
				{
					if(!SetFrequency(module, frequency) ||  !SetVoltage(module, startVolt) || !StartTest( module, RETRY_COUNT ) )
					{
						HardwareReset();
						lowVolt = startVolt;
					}
					else
					{
						highVolt = lvcc = startVolt;
						break;
					}
				}
				else
				{
					if(!SetVoltage(module, startVolt) || !SetFrequency(module, frequency) ||  !StartTest( module, RETRY_COUNT ) )
					{
						HardwareReset();
						lowVolt = startVolt;
					}
					else
					{
						highVolt = lvcc = startVolt;
						break;
					}
				}
				startVolt += step;
			}

			if( lvcc < 0 )
			{
				DbgLogPrint(1, "[%s] Highest Voltage Too Low.\n", ASVModuleIDToString(module));
				return lvcc;
			}
		}
		else
		{
			DbgLogPrint(1, "[%s] Highest Voltage Too Low.\n", ASVModuleIDToString(module));
			return lvcc;
		}
#endif
	}
	else
	{
		lvcc = highVolt;
	}

	lastSuccess = true;	//
	prevVolt = lvcc;

	while( !m_bThreadExit )
	{
		curVolt = SelectNextVoltage( lowVolt, highVolt, step );
		DbgLogPrint(1, "\n== > SelectNextVoltage( %f, %f ) = %f\n", lowVolt, highVolt, curVolt );
		if( curVolt < 0 )
			break;

		if( lastSuccess )
		{
			prevVolt = prevVolt;
		}
		else
		{
			prevVolt = bootup;
		}

		DbgLogPrint(1, "[%s] Start Voltage %f.\n", ASVModuleIDToString(module), curVolt );
		if( module == ASVM_CPU && m_TestConfig.enableFixedCore )
		{
			DbgLogPrint(1, "Fixed Core Voltage Mode %f volt.\n", m_TestConfig.voltFixedCore );
			if( !SetVoltage(ASVM_VPU, m_TestConfig.voltFixedCore) )
			{
				HardwareReset();
				return -1;
			}
		}
		if( module == ASVM_CPU && m_TestConfig.enableFixedSys )
		{
			DbgLogPrint(1, "Fixed Sys Voltage Mode %f volt.\n", m_TestConfig.voltFixedSys );
			if( !SetVoltage(ASVM_LDO_SYS, m_TestConfig.voltFixedSys) )
			{
				HardwareReset();
				return -1;
			}
		}

		DbgLogPrint(1, "curVolt = %f, prevVolt = %f, lastSuccess = %d\n", curVolt, prevVolt, lastSuccess );

		lastSuccess = false;
		//	Frequency 를 먼저 바꾸고 전압을 바꿔야 함.
		if( prevVolt > curVolt )
		{
			if(!SetFrequency(module, frequency) ||  !SetVoltage(module, curVolt) || !StartTest( module, RETRY_COUNT ) )
			{
				//	Hardware가 TimeOut이 난 상태로 보이므로 H/W Reset
				HardwareReset();

				//	전압 설정/ Frequency / Test
				//	실패하였으므로 최저 lvcc가 아닌 최전 전압은 마지막 실패 값으로 update해야함.
				lowVolt = curVolt;
			}
			else
			{
				//	테스트를 성공하였으므로 여기서는 낮은 쪽을 선택하게 된다.
				lvcc = curVolt;	//	성공하였기 때문에 lvcc를 먼저 update함
				highVolt = curVolt; // 성공하였기 때문에 최고 전압을 성공 전압으로 설정함.
				lastSuccess = true;
				prevVolt = curVolt;
				DbgLogPrint(1, "1. Success : prevVolt = %f\n", prevVolt );
#if 1	//	TODO: Temporal Test
				HardwareReset();
				prevVolt = bootup;
#endif
			}
		}
		//	전압을 먼저 바꾸고 Frequency를 바꿔야 함.
		else
		{
			//	전압 설정
			if( !SetVoltage(module, curVolt) || !SetFrequency(module, frequency) || !StartTest( module, RETRY_COUNT ) )
			{
				//	Hardware가 TimeOut이 난 상태로 보이므로 H/W Reset
				HardwareReset();

				//	전압 설정/ Frequency / Test
				//	실패하였으므로 최저 lvcc가 아닌 최전 전압은 마지막 실패 값으로 update해야함.
				lowVolt = curVolt;
			}
			else
			{
				//	테스트를 성공하였으므로 여기서는 낮은 쪽을 선택하게 된다.
				lvcc = curVolt;	//	성공하였기 때문에 lvcc를 먼저 update함
				highVolt = curVolt; // 성공하였기 때문에 최고 전압을 성공 전압으로 설정함.
				lastSuccess = true;
				prevVolt = curVolt;
				DbgLogPrint(1, "2. Success : prevVolt = %f\n", prevVolt );
#if 1	//	TODO: Temporal Test
				HardwareReset();
				prevVolt = bootup;
#endif
			}
		}
	}

	if( lvcc > 0 )
	{
		if( lastSuccess )
		{
			HardwareReset();
		}

		DbgLogPrint( 1, "\nFound LVCC = %f, Check LVCC Validity\n", lvcc);
		while( !m_bThreadExit )
		{
			if( module == ASVM_CPU && m_TestConfig.enableFixedCore )
			{
				DbgLogPrint(1, "Fixed Core Voltage Mode %f volt.\n", m_TestConfig.voltFixedCore );
				if( !SetVoltage(ASVM_VPU, m_TestConfig.voltFixedCore) )
				{
					HardwareReset();
					return -1;
				}
			}
			if( module == ASVM_CPU && m_TestConfig.enableFixedSys )
			{
				DbgLogPrint(1, "Fixed Sys Voltage Mode %f volt.\n", m_TestConfig.voltFixedSys );
				if( !SetVoltage(ASVM_LDO_SYS, m_TestConfig.voltFixedSys) )
				{
					HardwareReset();
					return -1;
				}
			}

			if( lvcc > bootup )
			{
				if( !SetVoltage( module, lvcc ) || !SetFrequency( module, frequency ) || !StartTest( module, LVCC_RETRY_COUNT ) )
				{
					DbgLogPrint( 1, "LVCC Validity Failed(lvcc=%fvolt), Try next voltage(%fvolt)\n", lvcc, lvcc+step);
					HardwareReset();
					if( module == ASVM_CPU && lvcc >= m_TestConfig.armVoltEnd )
					{
						DbgLogPrint( 1, "===> Validation Failed !!! Exit Loop!!\n");
						lvcc = -1;
						break;
					}
					if( ((module==ASVM_VPU)||(module==ASVM_3D)) && lvcc >= m_TestConfig.coreVoltEnd )
					{
						DbgLogPrint( 1, "===> Validation Failed !!! Exit Loop!!\n");
						lvcc = -1;
						break;
					}
					lvcc += step;
				}
				else
				{
					break;
				}
			}
			else
			{
				if( !SetFrequency( module, frequency ) || !SetVoltage( module, lvcc ) || !StartTest( module, LVCC_RETRY_COUNT ) )
				{
					DbgLogPrint( 1, "LVCC Validity Failed(lvcc=%fvolt), Try next voltage(%fvolt)\n", lvcc, lvcc+step);
					HardwareReset();
					if( module == ASVM_CPU && lvcc >= m_TestConfig.armVoltEnd )
					{
						DbgLogPrint( 1, "===> Validation Failed !!! Exit Loop!!\n");
						lvcc = -1;
						break;
					}
					if( ((module==ASVM_VPU)||(module==ASVM_3D)) && lvcc >= m_TestConfig.coreVoltEnd )
					{
						DbgLogPrint( 1, "===> Validation Failed !!! Exit Loop!!\n");
						lvcc = -1;
						break;
					}
					lvcc += step;
				}
				else
				{
					break;
				}
			}
		}
	}


	DbgLogPrint(1, "================ End %s %dMHz LVCC(%f) =====================\n", ASVModuleIDToStringSimple(module), frequency/1000000, lvcc);
	return lvcc;
}
#endif

#if 0	//	Berfore 1.0.0

float CASVTest::FastTestLoop( ASV_MODULE_ID module, unsigned int frequency )
{
	float lowVolt, highVolt, step, curVolt, lvcc = -1;
	int tryCount = 0;

	if( module == ASVM_CPU )
	{
		lowVolt  = m_TestConfig.armVoltStart;
		highVolt = m_TestConfig.armVoltEnd;
		step = m_TestConfig.armVoltStep;
	}
	else
	{
		lowVolt  = m_TestConfig.coreVoltStart;
		highVolt = m_TestConfig.coreVoltEnd;
		step = m_TestConfig.coreVoltStep;
	}

	//
	DbgLogPrint(1, "================ Start %s %dMHz LVCC =====================\n", ASVModuleIDToStringSimple(module), frequency/1000000);
	if( !HardwareReset() )
	{
		DbgLogPrint(1, "\n!!!!!!!!!!! Hardware Reset Failed !!!!!!!!!!!!!!\n");
	}

	//	여기서 System이 반드시 죽어야만 한다.
	//	만약 이번 Test가 Path가 된다면 Voltage Range가 잘못 설정된 것이다.
	if( module == ASVM_CPU && m_TestConfig.enableFixedCore )
	{
		DbgLogPrint(1, "Fixed Core Voltage Mode %f volt.\n", m_TestConfig.voltFixedCore );
		if( !SetVoltage(ASVM_VPU, m_TestConfig.voltFixedCore) )
		{
			HardwareReset();
			return lvcc;
		}
	}
	if( module == ASVM_CPU && m_TestConfig.enableFixedSys )
	{
		DbgLogPrint(1, "Fixed Sys Voltage Mode %f volt.\n", m_TestConfig.voltFixedSys );
		if( !SetVoltage(ASVM_LDO_SYS, m_TestConfig.voltFixedSys) )
		{
			HardwareReset();
			return lvcc;
		}
	}

	DbgLogPrint(1, "\n[%s] Start Voltage %f (lowest).\n", ASVModuleIDToString(module), lowVolt );
	if( !SetFrequency(module, frequency) )
	{
		HardwareReset();
	}
	else
	{
		if( m_bThreadExit )
			return lvcc;
		if(  !SetVoltage(module, lowVolt) )
		{
			HardwareReset();
		}
		else
		{
			if( !StartTest( module, RETRY_COUNT ) )
			{
				HardwareReset();
			}
			else
			{
				DbgLogPrint(1, "[Warnning] Lowest Voltage Too High\n");
				return lvcc;
			}
		}
	}

	if( m_bThreadExit )
		return lvcc;


	//
	//	여기서는 System이 반드시 살아 있어야 한다.
	//	만약 여기서 Test가 실패하면 Voltage Range 설정이 잘못된 것이다.
	//
	DbgLogPrint(1, "\n[%s] Start Voltage %f (highest).\n", ASVModuleIDToString(module), highVolt );
	if( module == ASVM_CPU && m_TestConfig.enableFixedCore )
	{
		DbgLogPrint(1, "Fixed Core Voltage Mode %f volt.\n", m_TestConfig.voltFixedCore );
		if( !SetVoltage(ASVM_VPU, m_TestConfig.voltFixedCore) )
		{
			HardwareReset();
			return lvcc;
		}
	}
	if( module == ASVM_CPU && m_TestConfig.enableFixedSys )
	{
		DbgLogPrint(1, "Fixed Sys Voltage Mode %f volt.\n", m_TestConfig.voltFixedSys );
		if( !SetVoltage(ASVM_LDO_SYS, m_TestConfig.voltFixedSys) )
		{
			HardwareReset();
			return lvcc;
		}
	}

	if( !SetFrequency(module, frequency) || !SetVoltage(module, highVolt) || !StartTest( module, RETRY_COUNT ) )
	{
		DbgLogPrint(1, "[Warnning] Hightest Voltage Too Low\n");
		HardwareReset();

		if( lowVolt < 1.1 && 1.1 < highVolt )
		{
			float typical = 1.1;
			DbgLogPrint(1, "==> Try Typical Voltage.(%f volt)\n", typical );
			if( module == ASVM_CPU && m_TestConfig.enableFixedCore )
			{
				DbgLogPrint(1, "Fixed Core Voltage Mode %f volt.\n", m_TestConfig.voltFixedCore );
				if( !SetVoltage(ASVM_VPU, m_TestConfig.voltFixedCore) )
				{
					HardwareReset();
					return lvcc;
				}
			}
			if( module == ASVM_CPU && m_TestConfig.enableFixedSys )
			{
				DbgLogPrint(1, "Fixed Sys Voltage Mode %f volt.\n", m_TestConfig.voltFixedSys );
				if( !SetVoltage(ASVM_LDO_SYS, m_TestConfig.voltFixedSys) )
				{
					HardwareReset();
					return lvcc;
				}
			}

			if( !SetFrequency(module, frequency) || !SetVoltage(module, typical) || !StartTest( module, RETRY_COUNT ) )
			{
				HardwareReset();
				return lvcc;
			}
			else
			{
				highVolt = typical;
			}
		}
		else
		{
			return lvcc;
		}
	}

	lvcc = highVolt;

	while( !m_bThreadExit )
	{
		curVolt = SelectNextVoltage( lowVolt, highVolt, step );
		DbgLogPrint(1, "\n== > SelectNextVoltage( %f, %f ) = %f\n", lowVolt, highVolt, curVolt );
		if( curVolt < 0 )
			break;

		DbgLogPrint(1, "[%s] Start Voltage %f.\n", ASVModuleIDToString(module), curVolt );
		if( module == ASVM_CPU && m_TestConfig.enableFixedCore )
		{
			DbgLogPrint(1, "Fixed Core Voltage Mode %f volt.\n", m_TestConfig.voltFixedCore );
			if( !SetVoltage(ASVM_VPU, m_TestConfig.voltFixedCore) )
			{
				HardwareReset();
				return lvcc;
			}
		}
		if( module == ASVM_CPU && m_TestConfig.enableFixedSys )
		{
			DbgLogPrint(1, "Fixed Sys Voltage Mode %f volt.\n", m_TestConfig.voltFixedSys );
			if( !SetVoltage(ASVM_LDO_SYS, m_TestConfig.voltFixedSys) )
			{
				HardwareReset();
				return lvcc;
			}
		}

		//	전압 설정
		if( !SetFrequency(module, frequency) || !SetVoltage(module, curVolt) || !StartTest( module, RETRY_COUNT ) )
		{
			//	Hardware가 TimeOut이 난 상태로 보이므로 H/W Reset
			HardwareReset();

			//	전압 설정/ Frequency / Test
			//	실패하였으므로 최저 lvcc가 아닌 최전 전압은 마지막 실패 값으로 update해야함.
			lowVolt = curVolt;
		}
		else
		{
			//	테스트를 성공하였으므로 여기서는 낮은 쪽을 선택하게 된다.
			lvcc = curVolt;	//	성공하였기 때문에 lvcc를 먼저 update함
			highVolt = curVolt; // 성공하였기 때문에 최고 전압을 성공 전압으로 설정함.
		}
	}

	if( lvcc > 0 )
	{
		DbgLogPrint( 1, "\nFound LVCC = %f, Check LVCC Validity\n", lvcc);
		while( !m_bThreadExit )
		{
			//	여기서 System이 반드시 죽어야만 한다.
			//	만약 이번 Test가 Path가 된다면 Voltage Range가 잘못 설정된 것이다.
			if( module == ASVM_CPU && m_TestConfig.enableFixedCore )
			{
				DbgLogPrint(1, "Fixed Core Voltage Mode %f volt.\n", m_TestConfig.voltFixedCore );
				if( !SetVoltage(ASVM_VPU, m_TestConfig.voltFixedCore) )
				{
					HardwareReset();
					return lvcc;
				}
			}
			if( module == ASVM_CPU && m_TestConfig.enableFixedSys )
			{
				DbgLogPrint(1, "Fixed Sys Voltage Mode %f volt.\n", m_TestConfig.voltFixedSys );
				if( !SetVoltage(ASVM_LDO_SYS, m_TestConfig.voltFixedSys) )
				{
					HardwareReset();
					return lvcc;
				}
			}
			if( !SetFrequency( module, frequency ) || !SetVoltage( module, lvcc ) || !StartTest( module, LVCC_RETRY_COUNT ) )
			{
				DbgLogPrint( 1, "LVCC Validity Failed(lvcc=%fvolt), Try next voltage(%fvolt)\n", lvcc, lvcc+step);
				HardwareReset();
				if( module == ASVM_CPU && lvcc >= m_TestConfig.armVoltEnd )
				{
					DbgLogPrint( 1, "===> Validation Failed !!! Exit Loop!!\n");
					lvcc = -1;
					break;
				}
				if( ((module==ASVM_VPU)||(module==ASVM_3D)) && lvcc >= m_TestConfig.coreVoltEnd )
				{
					DbgLogPrint( 1, "===> Validation Failed !!! Exit Loop!!\n");
					lvcc = -1;
					break;
				}
				lvcc += step;
			}
			else
			{
				break;
			}
		}
	}


	DbgLogPrint(1, "================ End %s %dMHz LVCC(%f) =====================\n", ASVModuleIDToStringSimple(module), frequency/1000000, lvcc);
	return lvcc;
}

#endif



#if 1	// Version 1.0.4 : 0.1 Volt씩 증가하면서 찾는 알고리즘
float CASVTest::FastTestLoop( ASV_MODULE_ID module, unsigned int frequency )
{
	double lowVolt, highVolt, curVolt, bootup, lvcc = -1, testVolt;
	int tryCount = 0;
	bool lastSuccess = false;
	float prevVolt;

	DbgLogPrint(1, "================ Start %s %dMHz LVCC =====================\n", ASVModuleIDToStringSimple(module), frequency/1000000);
	if( module == ASVM_CPU )
	{
		bootup  = m_TestConfig.armBootUp;
		lowVolt  = m_TestConfig.armVoltStart;
		highVolt = m_TestConfig.armVoltEnd;
	}
	else
	{
		bootup  = m_TestConfig.coreTypical;
		lowVolt  = m_TestConfig.coreVoltStart;
		highVolt = m_TestConfig.coreVoltEnd;
	}

	if( !TestLowestVolt( module, frequency, bootup, lowVolt ) )
		return lowVolt;

	DbgLogPrint(1, "================ Start 1st Loop =====================\n");
	//	Loop 1 : 0.1 Volt씩 전압을 조종하면서 1차 LVCC를 구한다.
	lowVolt  = FindNearlestVoltage(lowVolt);
	curVolt  = FindNearlestVoltage(lowVolt + 0.1);
	while( curVolt < highVolt)
	{
		if( m_bThreadExit )
			return lvcc;

		if(!SetFrequency(module, frequency) ||  !SetVoltage(module, curVolt) || !StartTest( module, RETRY_COUNT ) )
		{
			HardwareReset();
			lowVolt = curVolt;
		}
		else
		{
			HardwareReset();
			lvcc = highVolt = curVolt;
			break;
		}
		curVolt = FindNearlestVoltage(curVolt + 0.1);
	}

	if( lvcc < 0 )
	{
		return lvcc;
	}

	DbgLogPrint(1, "================ Start 2nd Loop(%f~%f) =====================\n", lowVolt, highVolt);
	//	Loop2 : 첫 번째 성공한 전압과 이전 실패한 전압사이에서 0.0066 Volt 씩 증가하며
	//			최초 성공 값을 구한다.
	curVolt = lowVolt;
	while( curVolt <= highVolt )
	{
		if( m_bThreadExit )
			return lvcc;

		if(!SetFrequency(module, frequency) ||  !SetVoltage(module, curVolt) || !StartTest( module, RETRY_COUNT ) )
		{
			HardwareReset();
			lowVolt = curVolt;
		}
		else
		{
			HardwareReset();
			lvcc = curVolt;
			break;
		}
		curVolt = FindNearlestVoltage(curVolt + 0.0066);
		if( curVolt >= highVolt )
		{
			break;
		}
	}

	lvcc = FindNearlestVoltage( lvcc - 0.0066 );
	if( lvcc > 0 )
	{
		DbgLogPrint( 1, "\nFound LVCC = %f, Check LVCC Validity\n", lvcc);
		while( !m_bThreadExit )
		{
			if( module == ASVM_CPU && m_TestConfig.enableFixedCore )
			{
				DbgLogPrint(1, "Fixed Core Voltage Mode %f volt.\n", m_TestConfig.voltFixedCore );
				if( !SetVoltage(ASVM_VPU, m_TestConfig.voltFixedCore) )
				{
					HardwareReset();
					return -1;
				}
			}
			if( module == ASVM_CPU && m_TestConfig.enableFixedSys )
			{
				DbgLogPrint(1, "Fixed Sys Voltage Mode %f volt.\n", m_TestConfig.voltFixedSys );
				if( !SetVoltage(ASVM_LDO_SYS, m_TestConfig.voltFixedSys) )
				{
					HardwareReset();
					return -1;
				}
			}

			if( !SetFrequency( module, frequency ) || !SetVoltage( module, lvcc ) || !StartTest( module, LVCC_RETRY_COUNT ) )
			{
				DbgLogPrint( 1, "LVCC Validity Failed(lvcc=%fvolt), Try next voltage(%fvolt)\n", lvcc, FindNearlestVoltage(lvcc+0.0066));
				HardwareReset();
				if( module == ASVM_CPU && lvcc >= m_TestConfig.armVoltEnd )
				{
					DbgLogPrint( 1, "===> Validation Failed !!! Exit Loop!!\n");
					lvcc = -1;
					break;
				}
				if( ((module==ASVM_VPU)||(module==ASVM_3D)) && lvcc >= m_TestConfig.coreVoltEnd )
				{
					DbgLogPrint( 1, "===> Validation Failed !!! Exit Loop!!\n");
					lvcc = -1;
					break;
				}
				lvcc = FindNearlestVoltage (lvcc + 0.0066);
			}
			else
			{
				break;
			}
		}
	}


	DbgLogPrint(1, "================ End %s %dMHz LVCC(%f) =====================\n", ASVModuleIDToStringSimple(module), frequency/1000000, lvcc);
	return lvcc;
}
#endif


void CASVTest::FindLVCCThread()
{
	unsigned int ECID[4];

	int scanCount = SCAN_RETRY_COUNT;
	int freqIndex = 0;
	float lvcc = -1;
	ASV_EVT_DATA evtData;
	DWORD startTick, endTick;

	//	CPU Test
	unsigned int frequency = m_TestConfig.freqEnd;

	do
	{
		if( Scan(ECID) )
		{
			break;
		}
		scanCount --;
	}while(scanCount > 0);
	if( scanCount == 0 )
	{
		DbgLogPrint(1, "==================== Scan Failed !!! =======================\n");
		if( m_cbEventFunc )
		{
			if( m_cbEventFunc )
				m_cbEventFunc( m_pcbArg, ASVT_EVT_ERR, NULL );
//			MessageBox(NULL, TEXT("Scan : Retry Failed!!!!"), TEXT("ERROR!!!"), MB_OK );
		}
		return ;
	}


	if( m_bChipIdMode )
		return;


	while( m_TestConfig.enableCpu && !m_bThreadExit && (frequency >= m_TestConfig.freqStart) )
	{
		startTick = GetTickCount();
		HardwareReset();
		lvcc = FastTestLoop( ASVM_CPU, frequency );

		//	Find TMU Data
		int TMU0=-1;
		GetTMUInformation( &TMU0, NULL );
		endTick = GetTickCount();
		if( m_cbEventFunc )
		{
			evtData.module = ASVM_CPU;
			evtData.frequency = frequency;
			evtData.lvcc = lvcc;
			evtData.time = endTick - startTick;
			evtData.TMU0 = TMU0;
			m_cbEventFunc( m_pcbArg, ASVT_EVT_REPORT_RESULT, &evtData );
		}
		frequency -= m_TestConfig.freqStep;
	}

#if 0
	//	VPU Test
	freqIndex = 0;
	while( m_TestConfig.enableVpu && !m_bThreadExit && freqIndex<NUM_VPU_FREQ_TABLE )
	{
		startTick = GetTickCount();
		lvcc = FastTestLoop( ASVM_VPU, gVpuFreqTable[freqIndex] );
		endTick = GetTickCount();
		if( m_cbEventFunc )
		{
			evtData.module = ASVM_VPU;
			evtData.frequency = gVpuFreqTable[freqIndex];
			evtData.lvcc = lvcc;
			evtData.time = endTick - startTick;
			m_cbEventFunc( m_pcbArg, ASVT_EVT_REPORT_RESULT, &evtData );
		}
		freqIndex ++;
	}

	//	3D Test
	freqIndex = 0;
	while( m_TestConfig.enable3D && !m_bThreadExit && freqIndex<NUM_3D_FREQ_TABLE )
	{
		startTick = GetTickCount();
		lvcc = FastTestLoop( ASVM_3D, g3DFreqTable[freqIndex] );
		endTick = GetTickCount();

		if( m_cbEventFunc )
		{
			evtData.module = ASVM_3D;
			evtData.frequency = g3DFreqTable[freqIndex];
			evtData.lvcc = lvcc;
			evtData.time = endTick - startTick;
			m_cbEventFunc( m_pcbArg, ASVT_EVT_REPORT_RESULT, &evtData );
		}
		freqIndex ++;
	}
#else
	//	VPU Test
	freqIndex = 0;
	while( m_TestConfig.enableVpu && !m_bThreadExit && m_TestConfig.numVpuFreq>freqIndex )
	{
		startTick = GetTickCount();
		HardwareReset();
		lvcc = FastTestLoop( ASVM_VPU, m_TestConfig.vpuFreqTable[freqIndex] );
		int TMU0=-1;
		GetTMUInformation( &TMU0, NULL );
		endTick = GetTickCount();
		if( m_cbEventFunc )
		{
			evtData.module = ASVM_VPU;
			evtData.frequency = m_TestConfig.vpuFreqTable[freqIndex];
			evtData.lvcc = lvcc;
			evtData.time = endTick - startTick;
			evtData.TMU0 = TMU0;
			m_cbEventFunc( m_pcbArg, ASVT_EVT_REPORT_RESULT, &evtData );
		}
		freqIndex ++;
	}

	//	3D Test
	freqIndex = 0;
	while( m_TestConfig.enable3D && !m_bThreadExit && m_TestConfig.numVrFreq>freqIndex )
	{
		startTick = GetTickCount();
		HardwareReset();
		lvcc = FastTestLoop( ASVM_3D, m_TestConfig.vrFreqTable[freqIndex] );
		int TMU0=-1;
		GetTMUInformation( &TMU0, NULL );
		endTick = GetTickCount();

		if( m_cbEventFunc )
		{
			evtData.module = ASVM_3D;
			evtData.frequency = m_TestConfig.vrFreqTable[freqIndex];
			evtData.lvcc = lvcc;
			evtData.time = endTick - startTick;
			evtData.TMU0 = TMU0;
			m_cbEventFunc( m_pcbArg, ASVT_EVT_REPORT_RESULT, &evtData );
		}
		freqIndex ++;
	}
#endif
	DbgLogPrint(1, "End Thread\n");

	if( m_cbEventFunc )
		m_cbEventFunc( m_pcbArg, ASVT_EVT_DONE, &evtData );

	_endthread();
}

bool FindLineFeed( char *str, int size, int *pos )
{
	int i=0;
	for( ; i<size ; i++ )
	{
		if( str[i] == '\n' )
		{
			*pos = i+1;
			return true;
		}
	}
	return false;
}

bool CASVTest::HardwareReset()
{
	DbgLogPrint(1, "\nReset Hardware!!!\n");
	//	Reset H/W
	m_pCom->ClearDTR();
	Sleep(HARDWARE_RESET_TIME);
	m_pCom->SetDTR();
	DbgLogPrint(1, "Wait Hardware Booting Message....\n");
	if( !WaitBootOnMessage() )
	{
		return false;
	}
	return true;
}


bool CASVTest::WaitBootOnMessage()
{
	DWORD waitResult;
	int pos;
	do{
		waitResult = WaitForSingleObject( m_hRxSem, HARDWARE_BOOT_DELAY );
		if( WAIT_OBJECT_0 == waitResult )
		{
			CNXAutoLock lock(&m_CritRxMsg);
			if( FindLineFeed( m_RxMessage, m_RxMsgLen, &pos ) )
			{
				if( !strncmp( m_RxMessage, "BOOT DONE", 9 ) )
				{
					m_RxMsgLen-=pos;
					memmove( m_RxMessage, m_RxMessage + pos, m_RxMsgLen );
					return true;
				}
				m_RxMsgLen-=pos;
				memmove( m_RxMessage, m_RxMessage + pos, m_RxMsgLen );
				continue;
			}
		}
		else if( WAIT_TIMEOUT == waitResult )
		{
			if( m_cbMessageFunc )
			{
				char s[128];
				sprintf( s, "WaitBootOnMessage : Timeout");
				m_cbMessageFunc( m_pcbArg, s, strlen(s));
				m_RxMessage[m_RxMsgLen] = '\n';
				m_RxMessage[m_RxMsgLen+1] = '\0';
				m_cbMessageFunc( m_pcbArg, m_RxMessage, 0 );
			}
			return false;
		}
	}while(1);
}


bool CASVTest::CheckCommandResponse()
{
	DWORD waitResult;
	int pos;
	do{
		waitResult = WaitForSingleObject( m_hRxSem, COMMAND_RESPONSE_DELAY );
		if( WAIT_OBJECT_0 == waitResult )
		{
			CNXAutoLock lock(&m_CritRxMsg);
			if( FindLineFeed( m_RxMessage, m_RxMsgLen, &pos ) )
			{
				if( !strncmp( m_RxMessage, "SUCCESS", 7 ) )
				{
					m_RxMsgLen-=pos;
					memmove( m_RxMessage, m_RxMessage + pos, m_RxMsgLen );
					return true;
				}
				else if( !strncmp( m_RxMessage, "FAIL", 4 ) )
				{
					m_RxMsgLen-=pos;
					memmove( m_RxMessage, m_RxMessage + pos, m_RxMsgLen );
					return false;
				}
				m_RxMsgLen-=pos;
				memmove( m_RxMessage, m_RxMessage + pos, m_RxMsgLen );
				continue;
			}
		}
		else if( WAIT_TIMEOUT == waitResult )
		{
			if( m_cbMessageFunc )
			{
				char s[128];
				sprintf( s, "CheckCommandResponse : Response Timeout");
				m_cbMessageFunc( m_pcbArg, s, strlen(s));
				m_RxMessage[m_RxMsgLen] = '\n';
				m_RxMessage[m_RxMsgLen+1] = '\0';
				m_cbMessageFunc( m_pcbArg, m_RxMessage, 0 );
			}
			return false;
		}
	}while(1);
}

bool CASVTest::GetECIDCmdResponse( unsigned int ecid[4] )
{
	DWORD waitResult;
	int pos;
	do{
		waitResult = WaitForSingleObject( m_hRxSem, COMMAND_RESPONSE_DELAY );
		if( WAIT_OBJECT_0 == waitResult )
		{
			CNXAutoLock lock(&m_CritRxMsg);
			if( FindLineFeed( m_RxMessage, m_RxMsgLen, &pos ) )
			{
				if( !strncmp( m_RxMessage, "SUCCESS", 7 ) )
				{
					CHIP_INFO chipInfo;
					ParseECID( ecid, &chipInfo );
					sscanf( m_RxMessage, "SUCCESS : ECID=%x-%x-%x-%x\n", &ecid[0], &ecid[1], &ecid[2], &ecid[3] );
					m_RxMsgLen-=pos;
					memmove( m_RxMessage, m_RxMessage + pos, m_RxMsgLen );
					return true;
				}
				else if( !strncmp( m_RxMessage, "FAIL", 4 ) )
				{
					m_RxMsgLen-=pos;
					memmove( m_RxMessage, m_RxMessage + pos, m_RxMsgLen );
					return false;
				}
				m_RxMsgLen-=pos;
				memmove( m_RxMessage, m_RxMessage + pos, m_RxMsgLen );
				continue;
			}
		}
		else if( WAIT_TIMEOUT == waitResult )
		{
			if( m_cbMessageFunc )
			{
				char s[128];
				sprintf( s, "GetECIDCmdResponse : Response Timeout");
				m_cbMessageFunc( m_pcbArg, s, strlen(s));
				m_RxMessage[m_RxMsgLen] = '\n';
				m_RxMessage[m_RxMsgLen+1] = '\0';
				m_cbMessageFunc( m_pcbArg, m_RxMessage, 0 );
			}
			return false;
		}
	}while(1);
	return true;
}

bool CASVTest::SetFrequency( ASV_MODULE_ID module, unsigned int frequency )
{
	char cmdBuf[MAX_CMD_STR];
	//	Set Target Voltage
	ASV_PARAM param;
	param.u32 = frequency;
	memset( cmdBuf, 0, sizeof(cmdBuf) );
	MakeCommandString( cmdBuf, sizeof(cmdBuf), ASVC_SET_FREQ, module, param );

	//	Send Command
	if( m_pCom )
	{
		DbgLogPrint(1, "Set Frequency (%dMHz)\n", frequency/1000000 );
		m_pCom->WriteData(cmdBuf, strlen(cmdBuf));
	}

	return CheckCommandResponse();
}

bool CASVTest::SetVoltage( ASV_MODULE_ID module, float voltage )
{
	char cmdBuf[MAX_CMD_STR];
	//	Set Target Voltage
	ASV_PARAM param;
	bool ret;
	param.f32 = voltage;
	memset( cmdBuf, 0, sizeof(cmdBuf) );
	MakeCommandString( cmdBuf, sizeof(cmdBuf), ASVC_SET_VOLT, module, param );

	//	Send Command
	if( m_pCom )
	{
		DbgLogPrint(1, "Set Voltage (%f volt)\n", voltage );
		m_pCom->WriteData(cmdBuf, strlen(cmdBuf));
	}
	ret = CheckCommandResponse();

	Sleep( 100 );
	return ret;
}

bool CASVTest::StartTest( ASV_MODULE_ID module, int retryCnt )
{
	int i=0;
	int max = retryCnt;
	do{
		DbgLogPrint(1, "Start %s Module Test( Try Count = %d/%d )\n", ASVModuleIDToStringSimple(module), ++i, max );
		char cmdBuf[MAX_CMD_STR];
		ASV_PARAM param;
		memset( cmdBuf, 0, sizeof(cmdBuf) );
		param.u64 = 0;
		MakeCommandString( cmdBuf, sizeof(cmdBuf), ASVC_RUN, module, param );
		//	Send Command
		if( m_pCom )
		{
			m_pCom->WriteData(cmdBuf, strlen(cmdBuf));
		}
		if( true != CheckCommandResponse() )
		{
			return false;
		}
		retryCnt --;
	}while( retryCnt > 0 && !m_bThreadExit );

	return true;
}


//
//	Low / High
//
float CASVTest::SelectNextVoltage( float low, float high, float step )
{
	int numStep;

	if( low == high || ((high-low)<(step*1.5)) )
		return -1;

	numStep = (int)((high - low)/step);
	if( numStep > 3 )
	{
		numStep /= 2;
		return (high - numStep*step);
	}
	return ((high - step) > low)?high-step : -1;
}

void CASVTest::RxComRxCallback( char *buf, int len )
{
	//	Send Event
	CNXAutoLock lock(&m_CritRxMsg);
	memcpy( m_RxMessage + m_RxMsgLen, buf, len );
	m_RxMsgLen += len;
	ReleaseSemaphore( m_hRxSem, 2, &m_nSem);
	DbgMsg( TEXT("m_nSem = %d\n"), m_nSem );
	if( m_cbMessageFunc )
	{
		if( len > 0 )
			m_cbMessageFunc( m_pcbArg, buf, len );
	}
}

void CASVTest::DbgLogPrint( int flag, char *fmt,... )
{
	va_list ap;
	va_start(ap,fmt);
	vsnprintf( m_DbgStr, 4095, fmt, ap );
	va_end(ap);
	if( m_cbMessageFunc )
	{
		m_cbMessageFunc( m_pcbArg, m_DbgStr, 0 );
	}
}

//	Fuse	MSB	Description
//	0~20	0	Lot ID
//	21~25	21	Wafer No
//	26~31	26	X_POS_H

//	0~1	32	X_POS_L
//	2~9	34	Y_POS
//	10~15	42	"000000" fix
//	16~23	48	CPU_IDS
//	24~31	56	CPU_RO

//	64~79	64	"0000000000000000" fix
//	80~95	80	"0000000000000000" fix

//	96~111	96	"0001011100100000" fix
//	112~127	112	"0010110001001000" fix

unsigned int ConvertMSBLSB( unsigned int data, int bits )
{
	unsigned int result = 0;
	unsigned int mask = 1;

	int i=0;
	for( i=0; i<bits ; i++ )
	{
		if( data&(1<<i) )
		{
			result |= mask<<(bits-i-1);
		}
	}
	return result;
}

void CASVTest::ParseECID( unsigned int ECID[4], CHIP_INFO *chipInfo)
{
	//	Read GUID
	chipInfo->lotID			= ConvertMSBLSB( ECID[0] & 0x1FFFFF, 21 );
	chipInfo->waferNo		= ConvertMSBLSB( (ECID[0]>>21) & 0x1F, 5 );
	chipInfo->xPos			= ConvertMSBLSB( ((ECID[0]>>26) & 0x3F) | ((ECID[1]&0x3)<<6), 8 );
	chipInfo->yPos			= ConvertMSBLSB( (ECID[1]>>2) & 0xFF, 8 );
	chipInfo->ids			= ConvertMSBLSB( (ECID[1]>>16) & 0xFF, 8 );
	chipInfo->ro				= ConvertMSBLSB( (ECID[1]>>24) & 0xFF, 8 );
	chipInfo->usbProductId	= ECID[3] & 0xFFFF;
	chipInfo->usbVendorId	= (ECID[3]>>16) & 0xFFFF;
}


//
//	TMU Data
//

bool CASVTest::GetTMUResponse( int *pTMU )
{
	DWORD waitResult;
	int pos;
	do{
		waitResult = WaitForSingleObject( m_hRxSem, COMMAND_RESPONSE_DELAY );
		if( WAIT_OBJECT_0 == waitResult )
		{
			CNXAutoLock lock(&m_CritRxMsg);
			if( FindLineFeed( m_RxMessage, m_RxMsgLen, &pos ) )
			{
				if( !strncmp( m_RxMessage, "SUCCESS", 7 ) )
				{
					sscanf( m_RxMessage, "SUCCESS : TMU=%d\n", pTMU );
					m_RxMsgLen-=pos;
					memmove( m_RxMessage, m_RxMessage + pos, m_RxMsgLen );
					return true;
				}
				else if( !strncmp( m_RxMessage, "FAIL", 4 ) )
				{
					m_RxMsgLen-=pos;
					memmove( m_RxMessage, m_RxMessage + pos, m_RxMsgLen );
					return false;
				}
				m_RxMsgLen-=pos;
				memmove( m_RxMessage, m_RxMessage + pos, m_RxMsgLen );
				continue;
			}
		}
		else if( WAIT_TIMEOUT == waitResult )
		{
			if( m_cbMessageFunc )
			{
				char s[128];
				sprintf( s, "GetTMUResponse : Response Timeout");
				m_cbMessageFunc( m_pcbArg, s, strlen(s));
				m_RxMessage[m_RxMsgLen] = '\n';
				m_RxMessage[m_RxMsgLen+1] = '\0';
				m_cbMessageFunc( m_pcbArg, m_RxMessage, 0 );
			}
			return false;
		}
	}while(1);
	return true;
}

bool CASVTest::GetTMUInformation( int *pTMU0, int *pTMU1 )
{
	char cmdBuf[MAX_CMD_STR];
	//	Set Target Voltage
	memset( cmdBuf, 0, sizeof(cmdBuf) );
	ASV_PARAM param = {0};

	if( pTMU0 )
	{
		//	Get TMU 0 Value
		MakeCommandString( cmdBuf, sizeof(cmdBuf), ASVC_GET_TMU0, ASVM_CPU, param );
		//	Send Command
		if( m_pCom )
		{
			m_pCom->WriteData(cmdBuf, strlen(cmdBuf));
		}
		if( !GetTMUResponse(pTMU0) )
		{
			return false;
		}
	}

	if( pTMU1 )
	{
		//	Get TMU 0 Value
		MakeCommandString( cmdBuf, sizeof(cmdBuf), ASVC_GET_TMU1, ASVM_CPU, param );
		//	Send Command
		if( m_pCom )
		{
			m_pCom->WriteData(cmdBuf, strlen(cmdBuf));
		}
		if( !GetTMUResponse(pTMU1) )
		{
			return false;
		}
	}
	return true;
}

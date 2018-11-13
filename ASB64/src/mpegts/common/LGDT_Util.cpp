/*****************************************************************************
 *
 *	Driver APIs for VSB/QAM Demodulator LGDT3305
 *
 *  Copyright (c) 2003-2008, System IC Div., LG Electronics, Inc.
 *  All Rights Reserved
 *
 *  File Name:      LGDT_Util.cpp
 *
 *  Description:    
 *
 *  Revision History:
 *
 *    Date		Author	Description
 *  -------------------------------------------------------------------------
 *   07-01-2006	pyjung	Original
 *   06-03-2007	jywon	Modify
 *
 ***************************************************************************/

//#include "LGDT3305.h"
#ifdef LINUX_PLATFORM
#include "linux_i2c.h"
#else
#include "stdafx.h"
#include "LGDT_I2C.h"
#include <windows.h>
#include "LG_IIC_Interface.h"			// Add USB-I2C interface : j.y. won 2007.06.03
#endif
#include "LGDT_Util.h"


// Add USB-I2C interface : j.y. won 2007.06.03
static	bool	bUSB_Open	= false;
static	LgdtInterface_t	typeOfInterface = LGDT_LPT;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//					  													   //
//			FOLLOWING FUNCTIONS ARE TO BE IMPLEMENTED BY CUSTOMER		   //
//																		   //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

/////////////////////////////////////////////////////////////////////////////
//
//  Name: Lgdt_I2C_Read
//
//  Description:    Reads registers from 'addrReg' address.
//
//  Parameters:     (DATA08)nAddrDev	- the address of the slave device.
//					(DATA16)nAddrReg	- the address of the register to read.
//					(DATA08)*pData		- data to be read.
//					(DATA08)nSizeData	- size of data to read.
//
//  Returns:        (int)0
//
//  Revision History:
//
//    Date		Author	Description
//  -------------------------------------------------------------------------
//   07-01-2006	pyjung	Original
//
/////////////////////////////////////////////////////////////////////////////
#ifdef LINUX_PLATFORM
int Lgdt_I2C_Read(DATA08 nAddr, DATA16 nReg, DATA08 *pData, DATA08 nCount)
{
    return i2c_a2b_read_byte(nAddr, nReg, pData, nCount);
}

#else
int Lgdt_I2C_Read(DATA08 nAddrDev, DATA16 nAddrReg, DATA08 *pData, DATA08 nSizeData)
{
	int			i=0;
	int			retval = 0;
//	DATA08		 nData;

	// Add USB-I2C interface : j.y. won 2007.06.03
	if(typeOfInterface == LGDT_LPT){
		/*	FOLLOWING ROUTINE IS AN EXAMPLE FOR CUSTOMER	*/
		// Use 8bits for Device ID and Use OR for READ flag .....
//		I2C_Read16(nAddrDev, nAddrReg, &nData, nSizeData);		//	#####
//		for(i=0 ; i < nSizeData; i++)
//		I2C_Read16(nAddrDev, nAddrReg+i, pData+i, 1);		//	#####
		I2C_Read16(nAddrDev, nAddrReg, pData, nSizeData);		//	#####
	}
	else if(typeOfInterface == LGDT_USB){
		// Use 7bits for Device ID.....
		retval = LG_IIC_ReadType2HS(nAddrDev>>1, (BYTE)(nAddrReg>>8), (BYTE)nAddrReg&0xFF, nSizeData, pData);
//		printf("\n USB Read return value = %d ", retval);
		if(retval > 0)
			return 0;
		else
			return 1;
	}
	else{
		for( i=0 ; i <nSizeData; i++)
			*(pData+i) = 0;
		return -1;
	}

//	*pData = nData;
	return 0;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
//  Name: Lgdt_I2C_Write
//
//  Description:    Sends data to a slave device.
//					The 1st data array is device address.
//					The 2nd data array is register address to write.
//					The data is located from the 3rd data array.
//
//  Parameters:     (DATA08)*pData	- data to write. This include addresses
//									 of the slave device and the register to write.
//					(DATA08)nSizeData- size of data.
//
//  Returns:        (int)0
//
//  Revision History:
//
//    Date		Author	Description
//  -------------------------------------------------------------------------
//   07-01-2006	pyjung	Original
//
/////////////////////////////////////////////////////////////////////////////
#ifdef LINUX_PLATFORM
int Lgdt_I2C_Write(DATA08 nAddr, DATA16 nReg, DATA08 *pData, DATA08 nCount)
{
    return i2c_a2b_write_byte(nAddr, nReg, pData, nCount);
}
#else

int Lgdt_I2C_Write(DATA08 *pData, DATA08 nSizeData)
{
	int		retval = 0;
	int		i=0;

	// Add USB-I2C interface : j.y. won 2007.06.03
	if(typeOfInterface == LGDT_LPT){
		/*	FOLLOWING ROUTINE IS AN EXAMPLE FOR CUSTOMER	*/
		// Use 8bits for Device ID and Use OR for READ flag .....
		I2C_Start();
		I2C_Sends(pData, nSizeData);
		I2C_Stop();
	}
	else if(typeOfInterface == LGDT_USB){
		// Use 7bits for Device ID.....
		retval = LG_IIC_WriteType2HS ((*pData)>>1, *(pData+1), *(pData+2), nSizeData-3, (pData+3));
//		for( i =0; i<nSizeData; i++)
//			printf(" [0x%x] ", *(pData+i));
		if(retval != FALSE)
			return 0;
		else
			return 1;
	}
	else
		return -1;

	return 0;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
//  Name: Lgdt_I2C_Read1B
//
//  Description:    Reads registers from 'addrReg' address.
//
//  Parameters:     (DATA08)nAddrDev	- the address of the slave device.
//					(DATA16)nAddrReg	- the address of the register to read.
//					(DATA08)*pData		- data to be read.
//					(DATA08)nSizeData	- size of data to read.
//
//  Returns:        (int)0
//
//  Revision History:
//
//    Date		Author	Description
//  -------------------------------------------------------------------------
//   07-01-2006	pyjung	Original
//
/////////////////////////////////////////////////////////////////////////////
#ifndef LINUX_PLATFORM
int Lgdt_I2C_Read1B(DATA08 nAddrDev, DATA08 nAddrReg, DATA08 *pData, DATA08 nSizeData)
{
	int			i=0;
	int			retval = 0;
//	DATA08		 nData;

	// Add USB-I2C interface : j.y. won 2007.06.03
	if(typeOfInterface == LGDT_LPT){
		/*	FOLLOWING ROUTINE IS AN EXAMPLE FOR CUSTOMER	*/
		// Use 8bits for Device ID and Use OR for READ flag .....
		I2C_Read8(nAddrDev, nAddrReg, pData, nSizeData);		//	#####
	}
	else if(typeOfInterface == LGDT_USB){
		// Use 7bits for Device ID.....
		retval = LG_IIC_ReadType1HS(nAddrDev>>1, (BYTE)nAddrReg&0xFF, nSizeData, pData);
//		printf("\n USB Read return value = %d ", retval);
		if(retval > 0 )
			return 0;
		else
			return 1;
	}
	else{
		for( i=0 ; i <nSizeData; i++)
			*(pData+i) = 0;
		return -1;
	}

//	*pData = nData;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//  Name: Lgdt_I2C_Write1B
//
//  Description:    Sends data to a slave device.
//					The 1st data array is device address.
//					The 2nd data array is register address to write.
//					The data is located from the 3rd data array.
//
//  Parameters:     (DATA08)*pData	- data to write. This include addresses
//									 of the slave device and the register to write.
//					(DATA08)nSizeData- size of data.
//
//  Returns:        (int)0
//
//  Revision History:
//
//    Date		Author	Description
//  -------------------------------------------------------------------------
//   07-01-2006	pyjung	Original
//
/////////////////////////////////////////////////////////////////////////////
int Lgdt_I2C_Write1B(DATA08 nAddrDev, DATA08 nAddrReg, DATA08 *pData, DATA08 nSizeData)
{
	int		retval = 0;
	int		i=0;

	// Add USB-I2C interface : j.y. won 2007.06.03
	if(typeOfInterface == LGDT_LPT){
		/*	FOLLOWING ROUTINE IS AN EXAMPLE FOR CUSTOMER	*/
		// Use 8bits for Device ID and Use OR for READ flag .....
		I2C_Start();
		I2C_Sends(&nAddrDev, 1);
		I2C_Sends(&nAddrReg, 1);
		I2C_Sends(pData, nSizeData);
		I2C_Stop();
	}
	else if(typeOfInterface == LGDT_USB){
		// Use 7bits for Device ID.....
		retval = LG_IIC_WriteType1HS (nAddrDev>>1, (BYTE)nAddrReg&0xFF, nSizeData, pData);
		if(retval != FALSE)
			return 0;
		else
			return 1;
	}
	else
		return -1;

	return 0;
}
#endif  //#ifndef LINUX_PLATFORM

/////////////////////////////////////////////////////////////////////////////
//
//  Name: Lgdt_I2C_Reads_multi
//
//  Description:    Reads registers.
//
//  Parameters:     (DATA08)nAddrDev	- the address of the slave device.
//					(DATA08)*pData		- data to be read.
//					(DATA08)nSizeData	- size of data to read.
//
//  Returns:        (int)0
//
//  Revision History:
//
//    Date		Author	Description
//  -------------------------------------------------------------------------
//   02-01-2008	jywon	Original
//
/////////////////////////////////////////////////////////////////////////////
#ifdef LINUX_PLATFORM
int Lgdt_I2C_Reads_multi(DATA08 *pData, DATA08 nSizeData)
{
    return i2c_a1b_read_multi(pData, nSizeData);
}
#else

int Lgdt_I2C_Reads_multi(DATA08 nAddrDev, DATA08 *pData, DATA08 nSizeData)
{
	int		retval = 0;
	int		i=0;

	nAddrDev &= 0xFE;
	// Add USB-I2C interface : j.y. won 2007.06.03
	if(typeOfInterface == LGDT_LPT){
		/*	FOLLOWING ROUTINE IS AN EXAMPLE FOR CUSTOMER	*/
		// Use 8bits for Device ID and Use OR for READ flag .....
		I2C_Get_Data(nAddrDev, pData, nSizeData);
	}
	else if(typeOfInterface == LGDT_USB){
		// Use 7bits for Device ID.....
		retval = LG_IIC_ReadType0LS (nAddrDev>>1, nSizeData, pData);
		if(retval != FALSE)
			return 0;
		else
			return 1;
	}
	else
		return -1;

	return 0;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
//  Name: Lgdt_I2C_Writes_multi
//
//  Description:    Write registers.
//
//  Parameters:     (DATA08)nAddrDev	- the address of the slave device.
//					(DATA08)*pData		- data to be read.
//					(DATA08)nSizeData	- size of data to read.
//
//  Returns:        (int)0
//
//  Revision History:
//
//    Date		Author	Description
//  -------------------------------------------------------------------------
//   02-01-2008	jywon	Original
//
/////////////////////////////////////////////////////////////////////////////
#ifdef LINUX_PLATFORM
int Lgdt_I2C_Writes_multi(DATA08 *pData, DATA08 nSizeData)
{
    return i2c_a1b_write_multi(pData, nSizeData);
}
#else

int Lgdt_I2C_Writes_multi(DATA08 nAddrDev, DATA08 *pData, DATA08 nSizeData)
{
	int		retval = 0;
	int		i=0;

	// Add USB-I2C interface : j.y. won 2007.06.03
	if(typeOfInterface == LGDT_LPT){
		/*	FOLLOWING ROUTINE IS AN EXAMPLE FOR CUSTOMER	*/
		// Use 8bits for Device ID and Use OR for READ flag .....
		I2C_Start();
		I2C_Sends(&nAddrDev, 1);
		I2C_Sends(pData, nSizeData);
		I2C_Stop();
	}
	else if(typeOfInterface == LGDT_USB){
		// Use 7bits for Device ID.....
		retval = LG_IIC_WriteType0LS (nAddrDev>>1, nSizeData, pData);
		if(retval != FALSE)
			return 0;
		else
			return 1;
	}
	else
		return -1;

	return 0;
}
#endif  // #ifdef LINUX_PLATFORM

/////////////////////////////////////////////////////////////////////////////
//
//  Name: LgdtDelay
//
//  Description:    delay execution for x milliseconds
//
//  Parameters:     (DATA16)nDelayTime - Delay time in milliseconds
//
//  Returns:        (int)0
//
//  Revision History:
//
//    Date		Author	Description
//  -------------------------------------------------------------------------
//   07-01-2006	pyjung	Original
//
/////////////////////////////////////////////////////////////////////////////
int LgdtDelay(DATA16 nDelayTime)
{
#ifdef LINUX_PLATFORM
    usleep(nDelayTime * 1000);
#else

	/*	FOLLOWING ROUTINE IS AN EXAMPLE FOR CUSTOMER	*/
	Sleep(nDelayTime);
#endif

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//  Name: LgdtOpenUSB
//
//  Description:    Open interface of USB-I2C
//
//  Parameters:     ()
//
//  Returns:        true or false
//
//  Revision History:
//
//    Date		Author	Description
//  -------------------------------------------------------------------------
//   06-03-2007	jywon	Original
//
/////////////////////////////////////////////////////////////////////////////
bool LgdtOpenUSB()
{
	bool retval = false;

#ifdef LINUX_PLATFORM
    bUSB_Open = false;
#else

	if( typeOfInterface == LGDT_USB)
	{
		if( LG_IIC_Open(NULL) == TRUE){
			bUSB_Open = true;
			retval = true;
		}
		else{
			bUSB_Open = false;
			retval = false;
		}
	}
	else
		bUSB_Open = false;
#endif

	return retval;
}

/////////////////////////////////////////////////////////////////////////////
//
//  Name: LgdtCloseUSB
//
//  Description:    Close interface of USB-I2C
//
//  Parameters:     ()
//
//  Returns:        true or false
//
//  Revision History:
//
//    Date		Author	Description
//  -------------------------------------------------------------------------
//   06-03-2007	jywon	Original
//
/////////////////////////////////////////////////////////////////////////////
bool LgdtCloseUSB()
{
	bool retval = false;

#ifdef LINUX_PLATFORM
    bUSB_Open = false;
#else

	if(LG_IIC_Close() == TRUE)
	{
		bUSB_Open = false;
//		typeOfInterface = LGDT_LPT;
		retval = true;
	}
	else
	{
//		bUSB_Open = true;
		retval = false;
	}
#endif

	return retval;
}

/////////////////////////////////////////////////////////////////////////////
//
//  Name: LgdtIsOpenUSB
//
//  Description:    If USB interface is open successfully, return true.
//					If not, return false
//
//  Parameters:     ()
//
//  Returns:        true or false
//
//  Revision History:
//
//    Date		Author	Description
//  -------------------------------------------------------------------------
//   06-03-2007	jywon	Original
//
/////////////////////////////////////////////////////////////////////////////

bool LgdtIsOpenUSB()
{
	return bUSB_Open;
}
/////////////////////////////////////////////////////////////////////////////
//
//  Name: LgdtGetInterface
//
//  Description:    Get the interface (LPT or USB)
//
//  Parameters:     LgdtInteface_t (*pInterface)
//
//  Returns:        true or false
//
//  Revision History:
//
//    Date		Author	Description
//  -------------------------------------------------------------------------
//   06-03-2007	jywon	Original
//
/////////////////////////////////////////////////////////////////////////////
bool LgdtGetInterface(LgdtInterface_t *pInterface)
{
	
	*pInterface = typeOfInterface;

	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//  Name: LgdtSetInterface
//
//  Description:    Set the interface (LPT or USB)
//
//  Parameters:     LgdtInteface_t (nInterface)
//
//  Returns:        true or false
//
//  Revision History:
//
//    Date		Author	Description
//  -------------------------------------------------------------------------
//   06-03-2007	jywon	Original
//
/////////////////////////////////////////////////////////////////////////////
bool LgdtSetInterface(LgdtInterface_t nInterface)
{
	typeOfInterface = nInterface;

	return true;
}



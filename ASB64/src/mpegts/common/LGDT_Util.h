/*
 * Copyright (c) 2003-2008, System IC Div., LG Electronics, Inc.
 * All Rights Reserved
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of LG Electronics, Inc.
 * and may contain proprietary, confidential and trade secret information of
 * LG Electronics, Inc. and/or its partners.
 *  
 * The contents of this file may not be disclosed to third parties or
 * copied or duplicated in any form, in whole or in part, without the prior
 * written permission of LG Electronics, Inc.
 *
 */

#ifndef __LGDT_UTIL_H__
#define __LGDT_UTIL_H__

#include "LGDT3305.h"

// Add USB-I2C interface : j.y. won 2007.06.03
typedef enum {
	LGDT_NO_INTERFACE = 0,
	LGDT_LPT		  = 1,
	LGDT_USB		  = 2,
	LGDT_ERROR_INTERFACE = 255,
} LgdtInterface_t;

// for "C" compiler
#if defined( __cplusplus )
extern "C"{
#endif


#ifdef LINUX_PLATFORM
int Lgdt_I2C_Read(DATA08 nAddr, DATA16 nReg, DATA08 *pData, DATA08 nCount);
int Lgdt_I2C_Write(DATA08 nAddr, DATA16 nReg, DATA08 *pData, DATA08 nCount);
int Lgdt_I2C_Reads_multi(DATA08 *pData, DATA08 nSizeData);
int Lgdt_I2C_Writes_multi(DATA08 *pData, DATA08 nSizeData);
#else

int Lgdt_I2C_Write(DATA08 *data, DATA08 size);
int Lgdt_I2C_Read(DATA08 nAddrDev, DATA16 nAddrReg, DATA08 *pData, DATA08 nSizeData);
int Lgdt_I2C_Write1B(DATA08 nAddrDev, DATA08 nAddrReg, DATA08 *pData, DATA08 nSizeData);
int Lgdt_I2C_Read1B(DATA08 nAddrDev, DATA08 nAddrReg, DATA08 *pData, DATA08 nSizeData);
int Lgdt_I2C_Reads_multi(DATA08 nAddrDev, DATA08 *pData, DATA08 nSizeData);
int Lgdt_I2C_Writes_multi(DATA08 nAddrDev, DATA08 *pData, DATA08 nSizeData);
#endif  // #ifndef LINUX_PLATFORM

int LgdtDelay(DATA16 delay);
#if defined( __cplusplus )
}
#endif

bool LgdtOpenUSB();
bool LgdtCloseUSB();
bool LgdtIsOpenUSB();
bool LgdtGetInterface(LgdtInterface_t *pInterface);
bool LgdtSetInterface(LgdtInterface_t nInterface);

#endif

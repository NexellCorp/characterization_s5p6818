/*
 * mtv_ioctl.h
 *
 * RAONTECH MTV IO control header file.
 *
 * Copyright (C) (2011, RAONTECH)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __MTV_IOCTL_H__
#define __MTV_IOCTL_H__

#include "common/sys_type.h"

#ifdef __cplusplus 
extern "C"{ 
#endif

#define MP2TS_DEV_NAME      "nxp-mpegtsi"
#define MP2TS_DEV_NAME_CAP0 "mp2ts-cap0"
#define MP2TS_DEV_NAME_CAP1 "mp2ts-cap1"
#define MP2TS_DEV_NAME_CORE "nxp-mpegtsi"
#define MP2TS_IOC_MAGIC     'N'


#define IOTCL_SCAN_NOT_DETECTED_RET		-55555


#if defined(RTV_MULTI_SERVICE_MODE)
#define MAX_NUM_MTV_MULTI_SVC_BUF	3

typedef struct
{
	unsigned int max_num_item;

#ifdef RTV_FIC_INTR_ENABLED /* FIC interrupt Mode. */
	unsigned int freq_khz; /* for FIC. */
	unsigned int fic_size[MAX_NUM_MTV_MULTI_SVC_BUF];
	unsigned char fic_buf[MAX_NUM_MTV_MULTI_SVC_BUF][384];
#endif

	unsigned int av_size[MAX_NUM_MTV_MULTI_SVC_BUF];
	unsigned char av_ts[MAX_NUM_MTV_MULTI_SVC_BUF][16*188];

	unsigned int data_size[MAX_NUM_MTV_MULTI_SVC_BUF];
	unsigned char data_ts[MAX_NUM_MTV_MULTI_SVC_BUF][3*1024];
} IOCTL_MULTI_SERVICE_BUF;
#endif

/*============================================================================
 * MT2TS IO control commands(0~10)
 *==========================================================================*/
#define	IOC_NX_MAGIC		0x6e78	/* "nx" */

#define IOCTL_MPEGTS_RUN                _IO(IOC_NX_MAGIC, 1)
#define IOCTL_MPEGTS_STOP               _IO(IOC_NX_MAGIC, 2)
#define IOCTL_MPEGTS_READ_BUF           _IO(IOC_NX_MAGIC, 3)
#define IOCTL_MPEGTS_WRITE_BUF          _IO(IOC_NX_MAGIC, 4)
#define IOCTL_MPEGTS_READ_BUF_STATUS    _IO(IOC_NX_MAGIC, 5)
#define IOCTL_MPEGTS_POWER_ON           _IO(IOC_NX_MAGIC, 6)
#define IOCTL_MPEGTS_POWER_OFF          _IO(IOC_NX_MAGIC, 7)
#define IOCTL_MPEGTS_DO_ALLOC           _IO(IOC_NX_MAGIC, 8)
#define IOCTL_MPEGTS_DO_DEALLOC         _IO(IOC_NX_MAGIC, 9)
#define IOCTL_MPEGTS_SET_CONFIG         _IO(IOC_NX_MAGIC, 10)
#define IOCTL_MPEGTS_GET_CONFIG         _IO(IOC_NX_MAGIC, 11)
#define IOCTL_MPEGTS_SET_PARAM          _IO(IOC_NX_MAGIC, 12)
#define IOCTL_MPEGTS_GET_PARAM          _IO(IOC_NX_MAGIC, 13)
#define IOCTL_MPEGTS_CLR_PARAM          _IO(IOC_NX_MAGIC, 14)
#define IOCTL_MPEGTS_GET_LOCK_STATUS    _IO(IOC_NX_MAGIC, 15)
#define IOCTL_MPEGTS_DECRY_TEST         _IO(IOC_NX_MAGIC, 16)
#define IOCTL_MPEGTS_ENCRY_TEST         _IO(IOC_NX_MAGIC, 17)


/*============================================================================
 * Test IO control commands(0~10)
 *==========================================================================*/
#define IOCTL_TEST_MTV_POWER_ON	_IO(MP2TS_IOC_MAGIC, 0)
#define IOCTL_TEST_MTV_POWER_OFF	_IO(MP2TS_IOC_MAGIC, 1)

#define MAX_NUM_MTV_REG_READ_BUF	40
typedef struct
{
	unsigned int page; /* index */
	unsigned int addr; /* input */

	unsigned int write_data;
	
	unsigned int read_cnt;
	unsigned char read_data[MAX_NUM_MTV_REG_READ_BUF]; /* output */
} IOCTL_REG_ACCESS_INFO;
#define IOCTL_TEST_REG_SINGLE_READ	_IOWR(MP2TS_IOC_MAGIC, 3, IOCTL_REG_ACCESS_INFO)
#define IOCTL_TEST_REG_BURST_READ	_IOWR(MP2TS_IOC_MAGIC, 4, IOCTL_REG_ACCESS_INFO)
#define IOCTL_TEST_REG_WRITE		_IOW(MP2TS_IOC_MAGIC, 5, IOCTL_REG_ACCESS_INFO)

typedef struct
{
	unsigned int pin; /* input */
	unsigned int value; /* input */
} IOCTL_GPIO_ACCESS_INFO;
#define IOCTL_TEST_GPIO_SET	_IOW(MP2TS_IOC_MAGIC, 5, IOCTL_GPIO_ACCESS_INFO)
#define IOCTL_TEST_GPIO_GET	_IOWR(MP2TS_IOC_MAGIC, 6, IOCTL_GPIO_ACCESS_INFO)


/*==============================================================================
 * ISDB-T IO control commands(10~29)
 *============================================================================*/
typedef struct
{
	unsigned int	ber; // output
	unsigned int	cnr;  // output
	unsigned int	per;  // output
	int 		rssi;  // output
} IOCTL_ISDBT_SIGNAL_INFO;

#define IOCTL_ISDBT_POWER_ON		_IOW(MP2TS_IOC_MAGIC,10, E_RTV_COUNTRY_BAND_TYPE)
#define IOCTL_ISDBT_POWER_OFF		_IO(MP2TS_IOC_MAGIC, 11)
#define IOCTL_ISDBT_SCAN_FREQ		_IOW(MP2TS_IOC_MAGIC,12, unsigned int)
#define IOCTL_ISDBT_SET_FREQ		_IOW(MP2TS_IOC_MAGIC,13, unsigned int)
#define IOCTL_ISDBT_GET_LOCK_STATUS    _IOR(MP2TS_IOC_MAGIC,14, unsigned int)
#define IOCTL_ISDBT_GET_TMCC		_IOR(MP2TS_IOC_MAGIC,15, RTV_ISDBT_TMCC_INFO)
#define IOCTL_ISDBT_GET_SIGNAL_INFO	_IOR(MP2TS_IOC_MAGIC,16, IOCTL_ISDBT_SIGNAL_INFO)
#define IOCTL_ISDBT_START_TS		_IO(MP2TS_IOC_MAGIC, 17)
#define IOCTL_ISDBT_STOP_TS		_IO(MP2TS_IOC_MAGIC, 18)

/*============================================================================
* TDMB IO control commands(30 ~ 49)
*===========================================================================*/
#if defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)
typedef struct
{
	unsigned int ch_freq_khz; // input
	unsigned int subch_id;  // input
	E_RTV_SERVICE_TYPE svc_type;   // input
} IOCTL_TDMB_SUB_CH_INFO;

typedef struct
{
	unsigned int 	lock_mask;
	unsigned int	ant_level;
	unsigned int 	ber; // output
	unsigned int 	cer; // output
	unsigned int 	cnr;  // output
	unsigned int 	per;  // output
	int 		rssi;  // output
} IOCTL_TDMB_SIGNAL_INFO;

#define IOCTL_TDMB_POWER_ON			_IOW(MP2TS_IOC_MAGIC, 30, E_RTV_COUNTRY_BAND_TYPE)
#define IOCTL_TDMB_POWER_OFF			_IO(MP2TS_IOC_MAGIC, 31)
#define IOCTL_TDMB_SCAN_FREQ			_IOW(MP2TS_IOC_MAGIC,32, unsigned int)
#define IOCTL_TDMB_SCAN_STOP			_IO(MP2TS_IOC_MAGIC,33)
#define IOCTL_TDMB_READ_FIC			_IOR(MP2TS_IOC_MAGIC,34, unsigned char)
#define IOCTL_TDMB_OPEN_SUBCHANNEL		_IOW(MP2TS_IOC_MAGIC,35, IOCTL_TDMB_SUB_CH_INFO)
#define IOCTL_TDMB_CLOSE_SUBCHANNEL		_IOW(MP2TS_IOC_MAGIC,36, unsigned int)
#define IOCTL_TDMB_CLOSE_ALL_SUBCHANNELS	_IO(MP2TS_IOC_MAGIC,37)
#define IOCTL_TDMB_GET_LOCK_STATUS		_IOR(MP2TS_IOC_MAGIC,38, unsigned int)
#define IOCTL_TDMB_GET_SIGNAL_INFO		_IOR(MP2TS_IOC_MAGIC,39, IOCTL_TDMB_SIGNAL_INFO)
#endif

/*==============================================================================
 * FM IO control commands(50 ~ 69)
 *============================================================================*/
#define MAX_NUM_FM_EXIST_CHANNEL 256

typedef struct
{
	unsigned int start_freq; // input
	unsigned int end_freq;   // input
	unsigned int num_ch_buf;  // input
	unsigned int ch_buf[MAX_NUM_FM_EXIST_CHANNEL]; // output
	int num_deteced_ch; // output
} IOCTL_FM_SCAN_INFO;


typedef struct
{
	unsigned int start_freq; // input
	unsigned int end_freq;   // input
	unsigned int detected_freq_khz; /* output */
} IOCTL_FM_SRCH_INFO;


typedef struct
{
	unsigned int val; // output
	unsigned int cnt;   // output
} IOCTL_FM_LOCK_STATUS_INFO;


#define IOCTL_FM_POWER_ON		_IOW(MP2TS_IOC_MAGIC,50, E_RTV_ADC_CLK_FREQ_TYPE)
#define IOCTL_FM_POWER_OFF		_IO(MP2TS_IOC_MAGIC, 51)
#define IOCTL_FM_SET_FREQ		_IOW(MP2TS_IOC_MAGIC,52, unsigned int)
#define IOCTL_FM_SCAN_FREQ		_IOWR(MP2TS_IOC_MAGIC,53, IOCTL_FM_SCAN_INFO)
#define IOCTL_FM_SRCH_FREQ		_IOW(MP2TS_IOC_MAGIC,54, IOCTL_FM_SRCH_INFO)
#define IOCTL_FM_START_TS		_IO(MP2TS_IOC_MAGIC, 55)
#define IOCTL_FM_STOP_TS		_IO(MP2TS_IOC_MAGIC, 56)
#define IOCTL_FM_GET_LOCK_STATUS	_IOR(MP2TS_IOC_MAGIC,57, IOCTL_FM_LOCK_STATUS_INFO)
#define IOCTL_FM_GET_RSSI		_IOR(MP2TS_IOC_MAGIC,58, int)


/*==============================================================================
 * DAB IO control commands(70 ~ 89)
 *============================================================================*/
typedef enum
{
	DAB_SERVICE_VIDEO = 0x01,
	DAB_SERVICE_AUDIO = 0x02,
	DAB_SERVICE_DATA = 0x04
} DAB_SERVICE_TYPE;

typedef struct
{
	unsigned int ch_freq_khz; // input
	unsigned int subch_id;  // input
	DAB_SERVICE_TYPE svc_type;   // input
} IOCTL_DAB_SUB_CH_INFO;

typedef struct
{
	unsigned int	lock_mask;
	unsigned int	ant_level;
	unsigned int	ber;	// output
	unsigned int	cer;	// output
	unsigned int	cnr;	// output
	unsigned int	per;	// output
	int				rssi;	// output
} IOCTL_DAB_SIGNAL_INFO;


#define IOCTL_DAB_POWER_ON			_IO(MP2TS_IOC_MAGIC, 70)
#define IOCTL_DAB_POWER_OFF			_IO(MP2TS_IOC_MAGIC, 71)
#define IOCTL_DAB_SCAN_FREQ			_IOW(MP2TS_IOC_MAGIC,72, unsigned int)
#define IOCTL_DAB_SCAN_STOP			_IO(MP2TS_IOC_MAGIC,73)
#define IOCTL_DAB_READ_FIC			_IOR(MP2TS_IOC_MAGIC,74, unsigned char)
#define IOCTL_DAB_OPEN_SUBCHANNEL		_IOW(MP2TS_IOC_MAGIC,75, IOCTL_DAB_SUB_CH_INFO)
#define IOCTL_DAB_CLOSE_SUBCHANNEL		_IOW(MP2TS_IOC_MAGIC,76, unsigned int)
#define IOCTL_DAB_CLOSE_ALL_SUBCHANNELS	_IO(MP2TS_IOC_MAGIC,77)
#define IOCTL_DAB_GET_LOCK_STATUS		_IOR(MP2TS_IOC_MAGIC,78, unsigned int)
#define IOCTL_DAB_GET_SIGNAL_INFO		_IOR(MP2TS_IOC_MAGIC,79, IOCTL_DAB_SIGNAL_INFO)
#define IOCTL_DAB_OPEN_FIC			_IO(MP2TS_IOC_MAGIC, 80)
#define IOCTL_DAB_CLOSE_FIC			_IO(MP2TS_IOC_MAGIC, 81)


#ifdef __cplusplus 
} 
#endif 

#endif /* __MTV_IOCTL_H__*/


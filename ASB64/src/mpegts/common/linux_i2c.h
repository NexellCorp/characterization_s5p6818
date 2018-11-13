#ifndef __LINUX_I2C_H__
#define __LINUX_I2C_H__

#include <stdio.h>
#include <sys/ioctl.h>      //  _IOW
#include <fcntl.h>          //  open
#include <unistd.h>         //  close
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "../LGDT3306API/LG3306_Common.h"


#ifdef __cplusplus
extern "C" {
#endif

int     i2c_open(UINT08 addr);
int     i2c_a1b_bulk_read(UINT08 addr, UINT08 reg, UINT08 *dat, UINT08 num);
int     i2c_a1b_bulk_write(UINT08 addr, UINT08 reg, UINT08 *dat, UINT08 num);
int     i2c_a1b_read_multi(UINT08 *dat, UINT08 num);
int     i2c_a1b_write_multi(UINT08 *dat, UINT08 num);
int     i2c_a2b_bulk_read(UINT08 addr, UINT16 reg, UINT08 *dat, UINT08 num);
int     i2c_a2b_bulk_write(UINT08 addr, UINT16 reg, UINT08 *dat, UINT08 num);
void    i2c_close(void);


#ifdef __cplusplus
}
#endif

#endif  // #ifndef __LINUX_I2C_H__

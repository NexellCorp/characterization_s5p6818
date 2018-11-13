#include "linux_i2c.h"

#define I2C_CH_NUM          (1)
#define I2C_DEV_REG_WIDTH   (2) // unit of byte
#define I2C_BULK_WRITE_MAX  (16)

static int  i2c_file = -1;

int i2c_a1b_bulk_read(UINT08 addr, UINT08 reg, UINT08 *dat, UINT08 num)
{
    struct i2c_rdwr_ioctl_data  rw_arg;
    struct i2c_msg              ctl_msgs[2];

    if (i2c_file < 0)
        return (-1);

    rw_arg.msgs     = ctl_msgs;
    rw_arg.nmsgs    = 2;

    rw_arg.msgs[0].addr     = (__u16)(addr>>1);
    rw_arg.msgs[0].flags    = (__u16)0;                 /* !I2C_M_RD = write */
//    rw_arg.msgs[0].flags    = (__u16)I2C_M_RD;          /* I2C_M_RD = read */
//    rw_arg.msgs[0].flags    = (__u16)I2C_M_NOSTART;     /* write and no stop */
    rw_arg.msgs[0].len      = (__u16)1;                 /* data size = reg */
    rw_arg.msgs[0].buf      = (__u8 *)&reg;

    rw_arg.msgs[1].addr     = (__u16)(addr>>1);
    rw_arg.msgs[1].flags    = (__u16)I2C_M_RD;          /* I2C_M_RD = read */
    rw_arg.msgs[1].len      = (__u16)num;               /* data size */
    rw_arg.msgs[1].buf      = (__u8 *)dat;

    if (ioctl(i2c_file, I2C_RDWR, &rw_arg) < 0)
    {
        printf("IoCtl : error - %s \n", __func__);
        return (-1);
    }

#if 0
    for (idx = 0; idx < num; idx++)
    {
    	*dat++ = buf[idx];
    }
#endif

	return 0;
}

int i2c_a1b_bulk_write(UINT08 addr, UINT08 reg, UINT08 *dat, UINT08 num)
{
    struct i2c_rdwr_ioctl_data  rw_arg;
    struct i2c_msg              ctl_msgs[2];

    if (i2c_file < 0)
        return (-1);

    rw_arg.msgs     = ctl_msgs;
    rw_arg.nmsgs    = 2;

    rw_arg.msgs[0].addr     = (__u16)(addr>>1);
    rw_arg.msgs[0].flags    = (__u16)0;                 /* !I2C_M_RD = write */
//    rw_arg.msgs[0].flags    = (__u16)I2C_M_NOSTART;     /* write and no stop */
    rw_arg.msgs[0].len      = (__u16)1;                 /* data size = reg */
    rw_arg.msgs[0].buf      = (__u8 *)&reg;

    rw_arg.msgs[1].addr     = (__u16)(addr>>1);
    rw_arg.msgs[1].flags    = (__u16)0;                 /* !I2C_M_RD = write */
    rw_arg.msgs[1].len      = (__u16)num;               /* data size */
    rw_arg.msgs[1].buf      = (__u8 *)dat;

    if (ioctl(i2c_file, I2C_RDWR, &rw_arg) < 0)
    {
        printf("IoCtl : error - %s \n", __func__);
        return (-1);
    }

	return 0;
}

// Repeat start read
int i2c_a1b_read_multi(UINT08 *dat, UINT08 num)
{
    struct i2c_rdwr_ioctl_data  rw_arg;
    struct i2c_msg              ctl_msgs[1];

    if (i2c_file < 0)
        return (-1);

    rw_arg.msgs     = ctl_msgs;
    rw_arg.nmsgs    = 1;

    rw_arg.msgs[0].addr     = (__u16)(*dat>>1); /* Slave Addr */
    rw_arg.msgs[0].flags    = (__u16)I2C_M_RD;  /* I2C_M_RD = read */
    rw_arg.msgs[0].len      = (__u16)num;       /* data size = read for register count */
    rw_arg.msgs[0].buf      = (__u8 *)dat;

    if (ioctl(i2c_file, I2C_RDWR, &rw_arg) < 0)
    {
        printf("IoCtl : error - %s \n", __func__);
        return (-1);
    }

	return 0;
}

// Repeat start write
int i2c_a1b_write_multi(UINT08 *dat, UINT08 num)
{
    struct i2c_rdwr_ioctl_data  rw_arg;
    struct i2c_msg              ctl_msgs[1];

    if (i2c_file < 0)
        return (-1);

    rw_arg.msgs     = ctl_msgs;
    rw_arg.nmsgs    = 1;

    rw_arg.msgs[0].addr     = (__u16)(*dat>>1); /* Slave Addr */
    rw_arg.msgs[0].flags    = (__u16)0;         /* !I2C_M_RD = write */
    rw_arg.msgs[0].len      = (__u16)(num-1);   /* data size = { REG, DAT(0), ..., DAT(num-1) } */
    rw_arg.msgs[0].buf      = (__u8 *)dat+1;

    if (ioctl(i2c_file, I2C_RDWR, &rw_arg) < 0)
    {
        printf("IoCtl : error - %s \n", __func__);
        return (-1);
    }

	return 0;
}

int i2c_a2b_bulk_read(UINT08 addr, UINT16 reg, UINT08 *dat, UINT08 num)
{
    struct i2c_rdwr_ioctl_data  rw_arg;
    struct i2c_msg              ctl_msgs[2];
    UINT08                      buf[2];

    if (i2c_file < 0)
        return (-1);

    rw_arg.msgs     = ctl_msgs;
    rw_arg.nmsgs    = 2;

    buf[0] = (reg >> 8) & 0xFF;
    buf[1] = (reg >> 0) & 0xFF;

    rw_arg.msgs[0].addr     = (__u16)(addr>>1);
    rw_arg.msgs[0].flags    = (__u16)0;                 /* !I2C_M_RD = write */
//    rw_arg.msgs[0].flags    = (__u16)I2C_M_RD;          /* I2C_M_RD = read */
//    rw_arg.msgs[0].flags    = (__u16)I2C_M_NOSTART;     /* write and no stop */
    rw_arg.msgs[0].len      = (__u16)I2C_DEV_REG_WIDTH; /* data size = reg */
    rw_arg.msgs[0].buf      = (__u8 *)buf;

    rw_arg.msgs[1].addr     = (__u16)(addr>>1);
    rw_arg.msgs[1].flags    = (__u16)I2C_M_RD;          /* I2C_M_RD = read */
    rw_arg.msgs[1].len      = (__u16)num;               /* data size */
    rw_arg.msgs[1].buf      = (__u8 *)dat;

    if (ioctl(i2c_file, I2C_RDWR, &rw_arg) < 0)
    {
        printf("IoCtl : error - %s \n", __func__);
        return (-1);
    }

#if 0
    for (idx = 0; idx < num; idx++)
    {
    	*dat++ = buf[idx];
    }
#endif

//    printf("IoCtl : success - %s \n", __func__);

	return 0;
}

int i2c_a2b_bulk_write(UINT08 addr, UINT16 reg, UINT08 *dat, UINT08 num)
{
    struct i2c_rdwr_ioctl_data  rw_arg;
    struct i2c_msg              ctl_msgs[1];
    UINT08                      buf[I2C_BULK_WRITE_MAX + I2C_DEV_REG_WIDTH];
    UINT08                      idx;

    if (i2c_file < 0)
        return (-1);

    if (num > I2C_BULK_WRITE_MAX)
    {
        return (-1);
    }

    rw_arg.msgs     = ctl_msgs;
    rw_arg.nmsgs    = 1;

    buf[0] = (reg >> 8) & 0xFF;
    buf[1] = (reg >> 0) & 0xFF;

	for (idx = 0; idx < num; idx++)
    {
		buf[I2C_DEV_REG_WIDTH + idx]    = dat[idx];
	}

    rw_arg.msgs[0].addr     = (__u16)(addr>>1);
    rw_arg.msgs[0].flags    = (__u16)0;                             /* !I2C_M_RD = write */
//    rw_arg.msgs[0].flags    = (__u16)(!I2C_M_RD|I2C_M_NO_RD_ACK);   /* !I2C_M_RD = write */
    rw_arg.msgs[0].len      = (__u16)(I2C_DEV_REG_WIDTH + num);     /* data size = { REG(MSB), REG(LSB), DAT(MSB), DAT(LSB) } */
    rw_arg.msgs[0].buf      = (__u8 *)buf;

    if (ioctl(i2c_file, I2C_RDWR, &rw_arg) < 0)
    {
        printf("IoCtl : error - %s \n", __func__);
        return (-1);
    }

	return 0;
}

void i2c_close(void)
{
#if 0
	if( i2c_file )
	{
		close( i2c_file );
		i2c_file = 0;
	}
#endif
}

int i2c_open(UINT08 addr)
{
    int     adp_num = I2C_CH_NUM;
    char    devName[32];

    if (i2c_file > -1)
        return 0;

    sprintf(devName, "/dev/i2c-%d", adp_num);
    i2c_file = open(devName, O_RDWR);
    if (i2c_file < 0)
    {
        printf("%s error\n", __func__);
    }

    if (ioctl(i2c_file, I2C_SLAVE, (addr>>1)) < 0)
    {
        printf("IoCtl : error - Slace dev\n");

        close(i2c_file);
        i2c_file = (-1);
    }

    return i2c_file;
}

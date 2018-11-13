
#ifndef __SYSTEM_TYPE_H__
#define __SYSTEM_TYPE_H__

/*==============================================================================
 * Includes the user header files if neccessry.
 *============================================================================*/ 
#if defined(__KERNEL__) /* Linux kernel */
    #include <linux/io.h>
    #include <linux/kernel.h>
    #include <linux/delay.h>
    #include <linux/mm.h>
    #include <linux/mutex.h>
    #include <linux/uaccess.h>

#elif defined(WINCE)
    #include <windows.h>
    #include <drvmsg.h>

#else
	#include <stdio.h>
#endif

#ifdef __cplusplus 
extern "C"{ 
#endif  

/*============================================================================
* Modifies the basic data types if neccessry.
*===========================================================================*/
typedef int					BOOL;
typedef signed char			S8;
typedef unsigned char		U8;
typedef signed short		S16;
typedef unsigned short		U16;
typedef signed int			S32;
typedef unsigned int		U32;

typedef int                 INT;
typedef unsigned int        UINT;
typedef long                LONG;
typedef unsigned long       ULONG;
 
typedef volatile U8			VU8;
typedef volatile U16		VU16;
typedef volatile U32		VU32;

#define INLINE		__inline


#ifdef __cplusplus 
} 
#endif 

#endif /* __SYSTEM_TYPE_H__ */


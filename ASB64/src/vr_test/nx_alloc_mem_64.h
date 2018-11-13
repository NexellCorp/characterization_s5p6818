#ifndef __NX_ALLOC_MEM_H__
#define __NX_ALLOC_MEM_H__

#include <inttypes.h>

#ifdef	__cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------
//			Enum & Define
enum {
	NX_MEM_MAP_LINEAR = 0,		//	Linear Memory Type
	NX_MEM_MAP_TILED  = 1,		//	Tiled Memory Type
};


//
//	Nexell Private Memory Type
//
typedef struct
{
	uint64_t		privateDesc;		//	Private Descriptor's Address
	int32_t			align;
	int32_t			size;
	void*		virAddr;
	uint64_t		phyAddr;
} NX_MEMORY_INFO, *NX_MEMORY_HANDLE;


//	Nexell Private Memory Allocator
NX_MEMORY_HANDLE NX_AllocateMemory( int32_t size, int32_t align );
void NX_FreeMemory( NX_MEMORY_HANDLE handle );


#ifdef	__cplusplus
};
#endif

#endif	//	__NX_ALLOC_MEM_H__

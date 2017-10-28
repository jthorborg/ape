#ifndef CPPAPE_RUNTIME_H
#define CPPAPE_RUNTIME_H

#ifndef __cplusplus
#error baselib.h can only be used with a C++ compiler
#endif

#define __inline__ inline
#define __inline inline
#define __cdecl
#define _cdecl
#define RC_INVOKED
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#define __attribute__(x)
#define _CRT_ERRNO_DEFINED
#include <stddef.h>
#include "shared-src/ape/SharedInterface.h"

#undef __inline__

#undef NULL
#define NULL 0

typedef char bool;

enum
{
	false, true
};


inline size_t nextpow2(size_t current)
{
	size_t p = 1;
	while (p < current)
		p <<= 1;
	return p;
}

template<class T>
size_t distance(const T * begin, const T * end)
{
	return end - begin;
}

int printf(char * fmt, ...);

#endif
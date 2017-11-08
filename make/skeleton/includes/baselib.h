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
#include "tcc/_mingw.h"
#undef __CRT_INLINE
#define __CRT_INLINE inline
#include "tcc/stddef.h"
#include "shared-src/ape/SharedInterface.h"

//#undef __inline__

#undef NULL
#define NULL 0

typedef char bool;

enum
{
	false, true
};

void *operator new(unsigned am, void * loc);

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

int printf(const char * fmt, ...);
APE_SharedInterface& getInterface();


struct Status
{
	enum Codes
	{
		Ok = 0,
		Error = 1,
		Wait = 2,
		Silent = 3,
		Ready = 4,
		Disabled = 5,
		Handled = 6,
		NotImplemented = 7
	};

	Status() {}
	Status(Codes c) : code(c) {}
	Status& operator = (Codes c)
	{
		code = c;
		return *this;
	}

	operator Codes ()
	{
		return code;
	}

	operator APE_Status()
	{
		return (APE_Status)code;
	}

private:
	Codes code;
};

#endif
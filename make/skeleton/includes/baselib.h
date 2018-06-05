#ifndef CPPAPE_RUNTIME_H
#define CPPAPE_RUNTIME_H

#ifndef __cplusplus
#error baselib.h can only be used with a C++ compiler
#endif


#include <cstddef>
#include "shared-src/ape/SharedInterface.h"
#undef NULL
#define NULL 0


inline size_t nextpow2(size_t current)
{
	size_t p = 1;
	while (p < current)
		p <<= 1;
	return p;
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
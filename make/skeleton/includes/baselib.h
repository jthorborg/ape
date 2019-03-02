#ifndef CPPAPE_RUNTIME_H
#define CPPAPE_RUNTIME_H

#ifndef __cplusplus
#error baselib.h can only be used with a C++ compiler
#endif


#include <cstddef>
#include "shared-src/ape/SharedInterface.h"
#undef NULL
#define NULL 0


// #define CPPAPE_RELEASE

#ifdef CPPAPE_RELEASE
#define CPPAPE_NOEXCEPT_IF_RELEASE noexcept
#else
#define CPPAPE_NOEXCEPT_IF_RELEASE
#endif

inline size_t nextpow2(size_t current)
{
	size_t p = 1;
	while (p < current)
		p <<= 1;
	return p;
}

int printf(const char * fmt, ...);
[[noreturn]] void abort(const char* reason);

#define printf_once(...) \
	static bool __once_flag ## __COUNTER__ = false; \
	if(!(__once_flag ## __COUNTER__)) \
	{	\
		__once_flag ## __COUNTER__ = true; \
		printf(__VA_ARGS__); \
	}

namespace ape
{
	APE_SharedInterface& getInterface();

	struct StatusCode
	{
		static constexpr APE_Status
			Ok = STATUS_OK,
			Error = STATUS_ERROR,
			Wait = STATUS_WAIT,
			Silent = STATUS_SILENT,
			Ready = STATUS_READY,
			Disabled = STATUS_DISABLED,
			Handled = STATUS_HANDLED,
			NotImplemented = STATUS_NOT_IMPLEMENTED;

	};
}



#endif
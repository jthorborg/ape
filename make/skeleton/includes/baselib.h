#ifndef CPPAPE_RUNTIME_H
#define CPPAPE_RUNTIME_H

#ifndef __cplusplus
#error baselib.h can only be used with a C++ compiler
#endif

#include <cstddef>

#ifdef __cppape
#include "shared-src/ape/SharedInterface.h"
#else
// (folder layout different in dev mode)
#include <SharedInterface.h>
#endif

#ifdef NULL
#undef NULL
#endif

#define NULL 0


// #define CPPAPE_RELEASE

#ifdef CPPAPE_RELEASE
#define CPPAPE_NOEXCEPT_IF_RELEASE noexcept
#else
#define CPPAPE_NOEXCEPT_IF_RELEASE
#endif


[[noreturn]] void abort(const char* reason);

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

	namespace detail
	{
		struct PluginResource
		{
			~PluginResource()
			{
				getInterface().destroyResource(&getInterface(), 0, 0);
			}
		};
	}
}


#endif
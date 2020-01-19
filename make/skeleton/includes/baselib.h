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

// #define CPPAPE_RELEASE

#ifdef CPPAPE_RELEASE
#define CPPAPE_NOEXCEPT_IF_RELEASE noexcept
#else
#define CPPAPE_NOEXCEPT_IF_RELEASE
#endif

/// <summary>
/// Terminate the script (not the host application!) safely,
/// with a reason. All resources will automatically be cleaned up.
/// </summary>
[[noreturn]] void abort(const char* reason);

namespace ape
{
	/// <summary>
	/// Base traits class for all classes that are displayed in the GUI.
	/// </summary>
	class UIObject {};

	/// <summary>
	/// Acquire the low-level C API.
	/// </summary>
	APE_SharedInterface& getInterface();

	/// <summary>
	/// Low level error code used in certian comms APIs.
	/// </summary>
	struct StatusCode
	{
		static constexpr APE_Status
			/// <summary>
			/// Operation succeeded.
			/// </summary>
			Ok = STATUS_OK,
			/// <summary>
			/// Error at operation, state inconsistent.
			/// </summary>
			Error = STATUS_ERROR,
			/// <summary>
			/// Not ready yet. Deprecated.
			/// </summary>
			Wait [[deprecated]] = STATUS_WAIT,
			/// <summary>
			/// Silent. Deprecated.
			/// </summary>
			Silent [[deprecated]] = STATUS_SILENT,
			/// <summary>
			/// Ready.
			/// </summary>
			Ready  = STATUS_READY,
			/// <summary>
			/// Disabled. Deprecated.
			/// </summary>
			Disabled [[deprecated]] = STATUS_DISABLED,
			/// <summary>
			/// Operation handled.
			/// </summary>
			Handled = STATUS_HANDLED,
			/// <summary>
			/// No support for operation.
			/// </summary>
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
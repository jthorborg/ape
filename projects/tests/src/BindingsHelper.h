#ifndef BINDINGS_HELPER_H
	#define BINDINGS_HELPER_H

	#include <ape/SharedInterface.h>

	struct BindingsHelper
	{
		typedef void (APE_API * function)();
		static constexpr std::size_t functionSize = sizeof(function);

		static std::size_t numFunctions()
		{
			return offsetof(APE::BindingsInterfaceResolver, extra) / functionSize;
		}
	};

#endif
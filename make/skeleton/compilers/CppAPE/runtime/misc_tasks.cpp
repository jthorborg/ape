#include "misc_tasks.h"
#include <trace.h>

namespace ape
{

	APE_SharedInterface& getInterface();

	namespace Tracing
	{

	#ifdef CPPAPE_TRACING_ENABLED
		Tracer GlobalTracer;
	#endif

		void ResetTracers()
		{
	#ifdef CPPAPE_TRACING_ENABLED
			GlobalTracer.reset();
	#endif
		}

		void PresentTracers()
		{
	#ifdef CPPAPE_TRACING_ENABLED
			const auto& traces = GlobalTracer.getTraces();

			for (const auto& trace : traces)
			{
				const char* traceNames[2] = { trace.first.first, trace.first.second };
				getInterface().presentTrace(&getInterface(), traceNames, trace.first.second ? 2 : 1, trace.second.data.data(), trace.second.index);
			}
	#endif
		}
	}


}
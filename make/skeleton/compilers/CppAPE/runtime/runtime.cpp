#include <baselib.h>
#include <processor.h>
#include <shared-src/tcc4ape/ScriptBindings.h>
#include <cstdarg>
#include <cmath>
#include "misc_tasks.h"

namespace ape
{
	APE_SharedInterface * lastIFace;

	namespace detail
	{
		FactoryBase::ProcessorCreater pluginCreater;

		void FactoryBase::SetCreater(ProcessorCreater factory)
		{
			pluginCreater = factory;
		}
	}


	APE_SharedInterface& getInterface()
	{
		// TODO: thread_local
		return *lastIFace;
	}

	
	void* memoryAlloc(std::size_t am, std::size_t align)
	{
		return lastIFace->alloc(lastIFace, APE_Alloc_Tiny, am, align);
	}

	void memoryFree(void* loc)
	{
		lastIFace->free(lastIFace, loc);
	} 
}

using namespace ape;

extern "C"
{
	int vsnprintf(char * s, size_t n, const char * format, va_list arg);
	int snprintf(char * s, size_t n, const char * format, ...);

	void abort();
};

int printf(const char * fmt, ...)
{
	if (lastIFace == NULL)
		return -1;

	char buf[1024];

	va_list args;
	va_start(args, fmt);
	int ret = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	lastIFace->printThemedLine(lastIFace, APE_TextColour_Default, "%s", buf);
	return ret;
}

[[noreturn]]
void abort(const char * reason)
{
	if (lastIFace == NULL)
		abort();

	lastIFace->abortPlugin(lastIFace, reason);

}

extern "C"
{
	void _ccore_assert(const char* expression, const char* file, unsigned line)
	{
		lastIFace->printThemedLine(lastIFace, APE_TextColour_Error, "Assertion in %s:%d: %s", file, (int)line, expression);
		abort("Assertion failure");
	}

	void * SCRIPT_API PluginCreate(APE_SharedInterface * iface)
	{
		lastIFace = iface;
		if (!detail::pluginCreater)
		{
			iface->printThemedLine(iface, APE_TextColour_Error, "Error: No plugin to run, did you forget GlobalData(YourEffect, \"\")? This can also indicate no global constructors were run.");
			return NULL;
		}

		Processor * p = detail::pluginCreater();
		return p;
	}

	void SCRIPT_API PluginDelete(ScriptInstance * instance)
	{
		delete (Processor*)instance;
	}

	PluginGlobalData NAME_GLOBAL_DATA = {
		0, // size_t allocSize;
		0, // size_t version;
		"C++", // const char * name;
		1, //int wantsToSelfAlloc;
		PluginCreate, // void * (SCRIPT_API * PluginAlloc)(APE_SharedInterface *);
		PluginDelete //void * (SCRIPT_API * PluginFree)(ScriptInstance *);
	};
};

extern "C"
{

	APE_Status SCRIPT_API NAME_PROCESS_REPLACE(ScriptInstance * instance, APE_SharedInterface * iface, float ** inputs, float ** outputs, int frames)
	{
		Tracing::ResetTracers();

		lastIFace = iface;
		Processor * p = (Processor*)instance;
		if (!p)
			return StatusCode::Error;
		
		auto configuration = p->config();

		p->processFrames(
			{ inputs, configuration.inputs, frames },
			{ outputs,  configuration.outputs, frames },
			frames
		);

		Tracing::PresentTracers();

		return StatusCode::Ok;
	}

	APE_Status SCRIPT_API NAME_INIT(ScriptInstance * instance, APE_SharedInterface * iface)
	{
		lastIFace = iface;
		Processor * p = (Processor*)instance;

		if(!p)
			return StatusCode::Error;

		p->init();

		return StatusCode::Ready;
	}

	APE_Status SCRIPT_API NAME_END(ScriptInstance * instance, APE_SharedInterface * iface)
	{
		lastIFace = iface;
		Processor * p = (Processor*)instance;

		if (!p)
			return StatusCode::Error;

		p->close();

		return StatusCode::Ok;

	}

	APE_Status SCRIPT_API NAME_EVENT_HANDLER(ScriptInstance * instance, APE_SharedInterface * iface, APE_Event * event)
	{
		lastIFace = iface;
		Processor * p = (Processor*)instance;

		return p ? p->onEvent(event) : StatusCode::Error;
	}

};
#include <baselib.h>
#include <processor.h>
#include <shared-src/tcc4ape/ScriptBindings.h>
#include "ctorsdtors.h"
#include <stdarg.h>

APE_SharedInterface * lastIFace;
FactoryBase::ProcessorCreater pluginCreater;

void FactoryBase::SetCreater(ProcessorCreater factory)
{
	pluginCreater = factory;
}

APE_SharedInterface& getInterface()
{
	// TODO: thread_local
	return *lastIFace;
}

extern "C"
{
	void * malloc(size_t size);
	void * calloc(size_t size, size_t count);
	void free(void * pointer);
	int vsnprintf(char * s, size_t n, const char * format, va_list arg);
};

int printf(char * fmt, ...)
{
	if (lastIFace == NULL)
		return -1;

	char buf[1024];

	va_list args;
	va_start(args, fmt);
	int ret = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	lastIFace->printLine(lastIFace, 0, "%s", buf);
	return ret;
}

extern "C"
{

	void * SCRIPT_API PluginCreate(APE_SharedInterface * iface)
	{
		runtime_init();
		lastIFace = iface;
		if (!pluginCreater)
		{
			iface->printLine(iface, 0xFF000000, "Error: No plugin to run, did you forget GlobalData(YourEffect, \"\")?");
			return NULL;
		}
		Processor * p = pluginCreater();
		return p;
	}

	void SCRIPT_API PluginDelete(ScriptInstance * instance)
	{
		delete (Processor*)instance;
		runtime_exit();
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




void *operator new(unsigned int am)
{
	return calloc(am, 1);
}

void operator delete(void * loc)
{
	free(loc);
}

extern "C"
{

	APE_Status SCRIPT_API NAME_PROCESS_REPLACE(ScriptInstance * instance, APE_SharedInterface * iface, float ** inputs, float ** outputs, int frames)
	{
		lastIFace = iface;
		Processor * p = (Processor*)instance;
		return p ? p->process(inputs, outputs, frames) : Status(Status::Error);
	}

	APE_Status SCRIPT_API NAME_INIT(ScriptInstance * instance, APE_SharedInterface * iface)
	{
		lastIFace = iface;
		Processor * p = (Processor*)instance;
		return p ? p->init() : Status(Status::Error);
	}

	APE_Status SCRIPT_API NAME_END(ScriptInstance * instance, APE_SharedInterface * iface)
	{
		lastIFace = iface;
		Processor * p = (Processor*)instance;
		return p ? p->close() : Status(Status::Error);
	}

	APE_Status SCRIPT_API NAME_EVENT_HANDLER(ScriptInstance * instance, APE_SharedInterface * iface, APE_Event * event)
	{
		lastIFace = iface;
		Processor * p = (Processor*)instance;
		return p ? p->onEvent(event) : Status(Status::Error);
	}

};
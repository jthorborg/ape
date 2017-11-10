#include <baselib.h>
#include <processor.h>
#include <shared-src/tcc4ape/ScriptBindings.h>
#include "ctorsdtors.h"
#include <stdarg.h>
#include <mathlib.h>

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
	int vsnprintf(char * s, size_t n, const char * format, va_list arg);
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

	lastIFace->printLine(lastIFace, 0, "%s", buf);
	return ret;
}

void abort(const char * reason)
{
	if (lastIFace == NULL)
		abort();

	lastIFace->abortPlugin(lastIFace, reason);

}

int fpclassify(double x) {
	union { double f; uint64_t i; } u;
	u.f = x;
	int e = u.i >> 52 & 0x7ff;
	if (!e) return u.i << 1 ? FP_SUBNORMAL : FP_ZERO;
	if (e == 0x7ff) return u.i << 12 ? FP_NAN : FP_INFINITE;
	return FP_NORMAL;
}

int fpclassify(float x) {
	union { float f; uint32_t i; } u;
	u.f = x;
	int e = u.i >> 23 & 0xff;
	if (!e) return u.i << 1 ? FP_SUBNORMAL : FP_ZERO;
	if (e == 0xff) return u.i << 9 ? FP_NAN : FP_INFINITE;
	return FP_NORMAL;
}


extern "C"
{

	void * SCRIPT_API PluginCreate(APE_SharedInterface * iface)
	{
		runtime_init();
		lastIFace = iface;
		if (!pluginCreater)
		{
			iface->printLine(iface, 0xFF0000FF, "Error: No plugin to run, did you forget GlobalData(YourEffect, \"\")?");
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
	return lastIFace->alloc(lastIFace, APE_Alloc_Tiny, am);
}

void *operator new(unsigned int am, void * loc)
{
	return loc;
}


void operator delete(void * loc)
{
	lastIFace->free(lastIFace, loc);
}

extern "C"
{

	APE_Status SCRIPT_API NAME_PROCESS_REPLACE(ScriptInstance * instance, APE_SharedInterface * iface, float ** inputs, float ** outputs, int frames)
	{
		lastIFace = iface;
		Processor * p = (Processor*)instance;
		if (!p)
			return Status(Status::Error);

		p->process(inputs, outputs, frames);
		return Status(Status::Ok);
	}

	APE_Status SCRIPT_API NAME_INIT(ScriptInstance * instance, APE_SharedInterface * iface)
	{
		lastIFace = iface;
		Processor * p = (Processor*)instance;

		if(!p)
			return Status(Status::Error);

		p->init();

		return Status(Status::Ready);
	}

	APE_Status SCRIPT_API NAME_END(ScriptInstance * instance, APE_SharedInterface * iface)
	{
		lastIFace = iface;
		Processor * p = (Processor*)instance;

		if (!p)
			return Status(Status::Error);

		p->close();

		return Status(Status::Ok);

	}

	APE_Status SCRIPT_API NAME_EVENT_HANDLER(ScriptInstance * instance, APE_SharedInterface * iface, APE_Event * event)
	{
		lastIFace = iface;
		Processor * p = (Processor*)instance;

		return p ? p->onEvent(event) : Status(Status::Error);
	}

};
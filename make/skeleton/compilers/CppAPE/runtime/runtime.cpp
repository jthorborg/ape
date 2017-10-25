#include <runtime.h>
#include <processor.h>
#include <shared-src/tcc4ape/ScriptBindings.h>


extern "C"
{
	void * malloc(size_t size);
	void * calloc(size_t size, size_t count);
	void free(void * pointer);
	int printf(char * fmt, ...);
};

extern "C"
{

	void * SCRIPT_API PluginCreate(APE_SharedInterface * iface)
	{
		return NULL;
	}

	void SCRIPT_API PluginDelete(ScriptInstance * instance)
	{
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

	APE_Status SCRIPT_API NAME_PROCESS_REPLACE(ScriptInstance *, APE_SharedInterface *, float**, float**, int)
	{
		return STATUS_OK;
	}

	APE_Status SCRIPT_API NAME_INIT(ScriptInstance *, APE_SharedInterface *)
	{
		return STATUS_READY;
	}

	APE_Status SCRIPT_API NAME_END(ScriptInstance *, APE_SharedInterface *)
	{
		return STATUS_OK;
	}

	APE_Status SCRIPT_API NAME_EVENT_HANDLER(ScriptInstance *, APE_SharedInterface *, APE_Event *)
	{
		return STATUS_OK;
	}

};
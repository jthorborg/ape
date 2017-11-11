#ifndef TCC4APE_SCRIPTBINDINGS_H
#define TCC4APE_SCRIPTBINDINGS_H

#include "../ape/ape.h"
#include "../ape/Events.h"
#include "../ape/SharedInterface.h"

#define SCRIPT_API APE_API
// names of function used in script

#define TCC4APE_STRINGIFY_2(x) #x
#define TCC4APE_STRINGIFY(x) TCC4APE_STRINGIFY_2(x)
#define NAME_PROCESS_REPLACE processReplacing
#define NAME_INIT onLoad
#define NAME_END onUnload
#define NAME_EVENT_HANDLER onEvent
#define NAME_GLOBAL_DATA __global_data

#define SYMBOL_PROCESS_REPLACE TCC4APE_STRINGIFY(NAME_PROCESS_REPLACE)
#define SYMBOL_INIT TCC4APE_STRINGIFY(NAME_INIT)
#define SYMBOL_END TCC4APE_STRINGIFY(NAME_END)
#define SYMBOL_EVENT_HANDLER TCC4APE_STRINGIFY(NAME_EVENT_HANDLER)
#define SYMBOL_GLOBAL_DATA TCC4APE_STRINGIFY(NAME_GLOBAL_DATA)

#define SCRIPT_API APE_API

typedef void ScriptInstance;

typedef APE_Status (SCRIPT_API * APE_ProcessReplacer) (ScriptInstance *, APE_SharedInterface *, float**, float**, int);
typedef APE_Status (SCRIPT_API * APE_Init) (ScriptInstance *, APE_SharedInterface *);
typedef APE_Status (SCRIPT_API * APE_End) (ScriptInstance *, APE_SharedInterface *);
typedef APE_Status (SCRIPT_API * APE_EventHandler) (ScriptInstance *, APE_SharedInterface *, APE_Event *);

struct PluginGlobalData
{
	size_t allocSize;
	size_t version;
	const char * name;
	int wantsToSelfAlloc;
	void * (SCRIPT_API * PluginAlloc)(APE_SharedInterface *);
	void (SCRIPT_API * PluginFree)(ScriptInstance *);
};

#ifdef __cplusplus
extern "C" {
	#endif
	extern APE_Status SCRIPT_API NAME_PROCESS_REPLACE(ScriptInstance *, APE_SharedInterface *, float**, float**, int);
	extern APE_Status SCRIPT_API NAME_INIT(ScriptInstance *, APE_SharedInterface *);
	extern APE_Status SCRIPT_API NAME_END(ScriptInstance *, APE_SharedInterface *);
	extern APE_Status SCRIPT_API NAME_EVENT_HANDLER(ScriptInstance *, APE_SharedInterface *, APE_Event *);

	#ifdef __cplusplus
};
#endif

#endif
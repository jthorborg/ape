/*************************************************************************************

	syswrap Compiler for Audio Programming Environment. 

    Copyright (C) 2013 Janus Lynggaard Thorborg [LightBridge Studios]

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

	See \licenses\ for additional details on licenses associated with this program.

**************************************************************************************

	file:syswrap.h
		
		Implements the interface for the compiler.

*************************************************************************************/
// includes
#include "APESupport.h"
#include <string>
#include <cpl/CModule.h>

// names of function used in script
#define SYMBOL_PROCESS_REPLACE "processReplacing"
#define SYMBOL_INIT "onLoad"
#define SYMBOL_END "onUnload"
#define SYMBOL_EVENT_HANDLER "onEvent"
#define SYMBOL_GET_DATA "getProgramInfo"

namespace syswrap
{
	// alias of the plugin's memory block (we cannot know the type exactly)
	typedef void * PluginData;

	// Helper struct to manage the plugin
	struct CPlugin {
		typedef Status (_cdecl * processReplacer)(PluginData*, CSharedInterface *, float**, float**, VstInt32);
		typedef Status (_cdecl * init)(PluginData*, CSharedInterface *);
		typedef Status (_cdecl * end)(PluginData*, CSharedInterface *);
		typedef Status (_cdecl * eventHandler)(PluginData*, CSharedInterface *, CEvent *);
		processReplacer processor;
		init entrypoint;
		end exitpoint;
		eventHandler handler;
		Status pluginStatus;
		CPlugin() : processor(NULL), entrypoint(NULL), exitpoint(NULL), pluginStatus(STATUS_DISABLED), handler(NULL) {}
		// test if plugin is ready. bSkipStatus denotes whether to include a test of the status
		bool test(bool bSkipStatus = false)
		{
															// exploiting short-circut evalutation to bypass status check. weee
			return processor && entrypoint && exitpoint && (bSkipStatus || (pluginStatus == STATUS_READY));

		}
		// tests if the plugin has an event handler
		bool hasEventHandler() {
			return handler != NULL;
		}
	};

	// main class of this program.
	class CCompiler
	{
		// opaque pointer to be used when error printing
		void * op;
		// first time load?
		static bool isLoaded;
		// increasing instance number, used to identify instance with dll name
		static int instanceCount;
		int instanceNumber;
		// error function
		errorFunc_t errorFunc;
		// the project our instance is associated with
		CProject * project;
		// plugin helper
		CPlugin plugin;
		// loaded module	
		cpl::CModule module;
		// pointer to the compiled code's global description struct
		struct _plugin_global_data {
			size_t allocSize;
			size_t version;
			const char * name;
			int selfAlloc;
			void * (_cdecl *palloc)(void*);
			void * (_cdecl *pfree)(void*);
		} * global_data;
		typedef _plugin_global_data *(_cdecl *getData_t)();
		// pointer to the code's plugin data struct
		void * plugin_data;
	public:
		// con/destructor
		CCompiler();
		~CCompiler();
		// changes current project
		void setProject(CProject * p) { project = p; }
		// prints to error function
		void print(const char * s);
		// inits memory for the plugin according to descriptor struct
		bool initLocalMemory();
		// free's the memory used
		bool freeLocalMemory();
		// wrappers for the compiler api
		Status processReplacing(float ** in, float ** out, int frames);
		Status compileProject();
		Status setErrorFunc(void * op, errorFunc_t e);
		Status releaseProject();
		Status initProject();
		Status activateProject();
		Status disableProject(bool didMisbehave);
		Status onEvent(CEvent * e);
	};
};



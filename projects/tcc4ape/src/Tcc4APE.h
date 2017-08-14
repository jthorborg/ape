/*************************************************************************************

	Tiny C Compiler for Audio Programming Environment. 

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

	file:Tcc4APE.h
		
		Implements the interface for the compiler.

*************************************************************************************/
// includes
#include "APESupport.h"
#include <string>
#include <cpl/MacroConstants.h>
#include "libtcc.h"
#include <cpl/CModule.h>

// names of function used in script
#define SYMBOL_PROCESS_REPLACE "processReplacing"
#define SYMBOL_INIT "onLoad"
#define SYMBOL_END "onUnload"
#define SYMBOL_EVENT_HANDLER "onEvent"

// objective-c export
size_t GetBundlePath(char *, size_t);

namespace TCC4Ape
{
	// alias of the plugin's memory block (we cannot know the type exactly)
	typedef void * PluginData;
	using namespace APE;
	std::string GetDirectoryPath();

	// Helper struct to manage the plugin
	struct CPlugin {
		typedef Status (STD_API * processReplacer)(PluginData*, CSharedInterface *, float**, float**, int);
		typedef Status (STD_API * init)(PluginData*, CSharedInterface *);
		typedef Status (STD_API * end)(PluginData*, CSharedInterface *);
		typedef Status (STD_API * eventHandler)(PluginData*, CSharedInterface *, CEvent *);
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

	/*
		Function pointers ('linkage') for TCC.
		Doing it this way abstracts whether we dynamically or statically link.
		always call initializeBindings() and check result before trying to call anything.
	*/
	struct TCC
	{
		decltype(tcc_new) * newState;
		decltype(tcc_set_lib_path) * setLibPath;
		decltype(tcc_add_include_path) * addIncludePath;
		decltype(tcc_set_output_type) * setOutputType;
		decltype(tcc_set_error_func) * setErrorFunc;
		decltype(tcc_compile_string) * compileString;
		decltype(tcc_delete) * deleteState;
		decltype(tcc_relocate) * relocate;
		decltype(tcc_get_symbol) * getSymbol;
		decltype(tcc_define_symbol) * defineSymbol;
		decltype(tcc_set_options) * setOptions;
		bool isInitialized;
		bool initializeBindings();

		#ifndef TCC4APE_STATIC_LINK
			cpl::CModule module;
		#endif

	};

	// main class of this program.
	class CCompiler
	{
		// opaque pointer to be used when error printing
		void * op;
		// error function
		errorFunc_t errorFunc;
		// the project our instance is associated with
		CProject * project;
		// the instance of the tcc compiler
		TCCState * state;
		// plugin helper
		CPlugin plugin;
		// pointer to the compiled code's global description struct
		struct __alignas(4) _plugin_global_data {
			size_t allocSize;
			size_t version;
			const char * name;
			int selfAlloc;
			void * (STD_API *palloc)(void*);
			void * (STD_API *pfree)(void*);

		} * global_data;
		// pointer to the code's plugin data struct
		void * plugin_data;
	public:

		static TCC tcc;

		// con/destructor
		CCompiler();
		~CCompiler();
		// changes current project
		void setProject(CProject * p) { project = p; }
		// prints to error function
		void print(const char * s);
		// inits memory for the plugin according to desricptor struct
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
		Status disableProject();
		Status onEvent(CEvent * e);
	};
};



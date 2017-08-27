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

#include <ape/ProtoCompiler.hpp>
#include <string>
#include <cpl/MacroConstants.h>
#include "libtcc.h"
#include <cpl/CModule.h>
#include <cpl/Utility.h>
#include <memory>
#include <mutex>

// names of function used in script
#define SYMBOL_PROCESS_REPLACE "processReplacing"
#define SYMBOL_INIT "onLoad"
#define SYMBOL_END "onUnload"
#define SYMBOL_EVENT_HANDLER "onEvent"
#define SCRIPT_API APE_API

namespace TCC4Ape
{
	using namespace APE;
	// alias of the plugin's memory block (we cannot know the type exactly)
	typedef void ScriptInstance;
	using Status = APE_Status;

	// Helper struct to manage the plugin
	struct ScriptPlugin 
	{
		typedef Status (SCRIPT_API * ProcessReplacer)
			(ScriptInstance *, APE_SharedInterface *, float**, float**, int);
		typedef Status (SCRIPT_API * Init)
			(ScriptInstance *, APE_SharedInterface *);
		typedef Status (SCRIPT_API * End)
			(ScriptInstance *, APE_SharedInterface *);
		typedef Status (SCRIPT_API * EventHandler)
			(ScriptInstance *, APE_SharedInterface *, Event *);

		ScriptPlugin() : processor(nullptr), entrypoint(nullptr), exitpoint(nullptr), pluginStatus(STATUS_DISABLED), handler(nullptr) {}

		// test if plugin is ready. bSkipAPE_Status denotes whether to include a test of the Status
		bool test(bool bSkipAPE_Status = false)
		{
			return processor && entrypoint && exitpoint && (bSkipAPE_Status || (pluginStatus == STATUS_READY));
		}
		// tests if the plugin has an event handler
		bool hasEventHandler() 
		{
			return handler != nullptr;
		}

		ProcessReplacer processor;
		Init entrypoint;
		End exitpoint;
		EventHandler handler;
		Status pluginStatus;
	};

	class TCCBindings
	{
	public:

		class CompilerAccess : cpl::Utility::CNoncopyable
		{
		public:

			CompilerAccess() : bindings(instance()), compilerLock(bindings.compilerMutex) {}

			bool isLinked() const noexcept { return bindings.linkedCorrectly; }

			TCCState * createState() const  { return bindings.newState();}
			void deleteState(TCCState * s) const  {	return bindings.deleteState(s);	}
			void setLibPath(TCCState * s, const char *path) const  { return bindings.setLibPath(s, path); }
			void setErrorFunc(TCCState * s, void * errorOpaque, ErrorFunc errorFunction) const  { return bindings.setErrorFunc(s, errorOpaque, errorFunction); }
			void addIncludePath(TCCState * s, const char * pathName) const { bindings.addIncludePath(s, pathName);	}
			void defineSymbol(TCCState * s, const char * symbol, const char * value) const { return bindings.defineSymbol(s, symbol, value); }
			bool compileString(TCCState * s, const char * buffer) const { return bindings.compileString(s, buffer) != -1; }
			int setOutputType(TCCState * s, int outputType) const { return bindings.setOutputType(s, outputType);	}
			int relocate(TCCState * s, void * options) const  { return bindings.relocate(s, options); }
			void * getSymbol(TCCState * s, const char * name) const  { return bindings.getSymbol(s, name); }
			void setOptions(TCCState * s, const char * commands) const  { return bindings.setOptions(s, commands); }

		private:

			TCCBindings & bindings;
			std::lock_guard<std::recursive_mutex> compilerLock;
		};

	private:

		TCCBindings();

		static TCCBindings & instance()
		{
			static TCCBindings bindings;
			return bindings;
		}

		decltype(tcc_new) * newState;
		decltype(tcc_delete) * deleteState;
		decltype(tcc_set_lib_path) * setLibPath;
		decltype(tcc_set_error_func) * setErrorFunc;
		decltype(tcc_add_include_path) * addIncludePath;
		decltype(tcc_define_symbol) * defineSymbol;
		decltype(tcc_compile_string) * compileString;
		decltype(tcc_set_output_type) * setOutputType;
		decltype(tcc_relocate) * relocate;
		decltype(tcc_get_symbol) * getSymbol;
		decltype(tcc_set_options) * setOptions;

		#ifndef TCC4APE_STATIC_LINK
			cpl::CModule tccDLib;
		#endif

	private:

		bool linkedCorrectly;
		std::recursive_mutex compilerMutex;
	};

	// main class of this program.
	class ScriptCompiler : public APE::ProtoCompiler
	{
	public:

		// con/destructor
		ScriptCompiler();
		~ScriptCompiler();

		// inits memory for the plugin according to desricptor struct
		bool initLocalMemory();
		// free's the memory used
		bool freeLocalMemory();
		// wrappers for the compiler api
		Status processReplacing(float ** in, float ** out, int frames) override;
		Status compileProject() override;
		Status releaseProject() override;
		Status initProject() override;
		Status activateProject() override;
		Status disableProject() override;
		Status onEvent(Event * e) override;

	private:

		class TCCDeleter
		{
			public: void operator()(TCCState * s) { TCCBindings::CompilerAccess().deleteState(s); }
		};
		typedef std::unique_ptr<TCCState, TCCDeleter> UniqueTCC;

		UniqueTCC state;
		ScriptPlugin plugin;

		struct PluginGlobalData
		{
			std::size_t allocSize;
			std::size_t version;
			const char * name;
			int wantsToSelfAlloc;
			void * (SCRIPT_API * PluginAlloc)(void *);
			void * (SCRIPT_API * PluginFree)(void *);
		};

		ScriptInstance * pluginData = nullptr;
		PluginGlobalData * globalData = nullptr;

	};
};



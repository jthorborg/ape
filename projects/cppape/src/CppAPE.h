/*************************************************************************************

	C++ Compiler for Audio Programming Environment. 

    Copyright (C) 2017 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:CppAPE.h
		
		Implements the interface for the compiler.

*************************************************************************************/
#ifndef CPPAPE_H
#define CPPAPE_H

#include <ape/ProtoCompiler.hpp>
#include <string>
#include <cpl/MacroConstants.h>
#include <cpl/Utility.h>
#include <tcc4ape/ScriptBindings.h>
#include <memory>
#include <cpl/filesystem.h>
#include "TranslationUnit.h"

namespace CppAPE
{
	namespace fs = cpl::fs;

	using namespace ape;

	// Helper struct to manage the plugin
	struct ScriptPlugin 
	{

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

		APE_ProcessReplacer processor;
		APE_Init entrypoint;
		APE_End exitpoint;
		APE_EventHandler handler;
		Status pluginStatus;
	};

	APE_Diagnostic JitToDiagnostic(jit_error_t error);

	// main class of this program.
	class ScriptCompiler : public ape::ProtoCompiler
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
		Status processReplacing(const float * const * in, float * const * out, std::size_t frames) override;
		Status compileProject() override;
		Status releaseProject() override;
		Status initProject() override;
		Status activateProject() override;
		Status disableProject(bool didMisbehave) override;
		Status onEvent(Event * e) override;

	private:

		bool transformSource(const Project& project, std::string& source);

		/// <summary>
		/// Mutual (os-wide) exclusion should be provided by the parent caller.
		/// </summary>
		bool SetupEnvironment();
		static std::shared_ptr<CxxJitContext> acquireCxxRuntime();

		static const std::vector<const char*> defines;
		std::vector<char> memoryEffectPCH;
		std::unique_ptr<CxxJitContext> state;
		ScriptPlugin plugin;

		ScriptInstance * pluginData = nullptr;
		PluginGlobalData * globalData = nullptr;
		std::shared_ptr<CxxTranslationUnit> cxxRuntime;
	};
};


#endif

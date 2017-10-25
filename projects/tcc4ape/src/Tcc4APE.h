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
#include <ape/TCCBindings.h>
#include <tcc4ape/ScriptBindings.h>

namespace TCC4Ape
{
	using namespace APE;
	// alias of the plugin's memory block (we cannot know the type exactly)
	using Status = APE_Status;

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

		UniqueTCC state;
		ScriptPlugin plugin;

		ScriptInstance * pluginData = nullptr;
		PluginGlobalData * globalData = nullptr;

	};
};



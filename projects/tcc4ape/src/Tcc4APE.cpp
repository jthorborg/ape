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

	file:Tcc4APE.cpp
		
		Implementation of the compiler class.

*************************************************************************************/

#include "Tcc4APE.h"
#include <libtcc.h>
#include <string>
#include <cpl/Misc.h>
#include <cpl/Common.h>

namespace cpl
{
	#ifndef APE_TESTS
	const ProgramInfo programInfo
	{
		"Audio Programming Environment",
		cpl::Version::fromParts(0, 1, 0),
		"Janus Thorborg",
		"sgn",
		false,
		nullptr,
		""
	};
	#endif
};

APE::ProtoCompiler * CreateCompiler()
{
	return new TCC4Ape::ScriptCompiler();
}

void DeleteCompiler(APE::ProtoCompiler * toBeDeleted)
{
	delete toBeDeleted;
}

namespace TCC4Ape
{

	ScriptCompiler::ScriptCompiler() { }

	ScriptCompiler::~ScriptCompiler() 
	{
		if (pluginData && !freeLocalMemory())
		{
			print("[TCC4Ape] : Leaked memory on script compiler destruction, unable to free!");
		}
		
	}

	Status ScriptCompiler::compileProject()
	{
		const TCCBindings::CompilerAccess compiler;

		if (!compiler.isLinked())
		{
			print("[TCC4Ape] : Error linking against TCC, module is either not found or invalid.");
			return Status::STATUS_ERROR;
		}

		state = UniqueTCC(compiler.createState());

		if(!state) 
		{
			print("[TCC4Ape] : Error allocating state for TCC compiler");
			return Status::STATUS_ERROR;
		}

		std::string dir = cpl::Misc::DirectoryPath();
		compiler.setLibPath(state.get(), dir.c_str());

		if (getProject()->arguments)
			compiler.setOptions(state.get(), getProject()->arguments);

		compiler.addIncludePath(state.get(), (std::string(getProject()->rootPath) + "/includes").c_str());
		compiler.addIncludePath(state.get(), (std::string(getProject()->rootPath) + "/includes/tcc").c_str());
		compiler.setOutputType(state.get(), TCC_OUTPUT_MEMORY);
		compiler.setErrorFunc(state.get(), getErrorFuncDetails().first, getErrorFuncDetails().second);

		//	TCC has the nice feature of setting size_t to unsigned int even on 64-bit.
		//	Lets fix that.

		#ifdef CPL_M_64_BIT
			compiler.defineSymbol(state, "__SIZE_TYPE__", "unsigned long long");
		#endif

		if(!getProject()->isSingleString) 
		{
			print("[TCC4Ape] : Error - Compiler only supports compiling single strings (one file).");
			return Status::STATUS_ERROR;
		}

		if(getProject()->sourceString && !compiler.compileString(state.get(), getProject()->sourceString))
			return Status::STATUS_ERROR;

		return Status::STATUS_OK;
	}

	Status ScriptCompiler::releaseProject()
	{
		state = nullptr;
		return Status::STATUS_OK;
	}

	Status ScriptCompiler::initProject()
	{
		globalData = nullptr;
		const TCCBindings::CompilerAccess compiler;

		if (compiler.relocate(state.get(), TCC_RELOCATE_AUTO) == -1)
		{
			print("[TCC4Ape] : Error relocating compiled plugin.");
			return Status::STATUS_ERROR;
		}

		plugin.entrypoint 
			= reinterpret_cast<decltype(plugin.entrypoint)>(compiler.getSymbol(state.get(), SYMBOL_INIT));
		plugin.exitpoint 
			= reinterpret_cast<decltype(plugin.exitpoint)>(compiler.getSymbol(state.get(), SYMBOL_END));
		plugin.processor 
			= reinterpret_cast<decltype(plugin.processor)>(compiler.getSymbol(state.get(), SYMBOL_PROCESS_REPLACE));
		plugin.handler 
			= reinterpret_cast<decltype(plugin.handler)>(compiler.getSymbol(state.get(), SYMBOL_EVENT_HANDLER));

		if(!plugin.test(true)) 
		{
			print("[TCC4Ape] : Not all functions required to run plugin was found.");
			return Status::STATUS_ERROR;
		}

		if(!compiler.getSymbol(state.get(), "check"))
		{
			print("[TCC4Ape] : Unable to find certain symbols, did you forget to include \"CInterface.h\"?");
			return Status::STATUS_ERROR;
		}

		globalData = reinterpret_cast<PluginGlobalData *>(compiler.getSymbol(state.get(), "_global_data"));
		if(!globalData)
		{
			print("[TCC4Ape] :  Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
				"after you declared your PluginData struct?");
			return Status::STATUS_ERROR;
		}

		return Status::STATUS_OK;
	}

	bool ScriptCompiler::initLocalMemory()
	{
		if (!globalData)
		{
			print("[TCC4Ape] : Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
				"after you declared your PluginData struct?");
			return Status::STATUS_ERROR;
		}

		if (globalData->wantsToSelfAlloc)
		{
			if (globalData->PluginAlloc && globalData->PluginFree)
			{
				pluginData = globalData->PluginAlloc(getProject()->iface);
				if (!globalData) 
				{
					print("[TCC4Ape] : error allocating memory for the sharedObject.");
					return false;
				}
				return true;
			}
			else
			{
				print("[TCC4Ape] : error allocating memory for the sharedObject: plugin specifies own allocators but they do not exist.");
				return false;
			}
		}

		pluginData = std::calloc(globalData->allocSize, 1);

		if (!pluginData)
		{
			print("[TCC4Ape] : error allocating memory for the plugin data object.");
			return false;
		}

		return true;
	}

	bool ScriptCompiler::freeLocalMemory()
	{
		if (!globalData)
		{
			print("[TCC4Ape] : Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
				"after you declared your PluginData struct?");
			return false;
		}

		if (!pluginData)
		{
			print("[TCC4Ape] : freeLocalMemory called without any plugin data");
			return false;
		}

		if (globalData->wantsToSelfAlloc)
		{
			if (globalData->PluginAlloc && globalData->PluginFree && pluginData)
			{
				globalData->PluginFree(pluginData);
				pluginData = nullptr;
				return true;
			}
			else
			{
				print("[TCC4Ape] : error freeing memory for the sharedObject.");
				return false;
			}
		}
		else if(pluginData)
		{
			std::free(pluginData);
			pluginData = nullptr;
			return true;
		}

		print("[TCC4Ape] : error freeing memory for the sharedObject.");
		return false;
	}


	Status ScriptCompiler::processReplacing(float ** in, float ** out, int frames)
	{
		return plugin.processor(pluginData, getProject()->iface, in, out, frames);
	}
	
	Status ScriptCompiler::activateProject()
	{
		if (!initLocalMemory())
			return Status::STATUS_ERROR;

		return plugin.entrypoint(pluginData, getProject()->iface); 
	}

	Status ScriptCompiler::disableProject()
	{
		auto ret = plugin.exitpoint(pluginData, getProject()->iface); 

		if (!freeLocalMemory())
			return Status::STATUS_ERROR;

		return ret;
	}

	Status ScriptCompiler::onEvent(Event * e)
	{
		if(plugin.hasEventHandler())
			return plugin.handler(pluginData, getProject()->iface, e); 

		return Status::STATUS_NOT_IMPLEMENTED;
	}
}
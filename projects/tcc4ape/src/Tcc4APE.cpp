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
		"Tcc4APE",
		cpl::Version::fromParts(0, 1, 0),
		"Janus Thorborg",
		"tccape",
		false,
		nullptr,
		""
	};
	#endif
};

ape::ProtoCompiler * CreateCompiler()
{
	return new TCC4Ape::ScriptCompiler();
}

void DeleteCompiler(ape::ProtoCompiler * toBeDeleted)
{
	delete toBeDeleted;
}

APE_Status CleanCompilerCache()
{
	return STATUS_OK;
}

namespace TCC4Ape
{

	ScriptCompiler::ScriptCompiler() { }

	ScriptCompiler::~ScriptCompiler() 
	{
		if (pluginData && !freeLocalMemory())
		{
			print(APE_Diag_Error, "[TCC4Ape] : Leaked memory on script compiler destruction, unable to free!");
		}
		
	}

	Status ScriptCompiler::compileProject()
	{
		const TCCBindings::CompilerAccess compiler;

		if (!compiler.isLinked())
		{
			print(APE_Diag_Error, "[TCC4Ape] : Error linking against TCC, module is either not found or invalid.");
			return Status::STATUS_ERROR;
		}

		state = UniqueTCC(compiler.createState());

		if(!state) 
		{
			print(APE_Diag_Error, "[TCC4Ape] : Error allocating state for TCC compiler");
			return Status::STATUS_ERROR;
		}

		std::string dir = cpl::Misc::DirectoryPath();
		compiler.setLibPath(state.get(), dir.c_str());

		if (getProject()->arguments)
			compiler.setOptions(state.get(), getProject()->arguments);

		compiler.addIncludePath(state.get(), (std::string(getProject()->rootPath) + "/includes").c_str());
		compiler.addIncludePath(state.get(), (std::string(getProject()->rootPath) + "/includes/tcc").c_str());
		compiler.setOutputType(state.get(), TCC_OUTPUT_MEMORY);
		compiler.setErrorFunc(
			state.get(), 
			this, 
			[](void* opaque, const char* msg)
			{
				auto* compiler = static_cast<ScriptCompiler*>(opaque);
				compiler->print(APE_Diag_CompilationError, msg);
			}
		);

		//	TCC has the nice feature of setting size_t to unsigned int even on 64-bit.
		//	Lets fix that.

		#ifdef CPL_M_64_BIT
			compiler.defineSymbol(state, "__SIZE_TYPE__", "unsigned long long");
		#endif

		if(!getProject()->isSingleString) 
		{
			print(APE_Diag_CompilationError, "[TCC4Ape] : Error - Compiler only supports compiling single strings (one file).");
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
			print(APE_Diag_CompilationError, "[TCC4Ape] : Error relocating compiled plugin.");
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
			print(APE_Diag_CompilationError, "[TCC4Ape] : Not all functions required to run plugin was found.");
			return Status::STATUS_ERROR;
		}

		if(!compiler.getSymbol(state.get(), "check"))
		{
			print(APE_Diag_CompilationError, "[TCC4Ape] : Unable to find certain symbols, did you forget to include \"CInterface.h\"?");
			return Status::STATUS_ERROR;
		}

		globalData = reinterpret_cast<PluginGlobalData *>(compiler.getSymbol(state.get(), "_global_data"));
		if(!globalData)
		{
			print(APE_Diag_CompilationError, "[TCC4Ape] :  Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
				"after you declared your PluginData struct?");
			return Status::STATUS_ERROR;
		}

		return Status::STATUS_OK;
	}

	bool ScriptCompiler::initLocalMemory()
	{
		if (!globalData)
		{
			print(APE_Diag_CompilationError, "[TCC4Ape] : Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
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
					print(APE_Diag_Error, "[TCC4Ape] : error allocating memory for the sharedObject.");
					return false;
				}
				return true;
			}
			else
			{
				print(APE_Diag_Error, "[TCC4Ape] : error allocating memory for the sharedObject: plugin specifies own allocators but they do not exist.");
				return false;
			}
		}

		pluginData = std::calloc(globalData->allocSize, 1);

		if (!pluginData)
		{
			print(APE_Diag_Error, "[TCC4Ape] : error allocating memory for the plugin data object.");
			return false;
		}

		return true;
	}

	bool ScriptCompiler::freeLocalMemory()
	{
		if (!globalData)
		{
			print(APE_Diag_Error, "[TCC4Ape] : Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
				"after you declared your PluginData struct?");
			return false;
		}

		if (!pluginData)
		{
			print(APE_Diag_Error, "[TCC4Ape] : freeLocalMemory called without any plugin data");
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
				print(APE_Diag_Error, "[TCC4Ape] : error freeing memory for the sharedObject.");
				return false;
			}
		}
		else if(pluginData)
		{
			std::free(pluginData);
			pluginData = nullptr;
			return true;
		}

		print(APE_Diag_Error, "[TCC4Ape] : error freeing memory for the sharedObject.");
		return false;
	}


	Status ScriptCompiler::processReplacing(const float * const * in, float * const * out, std::size_t frames)
	{
		return plugin.processor(pluginData, getProject()->iface, in, out, frames);
	}
	
	Status ScriptCompiler::activateProject()
	{
		if (!initLocalMemory())
			return Status::STATUS_ERROR;

		return plugin.entrypoint(pluginData, getProject()->iface); 
	}

	Status ScriptCompiler::disableProject(bool didMisbehave)
	{
		Status ret = Status::STATUS_OK;
		if(!didMisbehave)
			ret = plugin.exitpoint(pluginData, getProject()->iface); 

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
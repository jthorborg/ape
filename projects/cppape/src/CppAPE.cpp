/*************************************************************************************

	C++ compiler for Audio Programming Environment. 

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

	file:CppAPE.cpp
		
		Implementation of the compiler class.

*************************************************************************************/

#include "CppAPE.h"
#include <libtcc.h>
#include <string>
#include <cpl/Misc.h>
#include <cpl/Common.h>
#include "TranslationUnit.h"

namespace cpl
{
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

};

APE::ProtoCompiler * CreateCompiler()
{
	return new CppAPE::ScriptCompiler();
}

void DeleteCompiler(APE::ProtoCompiler * toBeDeleted)
{
	delete toBeDeleted;
}

namespace CppAPE
{

	static const cpl::Args sizeTypeDefines = []()
	{
		cpl::Args ret;
		#ifdef CPL_M_64BIT

			#ifdef CPL_WINDOWS
				// Windows 64 bit
				ret
					.argPair("-D", "__SIZE_TYPE__=\"unsigned long long\"")
					.argPair("-D", "__PTRDIFF_TYPE__=\"signed long long\"")
					.argPair("-D", "__WINT_TYPE__=\"unsigned short\"")
					.argPair("-D", "__WCHAR_TYPE__=\"short\"");

			#else
				// Unix LP 64 bit
				ret.
					.argPair("-D", "__LP64__=1")
					.argPair("-D", "__SIZE_TYPE__=\"unsigned long\"")
					.argPair("-D", "__PTRDIFF_TYPE__=\"signed long\"")
					.argPair("-D", "__WINT_TYPE__=\"int\"")
					.argPair("-D", "__WCHAR_TYPE__=\"int\"");


			#endif

		#else

			#ifdef CPL_WINDOWS
				// Windows 32 bit
				ret
					.argPair("-D", "__SIZE_TYPE__=\"unsigned int\"")
					.argPair("-D", "__PTRDIFF_TYPE__=\"int\"")
					.argPair("-D", "__WINT_TYPE__=\"unsigned short\"")
					.argPair("-D", "__WCHAR_TYPE__=\"short\"");
			#else
				// Unix LP 32 bit
				ret
					.argPair("-D", "__LP64__=1")
					.argPair("-D", "__SIZE_TYPE__=\"unsigned int\"")
					.argPair("-D", "__PTRDIFF_TYPE__=\"int\"")
					.argPair("-D", "__WINT_TYPE__=\"int\"")
					.argPair("-D", "__WCHAR_TYPE__=\"int\"");
			#endif
		#endif

		return ret;
	}();

	ScriptCompiler::ScriptCompiler() { }

	ScriptCompiler::~ScriptCompiler() 
	{
		if (pluginData && !freeLocalMemory())
		{
			print("[CppAPE] : Leaked memory on script compiler destruction, unable to free!");
		}
		
	}

	Status ScriptCompiler::compileProject()
	{
		const TCCBindings::CompilerAccess compiler;

		if (!compiler.isLinked())
		{
			print("[CppAPE] : Error linking against TCC, module is either not found or invalid.");
			return Status::STATUS_ERROR;
		}

		// TODO: More state.
		if(!SetupEnvironment())
		{
			print("[CppAPE] : Error setting up environment.");
			//return Status::STATUS_ERROR;
		}

		state = UniqueTCC(compiler.createState());

		if(!state) 
		{
			print("[CppAPE] : Error allocating state for TCC compiler");
			return Status::STATUS_ERROR;
		}

		std::string dir = cpl::Misc::DirectoryPath();
		compiler.setLibPath(state.get(), dir.c_str());

		if (getProject()->arguments)
			compiler.setOptions(state.get(), getProject()->arguments);

		compiler.setOutputType(state.get(), TCC_OUTPUT_MEMORY);
		compiler.setErrorFunc(state.get(), getErrorFuncDetails().first, getErrorFuncDetails().second);


		//	TCC has the nice feature of setting size_t to unsigned int even on 64-bit.
		//	Lets fix that.

		#ifdef CPL_M_64_BIT
			compiler.defineSymbol(state, "__SIZE_TYPE__", "unsigned long long");
		#endif

		if(!getProject()->isSingleString) 
		{
			print("[CppAPE] : Error - Compiler only supports compiling single strings (one file).");
			return Status::STATUS_ERROR;
		}

		try
		{
			auto unit = TranslationUnit::FromSource(getProject()->sourceString, getProject()->files[0]);
			auto root = fs::path(getProject()->rootPath);
			unit.includeDirs({(root / "includes").string(), (root / "includes" / "tcc").string()});
			unit.preArgs(sizeTypeDefines);

			if (!unit.translate())
			{
				print(unit.getError().c_str());
				return Status::STATUS_ERROR;
			}

			if(!compiler.compileString(state.get(), unit.getTranslation().c_str()))
				return Status::STATUS_ERROR;

		}
		catch (const std::exception& e)
		{
			print((std::string("Exception while compiling: ") + e.what()).c_str());
			return Status::STATUS_ERROR;
		}
		


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

		if (!compiler.relocate(state.get(), TCC_RELOCATE_AUTO))
		{
			print("[CppAPE] : Error relocating compiled plugin.");
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
			print("[CppAPE] : Not all functions required to run plugin was found.");
			return Status::STATUS_ERROR;
		}

		if(!compiler.getSymbol(state.get(), "check"))
		{
			print("[CppAPE] : Unable to find certain symbols, did you forget to include \"CInterface.h\"?");
			return Status::STATUS_ERROR;
		}

		globalData = reinterpret_cast<PluginGlobalData *>(compiler.getSymbol(state.get(), "_global_data"));
		if(!globalData)
		{
			print("[CppAPE] :  Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
				"after you declared your PluginData struct?");
			return Status::STATUS_ERROR;
		}

		#ifdef _DEBUG
		char buf[8192];
		sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p", SYMBOL_INIT, compiler.getSymbol(state.get(), SYMBOL_INIT));
		print(buf);
		sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p", SYMBOL_END, compiler.getSymbol(state.get(), SYMBOL_END));
		print(buf);
		sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p", SYMBOL_PROCESS_REPLACE, compiler.getSymbol(state.get(), SYMBOL_PROCESS_REPLACE));
		print(buf);
		sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p", SYMBOL_EVENT_HANDLER, compiler.getSymbol(state.get(), SYMBOL_EVENT_HANDLER));
		print(buf);


		#endif

		return Status::STATUS_OK;
	}

	bool ScriptCompiler::initLocalMemory()
	{
		if (!globalData)
		{
			print("[CppAPE] : Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
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
					print("[CppAPE] : error allocating memory for the sharedObject.");
					return false;
				}
				return true;
			}
			else
			{
				print("[CppAPE] : error allocating memory for the sharedObject: plugin specifies own allocators but they do not exist.");
				return false;
			}
		}

		pluginData = std::calloc(globalData->allocSize, 1);

		if (!pluginData)
		{
			print("[CppAPE] : error allocating memory for the plugin data object.");
			return false;
		}

		return true;
	}

	bool ScriptCompiler::freeLocalMemory()
	{
		if (!globalData)
		{
			print("[CppAPE] : Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
				"after you declared your PluginData struct?");
			return false;
		}

		if (!pluginData)
		{
			print("[CppAPE] : freeLocalMemory called without any plugin data");
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
				print("[CppAPE] : error freeing memory for the sharedObject.");
				return false;
			}
		}
		else if(pluginData)
		{
			std::free(pluginData);
			pluginData = nullptr;
			return true;
		}

		print("[CppAPE] : error freeing memory for the sharedObject.");
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
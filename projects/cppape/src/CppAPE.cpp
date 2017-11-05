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
#include <cpl/CExclusiveFile.h>
#include "TranslationUnit.h"

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
	return new CppAPE::ScriptCompiler();
}

void DeleteCompiler(APE::ProtoCompiler * toBeDeleted)
{
	delete toBeDeleted;
}

namespace CppAPE
{

	const cpl::Args ScriptCompiler::sizeTypeDefines = []()
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

	const TranslationUnit::CommonOptions& ScriptCompiler::userTranslationOptions()
	{
		static const TranslationUnit::CommonOptions translationOptions(fs::path("build"), fs::path("build") / "szal.gen.h");

		return translationOptions;
	}

	ScriptCompiler::ScriptCompiler() {}

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

		fs::path dirRoot = cpl::Misc::DirectoryPath();

		if (!compiler.isLinked())
		{
			print("[CppAPE] : Error linking against TCC, module is either not found or invalid.");
			return Status::STATUS_ERROR;
		}

		cpl::CExclusiveFile lockFile;

		if (!lockFile.open((dirRoot / "lockfile.l").string()))
		{
			print("[CppAPE] : error: couldn't lock file");
			return Status::STATUS_ERROR;
		}

		// TODO: More state.
		if(!SetupEnvironment())
		{
			print("[CppAPE] : Error setting up environment.");
			return Status::STATUS_ERROR;
		}
		
		userTranslationOptions().clean();

		state = UniqueTCC(compiler.createState());

		if(!state) 
		{
			print("[CppAPE] : Error allocating state for TCC compiler");
			return Status::STATUS_ERROR;
		}

		compiler.setLibPath(state.get(), dirRoot.string().c_str());

		if (getProject()->arguments)
			compiler.setOptions(state.get(), getProject()->arguments);

		compiler.setOutputType(state.get(), TCC_OUTPUT_MEMORY);
		compiler.setErrorFunc(state.get(), getErrorFuncDetails().first, getErrorFuncDetails().second);


		if(!getProject()->isSingleString) 
		{
			print("[CppAPE] : Error - Compiler only supports compiling single strings (one file).");
			return Status::STATUS_ERROR;
		}

		try
		{
			auto unit = TranslationUnit::FromSource(getProject()->sourceString, getProject()->files[0]);
			auto root = fs::path(getProject()->rootPath);

			unit
				.includeDirs({(root / "includes").string(), (root / "includes" / "tcc").string()})
				.preArgs(sizeTypeDefines)
				.options(userTranslationOptions())
				.addSource(dirRoot / "build" / "postfix.cpp");

			auto result = unit.translate();

			if(unit.getError().size())
				print(unit.getError().c_str());

			if (!result)
			{
				return Status::STATUS_ERROR;
			}

			userTranslationOptions().ensureCreated();

			if(!compiler.compileString(state.get(), unit.getTranslation().c_str()))
				return Status::STATUS_ERROR;

			// ugly hack to prevent message of ODR of vtables violation in runtime.o....
			compiler.setErrorFunc(state.get(), nullptr, nullptr);

			if (!compiler.addFile(state.get(), (dirRoot / "runtime" / "runtime.o").string().c_str()))
				return Status::STATUS_ERROR;

			compiler.setErrorFunc(state.get(), getErrorFuncDetails().first, getErrorFuncDetails().second);

			if (!compiler.addFile(state.get(), (dirRoot / "runtime" / "ctorsdtors.c").string().c_str()))
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

	RUNTIME_FUNCTION * ScriptCompiler::LookupRuntimeFunction(DWORD64 pc, void * context)
	{

		ScriptCompiler * c = (ScriptCompiler*)context;
		auto base = pc - (DWORD64)c->buffer.data();
		return {};
	}

	Status ScriptCompiler::initProject()
	{

		globalData = nullptr;
		const TCCBindings::CompilerAccess compiler;

		const auto size = compiler.relocate(state.get(), nullptr);
		if (size == -1)
			return STATUS_ERROR;

		buffer.resize(size);
		if (compiler.relocate(state.get(), buffer.data()) == -1)
		{
			print("[CppAPE] : Error relocating compiled plugin.");
			return Status::STATUS_ERROR;
		}

		RtlInstallFunctionTableCallback((DWORD64)buffer.data() | 0x3, (DWORD64)buffer.data(), buffer.size(), LookupRuntimeFunction, this, nullptr);


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

		/* if(!compiler.getSymbol(state.get(), "check"))
		{
			print("[CppAPE] : Unable to find certain symbols, did you forget to include \"CInterface.h\"?");
			return Status::STATUS_ERROR;
		} */

		globalData = reinterpret_cast<PluginGlobalData *>(compiler.getSymbol(state.get(), SYMBOL_GLOBAL_DATA));
		if(!globalData)
		{
			print("[CppAPE] :  Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
				"after you declared your PluginData struct?");
			return Status::STATUS_ERROR;
		}

		#ifdef _DEBUG
		char buf[8192];
		sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p\n -> 0x%p + 0x%llX", SYMBOL_INIT, compiler.getSymbol(state.get(), SYMBOL_INIT), buffer.data(), (byte*)compiler.getSymbol(state.get(), SYMBOL_INIT) - buffer.data());
		print(buf);
		sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p\n -> 0x%p + 0x%llX", SYMBOL_END, compiler.getSymbol(state.get(), SYMBOL_END), buffer.data(), (byte*)compiler.getSymbol(state.get(), SYMBOL_END) - buffer.data());
		print(buf);
		sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p\n -> 0x%p + 0x%llX", SYMBOL_PROCESS_REPLACE, compiler.getSymbol(state.get(), SYMBOL_PROCESS_REPLACE), buffer.data(), (byte*)compiler.getSymbol(state.get(), SYMBOL_PROCESS_REPLACE) - buffer.data());
		print(buf);
		sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p\n -> 0x%p + 0x%llX", SYMBOL_EVENT_HANDLER, compiler.getSymbol(state.get(), SYMBOL_EVENT_HANDLER), buffer.data(), (byte*)compiler.getSymbol(state.get(), SYMBOL_EVENT_HANDLER) - buffer.data());
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
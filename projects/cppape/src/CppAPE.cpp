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
#include <fstream>

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

ape::ProtoCompiler * CreateCompiler()
{
	return new CppAPE::ScriptCompiler();
}

void DeleteCompiler(ape::ProtoCompiler * toBeDeleted)
{
	delete toBeDeleted;
}

namespace CppAPE
{


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
	//	const TCCBindings::CompilerAccess compiler;

		fs::path dirRoot = cpl::Misc::DirectoryPath();

		/*if (!compiler.isLinked())
		{
			print("[CppAPE] : Error linking against TCC, module is either not found or invalid.");
			return Status::STATUS_ERROR;
		} */

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
		
		try
		{


			CxxTranslationUnit::Builder builder;
			builder.onMessage([this](auto err, auto msg) { print(msg); });
			auto root = fs::path(getProject()->rootPath);

			std::string source;

			if (getProject()->isSingleString)
			{
				source += getProject()->sourceString;
			}

			std::string startingPlace;

			if (getProject()->workingDirectory)
			{
				startingPlace = getProject()->workingDirectory;
			}
			else if (getProject()->files && getProject()->files[0])
			{
				startingPlace = fs::path(getProject()->files[0]).parent_path().string();
			}

			if (startingPlace.size())
				builder.includeDirs({ startingPlace });

			builder.includeDirs({ 
				(root / "includes").string(), 
				/*(root / "includes" / "tcc").string(), */
				(root / "includes" / "libcxx").string(),
				(root / "includes" / "ccore").string(),

			});

			if (std::ifstream postfix((dirRoot / "build" / "postfix.cpp").string().c_str()); postfix.good())
			{
				std::string temp;
				while (std::getline(postfix, temp))
					source += temp + "\n";
			}

			builder.args()
				.arg("-v")
				//.arg("fno-short-wchar")
				.arg("-fms-extensions")
				.arg("-O2")
				//.arg("--stdlib=libc++")
				.arg("-D_LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS")
				.arg("-fcxx-exceptions")
				.argPair("-D__STDC_VERSION__=", "199901L", cpl::Args::NoSpace)
				.argPair("-std=", "c++17", cpl::Args::NoSpace);


			auto projectUnit = builder.fromString(source);
#ifdef _DEBUG
			projectUnit.save((dirRoot / "build" / "compiled_source.ll").string().c_str());
#endif
			auto runtimeUnit = CxxTranslationUnit::loadSaved((dirRoot / "runtime" / "runtime.ll").string());

			state = std::make_unique<CxxJitContext>();
			state->setCallback([this](auto err, auto msg) { print(msg); });
			state->addTranslationUnit(projectUnit);
			state->addTranslationUnit(runtimeUnit);

		}
		catch (const std::exception& e)
		{
			print(std::string("Exception while compiling: ") + e.what());
			return Status::STATUS_ERROR;
		}



		/*userTranslationOptions().clean();

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

			compiler.addIncludePath(state.get(), (root / "includes" / "tcc").string().c_str());

			if (!compiler.compileString(state.get(), "#include <math.h>\n"))
				return Status::STATUS_ERROR;

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
		}*/

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

		if (!state)
		{
			print("[CppAPE] : No existing jit context.");
			return Status::STATUS_ERROR;
		}

		try
		{
			state->injectSymbol<void>("??_7type_info@@6B@", (void*)0x213);
			state->finalize();

			plugin.entrypoint = state->getFunction<APE_Init>(SYMBOL_INIT);
			plugin.exitpoint = state->getFunction<APE_End>(SYMBOL_END);
			plugin.processor = state->getFunction<APE_ProcessReplacer>(SYMBOL_PROCESS_REPLACE);
			plugin.handler = state->getFunction<APE_EventHandler>(SYMBOL_EVENT_HANDLER);

			if (!plugin.test(true))
			{
				print("[CppAPE] : Not all functions required to run plugin was found.");
				return Status::STATUS_ERROR;
			}

			globalData = state->getGlobal<PluginGlobalData>(SYMBOL_GLOBAL_DATA);
			if(!globalData)
			{
				print("[CppAPE] :  Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
					"after you declared your PluginData struct?");
				return Status::STATUS_ERROR;
			}

			#ifdef _DEBUG
				char buf[8192];
				sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p", SYMBOL_INIT, plugin.entrypoint);
				print(buf);
				sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p", SYMBOL_END, plugin.exitpoint);
				print(buf);
				sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p", SYMBOL_PROCESS_REPLACE, plugin.processor);
				print(buf);
				sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p", SYMBOL_EVENT_HANDLER, plugin.handler);
				print(buf);


			#endif

		}
		catch (const std::exception& e)
		{
			print((std::string)"[CppApe] : error initializing project: " + e.what());
			return Status::STATUS_ERROR;
		}




		/* if(!compiler.getSymbol(state.get(), "check"))
		{
			print("[CppAPE] : Unable to find certain symbols, did you forget to include \"CInterface.h\"?");
			return Status::STATUS_ERROR;
		} */



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

		volatile auto localPluginData = pluginData;
		pluginData = nullptr;

		if (globalData->wantsToSelfAlloc)
		{
			if (globalData->PluginAlloc && globalData->PluginFree && localPluginData)
			{
				globalData->PluginFree(localPluginData);
				return true;
			}
			else
			{
				print("[CppAPE] : error freeing memory for the sharedObject.");
				return false;
			}
		}
		else if(localPluginData)
		{
			std::free(localPluginData);
			return true;
		}

		print("[CppAPE] : error freeing memory for the sharedObject.");
		return false;
	}


	Status ScriptCompiler::processReplacing(const float * const * in, float * const * out, size_t frames)
	{
		return (*plugin.processor)(pluginData, getProject()->iface, in, out, frames);
	}
	
	Status ScriptCompiler::activateProject()
	{
		try
		{
			state->openRuntime();

			if (!initLocalMemory())
				return Status::STATUS_ERROR;
			return (*plugin.entrypoint)(pluginData, getProject()->iface);
		}
		catch (const LibCppJitExceptionBase& e)
		{
			print((std::string)"JIT Error while activing project: \n" + e.what());
		}
		catch (const std::exception& e)
		{
			print((std::string)"Unknown error while activating project: \n" + e.what());
		}

		return Status::STATUS_ERROR;

	}

	Status ScriptCompiler::disableProject(bool didMisbehave)
	{
		Status ret = Status::STATUS_OK;

		try
		{
			
			if (!didMisbehave)
			{
				ret = (*plugin.exitpoint)(pluginData, getProject()->iface);

				if (!freeLocalMemory())
					return Status::STATUS_ERROR;
			}
		
			pluginData = nullptr;

			// TODO: Always call?
			state->closeRuntime();

		}
		catch (const LibCppJitExceptionBase& e)
		{
			print((std::string)"JIT Error while disabling project: \n" + e.what());
		}
		catch (const std::exception& e)
		{
			print((std::string)"Unknown error while disabling project: \n" + e.what());
		}

		return ret;
	}

	Status ScriptCompiler::onEvent(Event * e)
	{
		if(plugin.hasEventHandler())
			return (*plugin.handler)(pluginData, getProject()->iface, e);

		return Status::STATUS_NOT_IMPLEMENTED;
	}
}
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
#include <string>
#include <cpl/Misc.h>
#include <cpl/Common.h>
#include <cpl/CExclusiveFile.h>
#include "TranslationUnit.h"
#include <fstream>
#include <sstream>

namespace cpl
{
	#ifndef APE_TESTS
	const ProgramInfo programInfo
	{
		"CppAPE",
		cpl::Version::fromParts(0, 1, 0),
		"Janus Thorborg",
		"cppape",
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

APE_Status CleanCompilerCache()
{
	using namespace CppAPE;

	fs::path dirRoot = cpl::Misc::DirectoryPath();

	cpl::CExclusiveFile lockFile;

	if (!lockFile.open((dirRoot / "lockfile.l").string()))
	{
		return Status::STATUS_ERROR;
	}

	if (fs::exists(dirRoot / "runtime" / "common.h.pch") && !fs::remove(dirRoot / "runtime" / "common.h.pch"))
		return Status::STATUS_ERROR;

	return Status::STATUS_OK;
}

namespace CppAPE
{
	const std::vector<const char*> ScriptCompiler::defines = {
		"__cppape",
        "_LIBCPP_BUILDING_HAS_NO_ABI_LIBRARY"
	};

	APE_Diagnostic JitToDiagnostic(jit_error_t error)
	{
		switch (error)
		{
		case jit_error_compilation_note:
		case jit_error_compilation_remark: return APE_Diag_Info;
		case jit_error_compilation_warning: return APE_Diag_Warning;
		case jit_error_compilation_error: return APE_Diag_CompilationError;
		default:
			return APE_Diag_Error;
		}
	}

	std::shared_ptr<CxxJitContext> ScriptCompiler::acquireCxxRuntime()
	{
		static std::shared_ptr<CxxJitContext> cxxRuntime;
		return cxxRuntime;
	}


	ScriptCompiler::ScriptCompiler() {}

	ScriptCompiler::~ScriptCompiler() 
	{
		if (pluginData && !freeLocalMemory())
		{
			print(APE_Diag_Error, "[CppAPE] : Leaked memory on script compiler destruction, unable to free!");
		}
		
	}

	Status ScriptCompiler::compileProject()
	{
		fs::path dirRoot = cpl::Misc::DirectoryPath();

		cpl::CExclusiveFile lockFile;

        auto lockFilePath = (dirRoot / "lockfile.l").string();
        
		if (!lockFile.open(lockFilePath.c_str()))
		{
            print(APE_Diag_Error, cpl::format("[CppAPE] : error: couldn't lock file at: %s", lockFilePath.c_str()).c_str());
			return Status::STATUS_ERROR;
		}

		if(!SetupEnvironment())
		{
			print(APE_Diag_Error, "[CppAPE] : Error setting up environment.");
			return Status::STATUS_ERROR;
		}

		// TODO: Cache static
		if (memoryEffectPCH.empty())
		{
			std::ifstream pchfile(
                (dirRoot / "runtime" / "common.h.pch").string().c_str(),
                std::ios::binary | std::ios::ate
            );
			std::streamsize size = pchfile.tellg();
			pchfile.seekg(0, std::ios::beg);

			memoryEffectPCH.resize(size);
			if (size == 0 || size == -1 || !pchfile.read(memoryEffectPCH.data(), size))
			{
				print(APE_Diag_Error, "[CppAPE] : Failed to read pchfile into memory");
				return Status::STATUS_ERROR;
			}
		}

		try
		{
			CxxTranslationUnit::Builder builder;
			builder.onMessage(
				[this](auto e, auto msg) 
				{
					print(JitToDiagnostic(e), msg); 
				}
			);

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

			if (!transformSource(*getProject(), source))
				return Status::STATUS_ERROR;

			builder.args()
				//.arg("fno-short-wchar")
				.arg("-fms-extensions")
				.arg("-O2")
				//.arg("--stdlib=libc++")
				.arg("-D_LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS")
				.arg("-fexceptions")
				.arg("-fcxx-exceptions")
#ifdef CPL_MAC
				.arg("-fno-use-cxa-atexit")
#endif
				.argPair("-D_LIBCPP_DEBUG=", "0", cpl::Args::NoSpace)
				.argPair("-D__CPPAPE_PRECISION__=", std::to_string(getProject()->floatPrecision), cpl::Args::NoSpace)
				.argPair("-D__CPPAPE_NATIVE_VECTOR_BIT_WIDTH__=", std::to_string(getProject()->nativeVectorBitWidth), cpl::Args::NoSpace)
				.argPair("-D__STDC_VERSION__=", "199901L", cpl::Args::NoSpace)
				.argPair("-std=", "c++17", cpl::Args::NoSpace)
				.argPair("-include-pch", (dirRoot / "runtime" / "common.h.pch").string());

			if (getProject()->numTraceLines > 0)
				builder.args().argPair("-D", "CPPAPE_TRACING_ENABLED", cpl::Args::NoSpace);

			for(auto define : defines)
				builder.args().argPair("-D", define, cpl::Args::NoSpace);

			builder.addMemoryFile("common.h.pch", memoryEffectPCH.data(), memoryEffectPCH.size());

			state = std::make_unique<CxxJitContext>(0, false);
			state->setCallback(
				[this](auto e, auto msg)
				{
					print(JitToDiagnostic(e), msg);
				}
			);

			state->injectSymbol("??_7type_info@@6B@", (*(void**)&typeid(*this)));
			state->injectSymbol("snprintf", std::snprintf);

			auto projectUnit = builder.fromString(source, getProject()->projectName, state.get());
			auto tasks = builder.fromFile((dirRoot / "runtime" / "misc_tasks.cpp").string());
			auto runtimeUnit = CxxTranslationUnit::loadSaved((dirRoot / "runtime" / "runtime.bc").string(), state.get());
			auto libraryUnit = CxxTranslationUnit::loadSaved((dirRoot / "runtime" / "libcxx.bc").string(), state.get());

#if defined(_DEBUG) || defined(DEBUG)
			projectUnit.save((dirRoot / "build" / "compiled_source.bc").string().c_str());
			tasks.save((dirRoot / "build" / "tasks.bc").string().c_str());
#endif
			tasks.addDependencyOn(runtimeUnit);
			projectUnit.addDependencyOn(tasks);
			projectUnit.addDependencyOn(libraryUnit);
			projectUnit.addDependencyOn(runtimeUnit);
			libraryUnit.addDependencyOn(runtimeUnit);
			runtimeUnit.addDependencyOn(tasks);

			state->addTranslationUnit(projectUnit);
			state->addTranslationUnit(tasks);
			state->addTranslationUnit(runtimeUnit);
			state->addTranslationUnit(libraryUnit);
		}
		catch (const std::exception& e)
		{
			print(APE_Diag_Error, std::string("Exception while compiling: ") + e.what());
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

		if (!state)
		{
			print(APE_Diag_Error, "[CppAPE] : No existing jit context.");
			return Status::STATUS_ERROR;
		}

		try
		{
			state->finalize();
			state->prepareGlobals();

			plugin.entrypoint = state->getFunction<APE_Init>(SYMBOL_INIT);
			plugin.exitpoint = state->getFunction<APE_End>(SYMBOL_END);
			plugin.processor = state->getFunction<APE_ProcessReplacer>(SYMBOL_PROCESS_REPLACE);
			plugin.handler = state->getFunction<APE_EventHandler>(SYMBOL_EVENT_HANDLER);

			if (!plugin.test(true))
			{
				print(APE_Diag_CompilationError, "[CppAPE] : Not all functions required to run plugin was found.");
				return Status::STATUS_ERROR;
			}

			globalData = state->getGlobal<PluginGlobalData>(SYMBOL_GLOBAL_DATA);
			if(!globalData)
			{
				print(APE_Diag_CompilationError, "[CppAPE] :  Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
					"after you declared your PluginData struct?");
				return Status::STATUS_ERROR;
			}

			#ifdef _DEBUG
				char buf[8192];
				sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p", SYMBOL_INIT, plugin.entrypoint);
				print(APE_Diag_Info, buf);
				sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p", SYMBOL_END, plugin.exitpoint);
				print(APE_Diag_Info, buf);
				sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p", SYMBOL_PROCESS_REPLACE, plugin.processor);
				print(APE_Diag_Info, buf);
				sprintf_s(buf, "[CppApe] symbol \"%s\" loaded at 0x%p", SYMBOL_EVENT_HANDLER, plugin.handler);
				print(APE_Diag_Info, buf);


			#endif

		}
		catch (const std::exception& e)
		{
			print(APE_Diag_Error, (std::string)"[CppApe] : error initializing project: " + e.what());
			return Status::STATUS_ERROR;
		}

		return Status::STATUS_OK;
	}

	bool ScriptCompiler::initLocalMemory()
	{
		if (!globalData)
		{
			print(APE_Diag_CompilationError, "[CppAPE] : Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
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
					print(APE_Diag_Error, "[CppAPE] : error allocating memory for the sharedObject.");
					return false;
				}
				return true;
			}
			else
			{
				print(APE_Diag_Error, "[CppAPE] : error allocating memory for the sharedObject: plugin specifies own allocators but they do not exist.");
				return false;
			}
		}

		pluginData = std::calloc(globalData->allocSize, 1);

		if (!pluginData)
		{
			print(APE_Diag_Error, "[CppAPE] : error allocating memory for the plugin data object.");
			return false;
		}

		return true;
	}

	bool ScriptCompiler::freeLocalMemory()
	{
		if (!globalData)
		{
			print(APE_Diag_Error, "[CppAPE] : Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
				"after you declared your PluginData struct?");
			return false;
		}

		if (!pluginData)
		{
			print(APE_Diag_Error, "[CppAPE] : freeLocalMemory called without any plugin data");
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
				print(APE_Diag_Error, "[CppAPE] : error freeing memory for the sharedObject.");
				return false;
			}
		}
		else if(localPluginData)
		{
			std::free(localPluginData);
			return true;
		}

		print(APE_Diag_Error, "[CppAPE] : error freeing memory for the sharedObject.");
		return false;
	}

	bool ScriptCompiler::transformSource(const Project& project, std::string& source)
	{
		if (project.traceLines == nullptr || project.numTraceLines == 0)
			return true;

		std::string result = "#include <trace.h>\n";
		result.reserve(source.size());
		int lineCounter = 0;
		std::string line;

		const auto traceBegin = project.traceLines;
		const auto traceEnd = project.traceLines + project.numTraceLines;

		for (std::size_t i = 0; i < source.size(); ++i)
		{
			switch (auto c = source[i])
			{
				case '\r':
				{
					if (i + 1 < source.size() && source[i + 1] == '\n')
						++i;
				}
				case '\n':
				{
					if (std::find(traceBegin, traceEnd, lineCounter) != traceEnd)
					{
						auto terminal = line.find_first_of(';');

						if (terminal == std::string::npos)
						{
							print(APE_Diag_CompilationError, 
								"[CppApe] error: Breakpoints can only be set at single statements, at line " + 
								std::to_string(lineCounter + 1) + ":\n " + line
							);
							return false;
						}

						result += "TRC(" + line.substr(0, terminal) + ")" + line.substr(terminal) + "\n";
					}
					else
					{
						result += line + "\n";
					}

					line.resize(0);

					lineCounter++;
					break;
				}
				default:
					line += c;
			}

		}

		source = std::move(result);

		return true;
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
			print(APE_Diag_Error, (std::string)"JIT Error while activing project: \n" + e.what());
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
			print(APE_Diag_Error, (std::string)"JIT Error while disabling project: \n" + e.what());
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

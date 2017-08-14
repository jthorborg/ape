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

namespace TCC4Ape
{
	TCC CCompiler::tcc;


	bool TCC::initializeBindings()
	{
		if (!isInitialized)
		{
			/*
				we use runtime binding on windows, and static linkage on mac.
				why? to get rid of delay loading on windows and problems with 
				search paths.
				on osx, we statically link to the library, so it shouldn't be a problem
			*/
			#ifdef TCC4APE_STATIC_LINK
				newState = tcc_new;
				setLibPath = tcc_set_lib_path;
				addIncludePath = tcc_add_include_path;
				setOutputType = tcc_set_output_type;
				setErrorFunc = tcc_set_error_func;
				compileString = tcc_compile_string;
				deleteState = tcc_delete;
				relocate = tcc_relocate;
				getSymbol = tcc_get_symbol;
				defineSymbol = tcc_define_symbol;
				setOptions = tcc_set_options;
			#else
				if (!module.load(DirectoryPath + "\\libtcc.dll"))
				{
					newState = (decltype(newState)) module.getFuncAddress("tcc_new");
					setLibPath = (decltype(setLibPath))module.getFuncAddress("tcc_set_lib_path");
					addIncludePath = (decltype(addIncludePath))module.getFuncAddress("tcc_add_include_path");
					setOutputType = (decltype(setOutputType))module.getFuncAddress("tcc_set_output_type");
					setErrorFunc = (decltype(setErrorFunc))module.getFuncAddress("tcc_set_error_func");
					compileString = (decltype(compileString))module.getFuncAddress("tcc_compile_string");
					deleteState = (decltype(deleteState))module.getFuncAddress("tcc_delete");
					relocate = (decltype(relocate))module.getFuncAddress("tcc_relocate");
					getSymbol = (decltype(getSymbol))module.getFuncAddress("tcc_get_symbol");
					defineSymbol = (decltype(defineSymbol))module.getFuncAddress("tcc_define_symbol");
					setOptions = (decltype(setOptions))module.getFuncAddress("tcc_set_options");
				}
			#endif

			isInitialized = true;
		}
		if (isInitialized)
		{
			// test whether ALL functions pointers are valid.
			auto test = newState && setLibPath && addIncludePath && setOutputType && setErrorFunc
				&& compileString && deleteState && relocate && getSymbol && defineSymbol && setOptions;
			return test;
		}
		else
			return false;
	}


	CCompiler::CCompiler()
		: project(nullptr), state(nullptr), errorFunc(nullptr), op(nullptr)
	{
	}
	CCompiler::~CCompiler()
	{
	}
	void CCompiler::print(const char * s)
	{
		if(op && errorFunc)
			errorFunc(op, s);
	}


	Status CCompiler::compileProject()
	{
		if (!tcc.initializeBindings())
		{
			print("[TCC4Ape] : Error linking against TCC, module is either not found or invalid.");
			return Status::STATUS_ERROR;
		}
		state = tcc.newState();
		if(!state) {
			print("[TCC4Ape] : Error allocating state for TCC compiler");
			return Status::STATUS_ERROR;
		}
		std::string dir = cpl::Misc::DirectoryPath();
		tcc.setLibPath(state, dir.c_str());

		if (project->arguments)
			tcc.setOptions(state, project->arguments);

		tcc.addIncludePath(state, (std::string(project->rootPath) + "/includes").c_str());
		tcc.setOutputType(state, TCC_OUTPUT_MEMORY);
		tcc.setErrorFunc(state, op, errorFunc);

		/*
			TCC has the nice feature of setting size_t to unsigned int even on 64-bit.
			Lets fix that.
		*/
		#ifdef __M_64BIT_
			tcc.defineSymbol(state, "__SIZE_TYPE__", "unsigned long long");
		#endif
		if(!project->isSingleString) 
		{
			print("[TCC4Ape] : Error - Compiler only supports compiling single strings (one file).");
			return Status::STATUS_ERROR;
		}
		if(project->sourceString && tcc.compileString(state, project->sourceString) == -1)
			return Status::STATUS_ERROR;
		return Status::STATUS_OK;
	}
	Status CCompiler::setErrorFunc(void * op, errorFunc_t e)
	{
		this->op = op;
		errorFunc = e;
		return Status::STATUS_OK;
	}
	Status CCompiler::releaseProject()
	{
		tcc.deleteState(state);
		return Status::STATUS_OK;
	}
	Status CCompiler::initProject()
	{
		int ret = tcc.relocate(state, TCC_RELOCATE_AUTO);
		if(ret < 0)
			return Status::STATUS_ERROR;

		plugin.entrypoint 
			= reinterpret_cast<decltype(plugin.entrypoint)>(tcc.getSymbol(state, SYMBOL_INIT));
		plugin.exitpoint 
			= reinterpret_cast<decltype(plugin.exitpoint)>(tcc.getSymbol(state, SYMBOL_END));
		plugin.processor 
			= reinterpret_cast<decltype(plugin.processor)>(tcc.getSymbol(state, SYMBOL_PROCESS_REPLACE));
		plugin.handler 
			= reinterpret_cast<decltype(plugin.handler)>(tcc.getSymbol(state, SYMBOL_EVENT_HANDLER));
		if(!plugin.test(true)) 
		{
			print("[TCC4Ape] : Not all functions required to run module was found.");
			return Status::STATUS_ERROR;
		}

		void * test = tcc.getSymbol(state, "check");
		if(!test) {
			print("[TCC4Ape] : Unable to find certain symbols, did you forget to include \"CInterface.h\"?");
			return Status::STATUS_ERROR;
		}

		_plugin_global_data * data = reinterpret_cast<_plugin_global_data *>(tcc.getSymbol(state, "_global_data"));
		if(!data) {
			print("[TCC4Ape] :  Unable to find certain global data, did you forget the line GlobalData(\"your plugin name\") "
				"after you declared your PluginData struct?");
			return Status::STATUS_ERROR;
		}

		this->global_data = data;

		return Status::STATUS_OK;
	}

	bool CCompiler::initLocalMemory()
	{
		if (global_data->selfAlloc)
		{
			if (global_data->palloc && global_data->pfree)
			{
				plugin_data = global_data->palloc(project->iface);
				if (!plugin_data) {
					print("[syswrap] : error allocating memory for the sharedObject.");
					return false;
				}
				return true;
			}
			else
			{
				print("[syswrap] : error allocating memory for the sharedObject: plugin specifies own allocators but they do not exist.");
				return false;
			}
		}
		plugin_data = ::malloc(global_data->allocSize);
		if (!plugin_data) {
			print("[syswrap] : error allocating memory for the sharedObject.");
			return false;
		}
		::memset(plugin_data, 0, global_data->allocSize);
		return true;
	}
	/*********************************************************************************************

	frees the block of memory create by initLocalMemory

	*********************************************************************************************/
	bool CCompiler::freeLocalMemory()
	{
		if (global_data->selfAlloc)
		{
			if (global_data->palloc && global_data->pfree && plugin_data)
			{
				global_data->pfree(plugin_data);
				return true;
			}
			else
			{
				print("[syswrap] : error freeing memory for the sharedObject.");
				return false;
			}
		}
		else
		{
			if (plugin_data) {
				::free(plugin_data);
				return true;
			}

		}
		print("[syswrap] : error freeing memory for the sharedObject.");
		return false;
	}


	Status CCompiler::processReplacing(float ** in, float ** out, int frames)
	{
		return plugin.processor(reinterpret_cast<PluginData*>(plugin_data), 
								reinterpret_cast<CSharedInterface*>(project->iface), 
								in, out, frames);
	}


	Status CCompiler::activateProject()
	{
		auto ret = initLocalMemory();
		if (!ret)
			return Status::STATUS_ERROR;
		return plugin.entrypoint(reinterpret_cast<PluginData*>(plugin_data), 
								reinterpret_cast<CSharedInterface*>(project->iface)); 
	}
	Status CCompiler::disableProject()
	{
		auto ret = plugin.exitpoint(reinterpret_cast<PluginData*>(plugin_data), 
								reinterpret_cast<CSharedInterface*>(project->iface)); 
		auto ret2 = freeLocalMemory();
		if (!ret2)
			return Status::STATUS_ERROR;
		return ret;
	}
	Status CCompiler::onEvent(CEvent * e)
	{
		if(plugin.hasEventHandler())
			return plugin.handler(reinterpret_cast<PluginData*>(plugin_data), 
								  reinterpret_cast<CSharedInterface*>(project->iface),
								  e); 
		return Status::STATUS_NOT_IMPLEMENTED;
	}
}
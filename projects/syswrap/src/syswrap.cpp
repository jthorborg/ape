/*************************************************************************************

	syswrap Compiler for Audio Programming Environment. 

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

	file:syswrap.cpp
		
		Implementation of the compiler

*************************************************************************************/

#include "syswrap.h"
#include <string>
#include <cpl/CModule.h>
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
}

namespace syswrap
{
	CCompiler::CCompiler()
		: project(nullptr),  errorFunc(nullptr), op(nullptr)
	{
		instanceNumber = instanceCount++;

	}
	CCompiler::~CCompiler()
	{

	}
	void CCompiler::print(const char * s)
	{
		if (op && errorFunc)
			errorFunc(op, s);
	}
	bool CCompiler::isLoaded = false;
	int CCompiler::instanceCount = 0;
	Status CCompiler::compileProject()
	{
		if (!isLoaded)
		{
			isLoaded = true;
			/*
				run environment and set up vars.
				This does not work as expected,
				ideally all vars in environment.bat
				should be preserved.
			*/
			print("syswrap compiler loaded.");
			auto ret = cpl::Misc::ExecCommand(cpl::Misc::DirectoryPath() + "/environment.bat");

			print(("env returned: " + std::to_string(ret.first)).c_str());

			if (ret.second.size())
			{
				print(ret.second.c_str());
			}

			if (ret.first != EXIT_SUCCESS)
			{
				print("syswrap: error setting up environment.");
				return Status::STATUS_ERROR;
			}
		}
		// syswrap needs files.
		if (project->isSingleString && !project->nFiles)
		{
			print("syswrap error: save your project firstly.");
			return Status::STATUS_ERROR;
		}
		// format command lines. [0] is programname, [1] is output name, [2] is a list of input files, [3] is compiler arguments
		std::string cmdLine = "\"" + cpl::Misc::DirectoryPath() + "/sysoutput" + std::to_string(instanceNumber) + ".dll\" ";
		std::string inFiles;

			inFiles = "\"";
			for (unsigned i = 0; i < project->nFiles; ++i)
			{

				inFiles += project->files[i];
				inFiles += " ";
			}

			inFiles[inFiles.size() - 1] = '\"';

		cmdLine += inFiles;
		if (project->arguments)
			cmdLine += " \"" + std::string(project->arguments) + "\"";
		// run build

		auto ret = cpl::Misc::ExecCommand(cpl::Misc::DirectoryPath() + "/build.bat " + cmdLine);

		print(ret.second.c_str());
		print(("%%errorlevel%%: " + std::to_string(ret.first)).c_str());

		if (ret.first != EXIT_SUCCESS)
			return Status::STATUS_ERROR;
		else
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
		module.release();
		// backslash is unfortunately needed here because of how batch script commands wont accept slashes
		std::string cmdLine = cpl::Misc::DirectoryPath() + "\\sysoutput" + std::to_string(instanceNumber);
		auto ret = cpl::Misc::ExecCommand(cpl::Misc::DirectoryPath() + "/cleanup.bat " + cmdLine);
		print(ret.second.c_str());
		return Status::STATUS_OK;
	}

	Status CCompiler::initProject()
	{
		// load compiled module

		char addr[50];

		auto ret = module.load(cpl::Misc::DirectoryPath() + "/sysoutput" + std::to_string(instanceNumber) + ".dll");

		sprintf_s(addr, "0x%X", module.getHandle());
		std::string loadMsg = "syswrap: loaded sysoutput.dll at ";
		loadMsg += addr;
		if (ret)
			print(("syswrap: error loading sysoutput.dll: " + std::to_string(ret)).c_str());
		else
			print(loadMsg.c_str());

		// link
		plugin.entrypoint
			= reinterpret_cast<decltype(plugin.entrypoint)>(module.getFuncAddress(SYMBOL_INIT));
		plugin.exitpoint
			= reinterpret_cast<decltype(plugin.exitpoint)>(module.getFuncAddress(SYMBOL_END));
		plugin.processor
			= reinterpret_cast<decltype(plugin.processor)>(module.getFuncAddress(SYMBOL_PROCESS_REPLACE));
		plugin.handler
			= reinterpret_cast<decltype(plugin.handler)>(module.getFuncAddress(SYMBOL_EVENT_HANDLER));
		// check for errors
		if (!plugin.test(true))
		{
			print("syswrap: Not all functions required to run module was found.");
			return Status::STATUS_ERROR;
		}
		// get global data
		getData_t data = reinterpret_cast<getData_t>(module.getFuncAddress(SYMBOL_GET_DATA));

		
		if (!data) {
			print("[syswrap] : Unable to retrieve program info (no symbol found).");
			return Status::STATUS_ERROR;
		}
		_plugin_global_data * global_data = data();
		if (!global_data) {
			print("[syswrap] : Unable to retrieve program info (no data returned).");
			return Status::STATUS_ERROR;
		}
		this->global_data = global_data;
		
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
		if (!initLocalMemory())
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
		if (plugin.hasEventHandler())
			return plugin.handler(reinterpret_cast<PluginData*>(plugin_data),
			reinterpret_cast<CSharedInterface*>(project->iface),
			e);
		return Status::STATUS_NOT_IMPLEMENTED;
	}
}
/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2014 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:CCodeGenerator.cpp
		Implementation of classes in CCodeGenerator.h

*************************************************************************************/

#include "CCodeGenerator.h"
#include <cpl/PlatformSpecific.h>
#include "Engine.h"
#include <sstream>
#include <cpl/Misc.h>
#include "Settings.h"
#include "SharedInterfaceEx.h"
#include "Engine.h"
#include "UIController.h"
#include "CConsole.h"

namespace ape
{
	/*
	The exported symbol names for external compilers
	*/
	static const char * g_sExports[] =
	{
		"CreateProject",
		"CompileProject",
		"InitProject",
		"ActivateProject",
		"DisableProject",
		"ProcessReplacing",
		"OnEvent",
		"ReleaseProject",
		"CleanCache"
	};

	/*
		Inits all bindings in this CBinding from module.
		If Setting is valid, it is expected to be of this format:
			...
			exports:
			{
				errorFunc = "_@4errorFunc";
				...
			}
		And allows to specify different names/decorations than default.
		If key and value is not found, it will default to the same value in
		g_sExports.
	*/
	bool CCompiler::CBindings::loadBindings(cpl::CModule & module, const libconfig::Setting & exportSettings)
	{
		bool settingsIsValid = exportSettings.isGroup() && !strcmp(exportSettings.getName(), "exports");
		const char * name;
		valid = true;

		cpl::foreach_uenum<ExportIndex>(
			[&](auto i)
			{
				name = nullptr;
				// see if we can get a valid name out of our settings
				if (settingsIsValid && exportSettings.exists(g_sExports[i]))
				{
					name = exportSettings[g_sExports[i]].c_str();
				}
				// shortcircuiting avoids null-dereferencing
				if (!name || (name && !name[0]))
					name = g_sExports[i];
				_table[i] = module.getFuncAddress(name);

				if (!_table[i]) 
				{
					std::stringstream fmt;
					fmt << "Error retrieving pointer for function " << g_sExports[i] << ". ";
					fmt << "Specific name: " << name << ". Was settings valid? " << std::boolalpha << settingsIsValid << ".";
					valid = false;
					throw std::runtime_error(fmt.str());
				}


			}
		);

		return valid;
	}
	bool CCompiler::initialize(const libconfig::Setting& languageSettings)
	{
		if(!initialized)
		{

			compilerName = languageSettings["name"].c_str();
			compilerPath = languageSettings["path"].c_str();
			language = languageSettings.getParent().getName();
			// loop over extensions and insert into this::extensions

			// more checks on this.
			const libconfig::Setting & settings = languageSettings["exports"];

			if(!compilerPath.length()) 
			{
				std::stringstream fmt;
				fmt << "Invalid (empty) path for compiler \'" << compilerName << "\' for language \'"
					<<  language << "\'.";
				throw std::runtime_error(fmt.str());
			}
			module.addSearchPath(cpl::fs::path(cpl::Misc::DirectoryPath() + compilerPath).parent_path());
			auto error = module.load(cpl::Misc::DirectoryPath() + compilerPath);
			if(error)
			{
				std::stringstream fmt;
				fmt << "Error loading compiler module \'" << compilerName << "\' for language \'"
					<<  language << "\' " << " at " << "\'" << cpl::Misc::DirectoryPath() + compilerPath << "\'. OS returns " << error << ".";
				throw std::runtime_error(fmt.str());
			}
			// will throw its own exception
			return initialized = bindings.loadBindings(module, settings);

		}
		return initialized;
	}
	CCompiler::CCompiler()
		: initialized(false)
	{

	}
	CCompiler::CBindings::CBindings() 
		: valid(false) 
	{
		// zero-out pointers
		std::memset(this, 0, sizeof(*this));
	}

	CCodeGenerator::CCodeGenerator(ape::Engine& engine)
		: engine(engine)
	{

	}


	void CCodeGenerator::pluginDiagnostic(Project * project, Diagnostic diag, const char * text)
	{
		if (!project || !text || !project->iface)
			return;

		auto& engine = SharedInterfaceEx::downcast(*project->iface).getEngine();

		engine.getController().externalDiagnostic(diag, text);
	}

	void CCodeGenerator::printError(const cpl::string_ref message, APE_TextColour colour)
	{
		engine.getController().getConsole().printLine(colour, "[Generator] : %s", message.c_str());
	}

	Status CCodeGenerator::activateProject(ProjectEx& project)
	{
		if (project.state > CodeState::Disabled)
		{
			std::stringstream fmt;
			fmt << "Cannot activate project when state is higher than initialized (" << static_cast<int>(project.state.load()) << ").";
			printError(fmt.str());
			return Status::STATUS_ERROR;
		}

		auto status = project.compiler->bindings.activateProject(&project);
		if (status != Status::STATUS_ERROR)
			project.state = CodeState::Activated;
		return status;

		return Status::STATUS_ERROR;
	}

	Status CCodeGenerator::disableProject(ProjectEx & project, bool didMisbehave)
	{
		if (project.state != CodeState::Activated) 
		{
			printError("Cannot disable project when it isn't activated.");
			return Status::STATUS_ERROR;
		} 
		project.state = CodeState::Disabled;
		auto status = project.compiler->bindings.disableProject(&project, didMisbehave ? 1 : 0);
		if(status == Status::STATUS_OK)
			project.state = CodeState::Disabled;
		return status;
	}

	Status CCodeGenerator::processReplacing(ProjectEx& project, float ** in, float ** out, std::size_t sampleFrames)
	{
		if (project.state != CodeState::Activated) 
		{
			printError("Access denied to processing function (plugin is not activated).");
			return Status::STATUS_ERROR;
		} 

		return project.compiler->bindings.processReplacing(&project, in, out, sampleFrames);
	}

	Status CCodeGenerator::onEvent(ProjectEx& project, Event * e)
	{
		if (project.state != CodeState::Activated)
		{
			printError("Access denied to event function (plugin is not activated).");
			return Status::STATUS_ERROR;
		} 

		return project.compiler->bindings.onEvent(&project, e);
	}

	bool CCodeGenerator::compileProject(ProjectEx& project)
	{
		if (project.state < CodeState::Created) 
		{
			printError("Cannot compile non-existing project.");
		} 
		else if(project.state > CodeState::Compiled)
		{
			printError("Cannot compile a project that's already initialized.");
		}
		else 
		{
			if(project.compiler->bindings.compileProject(&project) == Status::STATUS_OK)
			{
				project.state = CodeState::Compiled;
				return true;
			}
		}
		return false;
	}

	bool CCodeGenerator::initProject(ProjectEx& project)
	{
		if(project.state > CodeState::Compiled) 
		{
			printError("Cannot initiate project when state is higher than compiled.");
		} 
		else 
		{
			if(project.compiler->bindings.initProject(&project) == Status::STATUS_OK) 
			{
				project.state = CodeState::Initialized;
				return true;
			}
		}
		return false;
	}

	bool CCodeGenerator::createProject(ProjectEx& project)
	{
		if (project.state > CodeState::None)
		{
			printError("Cannot create already created project.");
		} 
		else if (!project.compiler || !project.compiler->isInitialized())
		{
			// set up compiler here.
			if (!project.languageID || !project.languageID[0])
			{
				printError("Null/empty file extension, not supported");
				return false;
			}

			std::string fileID = project.languageID;
			// ensure lID is valid here !
			try
			{
				const libconfig::Setting& languages = engine.getSettings().root()["languages"];
				std::string langID;
				for (auto && lang : languages)
				{
					if (!lang.isGroup())
						continue;
					for (auto && ext : lang["extensions"])
					{
						if (fileID == ext.c_str())
						{
							langID = lang.getName();
							break;
						}
					}
				}

				if (!langID.size())
				{
					printError("Cannot find compatible compiler for file id \"" + fileID + "\", check your extension settings");
					return false;
				}

				const libconfig::Setting & langstts = languages[langID]["compiler"];

				// compiler is automatically constructed and loaded if it doesn't exist.
				// if it does, we get a reference to it.
				auto& compiler = compilers[langID];

				// one could argue this should happen in compiler::compiler
				// but std::map and copy constructors etc. etc. we do it  here.
				if (!compiler.isInitialized()) 
				{
					if (!compiler.initialize(langstts)) 
					{
						printError("Unable to initialize compiler!");
						return false;
					}
				}
				std::string const args = langstts["arguments"].c_str();
				auto argString = new char[args.length() + 1];
				std::copy(args.begin(), args.end(), argString);
				argString[args.length()] = '\0';
				project.arguments = argString;
				project.compiler = &compiler;
			}
			catch (libconfig::ParseException & e)
			{
				printError("Exception while parsing!");
				printError(e.getError());
				return false;
			}
			catch (libconfig::SettingNameException & e)
			{
				printError(e.getPath());
				return false;
			}
			catch (libconfig::SettingNotFoundException & e)
			{
				printError("Setting not found!");
				printError(e.getPath());
				return false;
			}
			catch (libconfig::SettingException & e)
			{
				printError("Error in settings!");
				printError(e.getPath());
				return false;
			}
			catch (std::exception & e)
			{
				printError("Error while loading compiler!");
				printError(e.what());
				return false;
			}
		}

		project.reportDiagnostic = &pluginDiagnostic;

		if (project.compiler->bindings.createProject(&project) == Status::STATUS_OK) 
		{
			project.state = CodeState::Created;
			return true;
		}

		return false;
	}


	bool CCodeGenerator::releaseProject(ProjectEx& project)
	{
		if(project.state < CodeState::Created) 
		{
			printError("Cannot release project when it isn't created.");
			return false;
		} 
		else if (project.state == CodeState::Activated)
		{
			printError("Warning: Releasing activated project!");
		}

		if(project.compiler->bindings.releaseProject(&project) == STATUS_OK) 
		{
			project.state = CodeState::Released;
			return true;
		}
		return false;
	}

	void CCodeGenerator::cleanAllCaches()
	{
		try
		{
			printError("Cleaning " + std::to_string(compilers.size()) + " loaded compiler(s)", APE_TextColour_Default);
			for (auto& pairs : compilers)
			{
				if (pairs.second.isInitialized())
				{
					printError("Cleaning for " + pairs.first + ": " + pairs.second.name(), APE_TextColour_Default);
					pairs.second.bindings.cleanCache();
				}
				else
				{
					printError("Non-initialized compiler for " + pairs.first + " couldn't be cleaned: " + pairs.second.name());
				}
			}
		}
		catch (const std::exception& e)
		{
			printError(e.what());
		}
	}


};
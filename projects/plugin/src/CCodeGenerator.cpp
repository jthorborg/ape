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
#include "CompilerBinding.h"

namespace ape
{

	CCodeGenerator::CCodeGenerator(ape::Engine& engine)
		: engine(engine)
	{

	}

    CCodeGenerator::~CCodeGenerator()
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
		else if (!project.compiler)
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
                
                auto compiler = compilers.find(langID);
                
                if(compiler == compilers.end())
                {
                    auto binding = std::make_unique<CompilerBinding>(langstts);
                    compilers[langID] = std::move(binding);
                    compiler = compilers.find(langID);
                }
                
				std::string const args = langstts["arguments"].c_str();
				auto argString = new char[args.length() + 1];
				std::copy(args.begin(), args.end(), argString);
				argString[args.length()] = '\0';
				project.arguments = argString;
				project.compiler = compiler->second.get();
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
                printError("Cleaning for " + pairs.first + ": " + pairs.second->name(), APE_TextColour_Default);
                pairs.second->bindings.cleanCache();
			}
		}
		catch (const std::exception& e)
		{
			printError(e.what());
		}
	}


};

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
#include "PlatformSpecific.h"
#include "Engine.h"
#include "Misc.h"
#include "Settings.h"

namespace APE
{
	// this is horrible kill me, but the definition might change
	#define SUCCESS(x) (x == 0)

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
		bool CCompiler::CBindings::loadBindings(CModule & module, const libconfig::Setting & exportSettings)
		{
			bool settingsIsValid = exportSettings.isGroup() && !strcmp(exportSettings.getName(), "exports");
			const char * name;
			valid = true;
			for(unsigned i = 0; i < EExports::eDummy; ++i) {
				name = nullptr;
				// see if we can get a valid name out of our settings
				if(settingsIsValid && exportSettings.exists(g_sExports[i]))
				{
					name  = exportSettings[g_sExports[i]].c_str();
				}
				// shortcircuiting avoids null-dereferencing
				if(!name || (name && !name[0]))
					name = g_sExports[i];
				_table[i] = module.getFuncAddress(name);

				if(!_table[i]) {
					APE::Misc::CStringFormatter fmt;
					fmt << "Error retrieving pointer for function " << g_sExports[i] << ". ";
					fmt << "Specific name: " << name << ". Was settings valid? " << settingsIsValid << ".";
					valid = false;
					throw Misc::CStrException(fmt.str());
					break;
				}
			}
			return valid;
		}
		bool CCompiler::initialize(const libconfig::Setting & languageSettings)
		{
			if(!initialized) {

				compilerName = languageSettings["name"].c_str();
				#ifdef APE_CHECK_FOR_TCC_HACK
					if (Globals::CheckForTCC && compilerName.find_first_of("tcc") != std::string::npos)
					{
						Globals::ApplyTCCConvHack = true;
					}
				#endif
				compilerPath = languageSettings["path"].c_str();
				language = languageSettings.getParent().getName();
				// loop over extensions and insert into this::extensions

				// more checks on this.
				const libconfig::Setting & settings = languageSettings["exports"];

				if(!compilerPath.length()) {
					Misc::CStringFormatter fmt;
					fmt << "Invalid (empty) path for compiler \'" << compilerName << "\' for language \'"
						<<  language << "\'.";
					throw Misc::CStrException(fmt.str());
				}
				auto error = module.load(Misc::DirectoryPath() + compilerPath);
				if(error) {
					Misc::CStringFormatter fmt;
					fmt << "Error loading compiler module \'" << compilerName << "\' for language \'"
						<<  language << "\' " << " at " << "\'" << Misc::DirectoryPath() + compilerPath << "\'. OS returns " << error << ".";
					throw Misc::CStrException(fmt.str());
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


		Status CCodeGenerator::activateProject(ProjectEx * project)
		{
			if(!project) 
			{
				printError("Nullptr passed to activateProject");
			}
			else if(project->state > CodeState::Disabled)
			{
				Misc::CStringFormatter fmt;
				fmt << "Cannot activate project when state is higher than initialized (" << static_cast<int>(project->state) << ").";
				printError(fmt.str());
			} else 
			{
				auto status = project->compiler->bindings.activateProject(project);
				if(status != Status::STATUS_ERROR)
					project->state = CodeState::Activated;
				return status;
			}
			return Status::STATUS_ERROR;
		}

		CCodeGenerator::CCodeGenerator(APE::Engine * engine)
			: errorPrinter(nullptr), engine(engine)
		{

		}

		void CCodeGenerator::setErrorFunc(ErrorFunc f, void * op)
		{
			opaque = op;
			errorPrinter = f;
		}
		void CCodeGenerator::printError(const std::string & message)
		{
			if(opaque && errorPrinter)
				errorPrinter(opaque, std::string("[Generator] : " + message).c_str());

		}
		Status CCodeGenerator::disableProject(ProjectEx * project)
		{
			if(!project) {
				printError("Nullptr passed to disableProject!");
			}
			else if (!project->compiler)
			{
				printError("No compiler exists for project.");
			}
			else if (project->state != CodeState::Activated) {
				printError("Cannot disable project when state isn't activated.");
			} else {
				auto status = project->compiler->bindings.disableProject(project);
				if(SUCCESS(status))
					project->state = CodeState::Disabled;
				return status;
			}
			return Status::STATUS_ERROR;
		}

		Status CCodeGenerator::processReplacing(ProjectEx * project, Float ** in, Float ** out, Int sampleFrames)
		{
			if(!project) 
			{
				printError("Nullptr passed to processReplacing!");
			}
			else if (!project->compiler)
			{
				printError("No compiler exists for project.");
			}
			else if (project->state != CodeState::Activated) 
			{
				printError("Access denied to processing function (plugin is not activated).");
			} else {
				auto status = 
				(
					project->compiler->bindings.processReplacing(project, in, out, sampleFrames)
				);
				return status;
			}
			return Status::STATUS_ERROR;
		}

		Status CCodeGenerator::onEvent(ProjectEx * project, Event * e)
		{
			if(!project) 
			{
				printError("Nullptr passed to onEvent!");
			}
			else if (!project->compiler)
			{
				printError("No compiler exists for project.");
			}
			else if (project->state != CodeState::Activated) {
				printError("Access denied to event function (plugin is not activated).");
			} else {
				return project->compiler->bindings.onEvent(project, e);
			}
			return Status::STATUS_ERROR;

		}

		bool CCodeGenerator::compileProject(ProjectEx * project)
		{
			if(!project) 
			{
				printError("Nullptr passed to compileProject!");
			}
			else if (project->state > CodeState::None) 
			{
				printError("Cannot compile project when state is higher than none.");
			} 
			else 
			{
				if(!project->compiler || !project->compiler->isInitialized()) 
				{
					// set up compiler here.
					if (!project->languageID || !project->languageID[0])
					{
						printError("Null/empty file extension, not supported");
					}

					std::string fileID = project->languageID;
					// ensure lID is valid here !
					try
					{
						const libconfig::Setting & settings = engine->getRootSettings();
						std::string langID;
						for (auto && lang : settings["languages"])
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

						const libconfig::Setting & langstts = settings["languages"][langID]["compiler"];

						// compiler is automatically constructed and loaded if it doesn't exist.
						// if it does, we get a reference to it.
						auto compiler = &compilers[langID];
						// one could argue this should happen in compiler::compiler
						// but std::map and copy constructors etc. etc. we do it  here.
						if(!compiler->isInitialized()) {
							if(!compiler->initialize(langstts)) {
								printError("Unable to initialize compiler!");
								return false;
							}
						}
						std::string const args = langstts["arguments"].c_str();
						auto argString = new char[args.length() + 1];
						std::copy(args.begin(), args.end(), argString);
						argString[args.length()] = '\0';
						project->arguments = argString;
						project->compiler = compiler;
					}
					catch(libconfig::ParseException & e)
					{
						printError("Exception while parsing!");
						printError(e.getError());
						return false;
					}
					catch(libconfig::SettingNameException & e)
					{
						printError(e.getPath());
						return false;
					}
					catch(libconfig::SettingNotFoundException & e)
					{
						printError("Setting not found!");
						printError(e.getPath());
						return false;
					}
					catch(libconfig::SettingException & e)
					{
						printError("Error in settings!");
						printError(e.getPath());
						return false;
					}
					catch(std::exception & e)
					{
						printError("Error while loading compiler!");
						printError(e.what());
						return false;
					}
				}

				auto status = project->compiler->bindings.compileProject(project, opaque, errorPrinter);
				if(SUCCESS(status)) {
					project->state = CodeState::Compiled;
					return true;
				}
			}
			return false;
		}
		bool CCodeGenerator::initProject(ProjectEx * project)
		{
			if(!project) 
			{
				printError("Nullptr passed to initProject!");
			}
			else if (!project->compiler)
			{
				printError("No compiler exists for project.");
			}
			else if(project->state > CodeState::Compiled) 
			{
				printError("Cannot initiate project when state is higher than compiled.");
			} 
			else 
			{
				auto status = project->compiler->bindings.initProject(project);
				if(SUCCESS(status)) {
					project->state = CodeState::Initialized;
					return true;
				}
			}
			return false;
		}
		bool CCodeGenerator::releaseProject(ProjectEx * project)
		{
			if(!project) {
				printError("Nullptr passed to releaseProject!");
			}
			else if (!project->compiler)
			{
				printError("No compiler exists for project.");
			}
			else if(project->state < CodeState::Compiled) 
			{
				printError("Cannot release project when it isn't compiled.");
			} 
			else 
			{
				auto status = project->compiler->bindings.releaseProject(project);
				if(SUCCESS(status)) {
					project->state = CodeState::Released;
					return true;
				}
			}
			return false;
		}


};
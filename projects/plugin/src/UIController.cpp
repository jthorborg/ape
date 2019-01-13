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

	file:UIController.cpp
		
		Core implementation of the GUI and interactions inbetween.

*************************************************************************************/

#include "UIController.h"
#include <cpl/Misc.h>
#include "Engine.h"
#include "PluginState.h"
#include "CConsole.h"
#include "ProjectEx.h"
#include <stdio.h>
#include "UI/LabelQueue.h"
#include <chrono>
#include "CodeEditor/SourceManager.h"
#include <typeinfo>
#include "MainEditor/MainEditor.h"
#include "CodeEditor/AutosaveManager.h"
#include "UI/UICommands.h"

namespace ape 
{
	using namespace std::string_literals;

	UIController::UIController(ape::Engine& effect)
		: projectName(cpl::programInfo.programAbbr)
		, engine(effect)
		, console(std::make_unique<CConsole>())
	{

		editorSSO = std::make_unique<cpl::SerializableStateObject<MainEditor>>(
			[this] 
			{ 
				return std::make_unique<MainEditor>(*this); 
			}
		);

		commandStates = std::make_unique<UICommandState>(*this);
		
		auto& app = effect.getSettings().root()["application"];

		// create console
		if(app["log_console"])
			getConsole().setLogging(true, cpl::Misc::DirFSPath() / "logs" / ("log"s + std::to_string(engine.instanceCounter()) + ".txt"));

		if (app["console_std_writing"])
			getConsole().setStdWriting(true);

		sourceManager = MakeSourceManager(*this, effect.getSettings(), effect.uniqueInstanceID());
		autosaveManager = std::make_unique<AutosaveManager>(effect.uniqueInstanceID(), effect.getSettings(), *sourceManager, *this);
		
		labelQueue.setDefaultMessage("Ready", juce::Colours::lightgoldenrodyellow);
	}

	void UIController::onBreakpointsChanged(const std::set<int>& newTraces)
	{
		if (engine.isProcessingAPlugin())
			recompile();
	}


	void UIController::externalDiagnostic(Diagnostic level, const cpl::string_ref text)
	{
		auto messageColour = APE_TextColour_Default;

		switch (level)
		{
		case APE_Diag_Warning:
			messageColour = APE_TextColour_Warning;
			break;
		case APE_Diag_CompilationError:
		{
			// TODO: Find line error
			// TCC layout of error message: "<%file%>:%line%: error: %msg%
			// set the error if the editor is open (not our responsibility), defaults to -1 
			// which should be ignored by the func.
			// setEditorError(nLinePos);
		}
		case APE_Diag_Error:
			messageColour = APE_TextColour_Error;

		}

		getConsole().printLine(messageColour, "%s", text.c_str());

	}

	void UIController::setPlugin(std::shared_ptr<PluginState> newPlugin)
	{
		if (currentPlugin)
		{
			// TODO
			int k = 0;
		}

		currentPlugin = std::move(newPlugin);
		engine.exchangePlugin(currentPlugin);
	}
	
	void UIController::pulseUI()
	{
		engine.pulse();
		labelQueue.pulseQueue();
	}

	UIController::~UIController()
	{
		notifyDestruction();
	}


	void UIController::editorOpened(MainEditor * newEditor)
	{
		if (currentPlugin && currentPlugin->isEnabled())
		{
			newEditor->onPluginStateChanged(*currentPlugin, true);
		}

		autosaveManager->checkAutosave();
	}


	void UIController::editorClosed()
	{
	}

	MainEditor * UIController::create()
	{
		if (editorSSO->hasCached())
			getConsole().printLine(CConsole::Error, "[GUI] : error! Request to create new editor while old one still exists. "
			"Reference to old editor lost!");

		auto instance = editorSSO->getUnique();
		auto const bUseOpenGL = engine.getSettings().lookUpValue(false, "application", "render_opengl");
		instance->initialize(bUseOpenGL);
		
		return instance.acquire();
	}


	void UIController::setEditorError(int nLine) 
	{
		sourceManager->setErrorLine(nLine);
	}

	bool UIController::performCommand(UICommand command)
	{
		switch (command)
		{

		case UICommand::Recompile:
		{
			recompile();
			break;
		}

		case UICommand::Activate:
		{
			if (compilerState.valid())
			{
				switch (compilerState.wait_for(std::chrono::seconds(0)))
				{
				case std::future_status::ready:
					setPlugin(compilerState.get());
					break;
				case std::future_status::deferred:
				case std::future_status::timeout:
					labelQueue.pushMessage("Cannot activate plugin while compiling...", CColours::red, 2000);
					getConsole().printLine(CConsole::Error, "[GUI] : cannot activate while compiling.");
					return false;
				}
			}

			if (currentPlugin)
			{
				if (currentPlugin->isEnabled())
				{
					getConsole().printLine(CConsole::Error, "[GUI] : Cannot activate plugin that's not disabled.", 2000);
					labelQueue.setDefaultMessage("Error activating plugin.", CColours::red);
					return false;
				}

				if (engine.activatePlugin())
				{
					// success
					labelQueue.setDefaultMessage("Plugin activated", CColours::green);
					getUICommandState().changeValueExternally(getUICommandState().activationState, 1);
					if (editorSSO->hasCached())
					{
						editorSSO->getCached()->onPluginStateChanged(*currentPlugin, true);
					}

				}
				else
				{
					getConsole().printLine(CConsole::Error, "[GUI] : Error activating plugin.", 2000);
					labelQueue.setDefaultMessage("Error activating plugin.", CColours::red);
				}
			}
			else
			{
				labelQueue.pushMessage("No compiled symbols found", CColours::red, 2000);
				getConsole().printLine(CConsole::Error, "[GUI] : Failure to activate plugin, no compiled code available.");
				return false;
			}
			break;
		}

		case UICommand::Deactivate:
		{
			if (currentPlugin && currentPlugin->isEnabled())
			{
				labelQueue.pushMessage("Plugin disabled", CColours::lightgoldenrodyellow, 1000);

				if (editorSSO->hasCached())
					editorSSO->getCached()->onPluginStateChanged(*currentPlugin, false);

				engine.disablePlugin(true);

				getUICommandState().changeValueExternally(getUICommandState().activationState, 0);
			}
			break;
		}

		case UICommand::Clean:
		{
			engine.getCodeGenerator().cleanAllCaches();
		}

		default:
			break;
		}

		return true;
	}

	void UIController::serialize(cpl::CSerializer::Archiver & ar, cpl::Version version)
	{
		ar["source-manager"] << *sourceManager;
		ar["command-state"] << *commandStates;
		ar["editor"] = editorSSO->getState();
	}

	void UIController::deserialize(cpl::CSerializer::Builder & builder, cpl::Version version)
	{
		builder["source-manager"] >> *sourceManager;
		if(builder.findForKey("command-state") != nullptr)
			builder["command-state"] >> *commandStates;

		editorSSO->setState(builder["editor"], version);
	}

	void UIController::recompile(bool hotReload)
	{
		if (!compilerState.valid())
		{
			compilerState = createPlugin(sourceManager->createProject(), hotReload);
			//engine.disablePlugin(false);
		}
		else
		{
			switch (compilerState.wait_for(std::chrono::seconds(0)))
			{
			case std::future_status::ready:
				compilerState = createPlugin(sourceManager->createProject(), hotReload);
				//engine.disablePlugin(false);
				break;
			case std::future_status::deferred:
			case std::future_status::timeout:
				labelQueue.pushMessage("Already compiling, please wait...", CColours::red, 2000);
				getConsole().printLine(CColours::red, "[GUI] : cannot compile while compiling.");
				break;

			}
		}
	}

	std::future<std::unique_ptr<PluginState>> UIController::createPlugin(bool enableHotReload)
	{
		return createPlugin(sourceManager->createProject(), enableHotReload);
	}

	const std::string & UIController::getProjectName() const noexcept 
	{ 
		return projectName;
	}


	std::future<std::unique_ptr<PluginState>> UIController::createPlugin(std::unique_ptr<ProjectEx> project, bool enableHotReload)
	{
		if (!project)
		{
			getConsole().printLine(CConsole::Error, "[GUI] : Compilation error - invalid project or no text recieved from editor.");
			labelQueue.pushMessage("No code to compile", CColours::red, 3000);

			return {};
		}

		setProjectName(project->projectName);
		labelQueue.pushMessage("Compiling...", CColours::red, 500);
		getConsole().printLine("[GUI] : Compiling...");

		getUICommandState().changeValueExternally(getUICommandState().compile, 1);

		return std::async(
			[=] (auto projectToCompile)
			{
				std::unique_ptr<PluginState> ret;
				try
				{
					auto start = std::chrono::high_resolution_clock::now();
					ret = std::make_unique<PluginState>(engine, engine.getCodeGenerator(), std::move(projectToCompile));
					auto delta = std::chrono::high_resolution_clock::now() - start;
					auto time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(delta);

					getConsole().printLine("[GUI] : Compiled successfully (%f ms).", time);
					labelQueue.pushMessage("Compiled OK!", CColours::green, 2000);

				}
				catch (const std::exception& e)
				{
					getConsole().printLine(CConsole::Error, "[GUI] : Error compiling project (%s: %s).", typeid(e).name(), e.what());
					labelQueue.pushMessage("Error while compiling (see console)!", CColours::red, 5000);
				}
				
				cpl::GUIUtils::MainEvent(*this, 
					[=] 
					{ 
						if(enableHotReload)
							performCommand(UICommand::Activate); 

						getUICommandState().changeValueExternally(getUICommandState().compile, 0);
					}
				);

				return ret;
			},
			std::move(project)
		);
	}

	void UIController::setProjectName(std::string name) 
	{ 
		projectName = std::move(name); 
		labelQueue.setDefaultPrefix(projectName + " - "); 
	}

	void * UIController::getSystemWindow() 
	{ 
		return editorSSO->hasCached() ? editorSSO->getCached()->getWindowHandle() : nullptr; 
	}

	juce::AudioProcessorEditor* Engine::createEditor()
	{
		return controller->create();
	}
};

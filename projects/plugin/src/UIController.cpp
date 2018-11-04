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
#include "CQueueLabel.h"
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
		, editor(nullptr)
		, engine(effect)
		, bIsActive(effect.status.bActivated)
		, bUseBuffers(engine.status.bUseBuffers)
		, bUseFPUE(engine.status.bUseFPUE)
		, bFirstDraw(true)
		, incGraphicCounter(0)
		, consolePtr(std::make_unique<CConsole>())
	{
		commandStates = std::make_unique<UICommandState>(*this);
		
		auto& app = effect.getSettings().root()["application"];

		// create console
		if(app["log_console"])
			console().setLogging(true, cpl::Misc::DirFSPath() / "logs" / ("log"s + std::to_string(engine.instanceCounter()) + ".txt"));

		if (app["console_std_writing"])
			console().setStdWriting(true);

		uiRefreshInterval = app["ui_refresh_interval"];

		sourceManager = MakeSourceManager(*this, effect.getSettings(), effect.uniqueInstanceID());
		autosaveManager = std::make_unique<AutosaveManager>(effect.uniqueInstanceID(), effect.getSettings(), *sourceManager, *this);

		// 0.025 is pretty smooth
		clockData.pole = 0.025f;
		clockData.averageClocks = 0;
		clockData.lastSample = engine.clocksPerSample;
		
		setStatusText("Ready");
	}

	void UIController::errorPrint(void * data, const char * text)
	{
		if (!data || !text)
			return;

		UIController * controller = static_cast<UIController*>(data);

		if (controller->magic != magic_value)
			return;

		controller->onErrorMessage(text);
	}

	void UIController::onBreakpointsChanged(const std::set<int>& newTraces)
	{
		recompile();
	}


	void UIController::onErrorMessage(const cpl::string_ref text)
	{
		console().printLine(CConsole::Warning, "[Compiler] : %s", text.c_str());

		int nLinePos(-1), i(0);

		for (; i < text.size(); ++i) 
		{
			// layout of error message: "<%file%>:%line%: error: %msg%
			if (text[i] == '>')
			{
				i += 2; // skip '>:'
				if (i >= text.size())
					break;
				sscanf(text.c_str() + i, "%d", &nLinePos);
				break;
			}
		}

		// set the error if the editor is open (not our responsibility), defaults to -1 
		// which should be ignored by the func.
		setEditorError(nLinePos);
	}

	void UIController::updateInfoLabel()
	{
		if (!editor)
		{
			console().printLine(CConsole::Error, "[GUI] : error! Request to update clock counter from invalid editor!");
		}
		
		clockData.lastSample = engine.clocksPerSample;
		clockData.averageClocks = clockData.averageClocks + clockData.pole * (clockData.lastSample - clockData.averageClocks);
		char buf[200];
		
		sprintf_s(buf, "Instance %d - accps: ~%d (%d)",
				  engine.instanceCounter(),
				  (unsigned)clockData.averageClocks,
				  clockData.lastSample);;
		
		editor->infoLabel->setText(buf);
	}
	
	void UIController::render()
	{
		engine.getParameterManager().pulse();

		incGraphicCounter++;

		// stuff that should run once at least
		if(bFirstDraw)
		{
			updateInfoLabel();
			bFirstDraw = false;
		}
		// not-so-often updated stuff
		if(!(incGraphicCounter % 5))
		{
			updateInfoLabel();
		}
	}

	UIController::~UIController()
	{
		notifyDestruction();
	}


	void UIController::editorOpened(MainEditor * newEditor)
	{
		//setStatusText();
		if (auto state = engine.getCurrentPluginState(); state && state->isEnabled())
		{
			newEditor->onPluginStateChanged(*state, true);
		}
		newEditor->startTimer(engine.getSettings().root()["application"]["ui_refresh_interval"]);

		autosaveManager->checkAutosave();

		bFirstDraw = true;
	}


	void UIController::editorClosed()
	{
		editor = nullptr;
	}

	MainEditor * UIController::create()
	{
		if (editor)
			console().printLine(CConsole::Error, "[GUI] : error! Request to create new editor while old one still exists. "
			"Reference to old editor lost!");
		editor = new MainEditor(*this);

		bool bUseOpenGL(false);
		bUseOpenGL = engine.getSettings().root()["application"]["render_opengl"];

		
		editor->initialize(bUseOpenGL);
		
		setStatusText();
		return editor;
	}


	void UIController::setEditorError(int nLine) 
	{
		sourceManager->setErrorLine(nLine);
	}


	void UIController::setStatusText(const std::string & text, CColour colour)
	{

		cpl::CMutex lockGuard(this);
		statusLabel = projectName + " - " + text;
		statusColour = colour;
		if (editor)
		{
			editor->statusLabel->setDefaultMessage(statusLabel, colour);
		}
	}


	void UIController::setStatusText(const std::string & text, CColour colour, int ms)
	{
		if (editor)
		{
			editor->statusLabel->pushMessage(text, colour, ms);
		}
	}


	void UIController::setStatusText()
	{
		cpl::CMutex lockGuard(this);
		if (editor)
		{
			editor->statusLabel->setDefaultMessage(statusLabel, statusColour);
		}
	}


	std::string UIController::getStatusText()
	{
		cpl::CMutex lockGuard(this);
		return statusLabel;
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
					engine.disablePlugin(true);
					engine.exchangePlugin(compilerState.get());
					break;
				case std::future_status::deferred:
				case std::future_status::timeout:
					setStatusText("Cannot activate plugin while compiling...", CColours::red, 2000);
					console().printLine(CConsole::Error, "[GUI] : cannot activate while compiling.");
					return false;
				}
			}

			if (auto currentState = engine.getCurrentPluginState())
			{
				if (currentState->getState() != STATUS_DISABLED)
				{
					console().printLine(CConsole::Error, "[GUI] : Cannot activate plugin that's not disabled.", 2000);
					setStatusText("Error activating plugin.", CColours::red);
					return false;
				}

				if (engine.activatePlugin())
				{
					// success
					setStatusText("Plugin activated", CColours::green);
					getUICommandState().changeValueExternally(getUICommandState().activationState, 1);
					if (editor)
					{
						editor->onPluginStateChanged(*currentState, true);
					}

				}
				else
				{
					console().printLine(CConsole::Error, "[GUI] : Error activating plugin.", 2000);
					setStatusText("Error activating plugin.", CColours::red);
				}
			}
			else
			{
				setStatusText("No compiled symbols found", CColours::red, 2000);
				console().printLine(CConsole::Error, "[GUI] : Failure to activate plugin, no compiled code available.");
				return false;
			}
			



			break;
		}

		case UICommand::Deactivate:
		{
			if (auto plugin = engine.getCurrentPluginState())
			{
				if (plugin->getState() != STATUS_DISABLED)
				{
					setStatusText("Plugin disabled", CColours::lightgoldenrodyellow, 1000);
					if (editor)
						editor->onPluginStateChanged(*plugin, false);
					engine.disablePlugin(true);

					getUICommandState().changeValueExternally(getUICommandState().activationState, 0);
				}
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
	}

	void UIController::deserialize(cpl::CSerializer::Builder & builder, cpl::Version version)
	{
		builder["source-manager"] >> *sourceManager;
		if(builder.findForKey("command-state") != nullptr)
			builder["command-state"] >> *commandStates;
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
				setStatusText("Already compiling, please wait...", CColours::red, 2000);
				console().printLine(CColours::red, "[GUI] : cannot compile while compiling.");
				break;

			}
		}
	}

	std::future<std::unique_ptr<PluginState>> UIController::createPlugin(bool enableHotReload)
	{
		return createPlugin(sourceManager->createProject(), enableHotReload);
	}


	std::future<std::unique_ptr<PluginState>> UIController::createPlugin(std::unique_ptr<ProjectEx> project, bool enableHotReload)
	{
		if (!project)
		{
			console().printLine(CConsole::Error, "[GUI] : Compilation error - "
				"invalid project or no text recieved from editor.");
			setStatusText("No code to compile", CColours::red, 3000);
			return {};
		}

		setProjectName(project->projectName);
		setStatusText("Compiling...", CColours::red, 500);

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

					console().printLine("[GUI] : Compiled successfully (%f ms).", time);
					setStatusText("Compiled OK!", CColours::green, 2000);

				}
				catch (const std::exception& e)
				{
					console().printLine(CConsole::Error, "[GUI] : Error compiling project (%s: %s).", typeid(e).name(), e.what());
					setStatusText("Error while compiling (see console)!", CColours::red, 5000);
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

	void * UIController::getSystemWindow() 
	{ 
		return editor ? editor->getWindowHandle() : nullptr; 
	}

	juce::AudioProcessorEditor* Engine::createEditor()
	{
		return controller->create();
	}
};

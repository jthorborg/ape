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

	file:GUI.cpp
		
		Core implementation of the GUI and interactions inbetween.

*************************************************************************************/

#include "UIController.h"
#include "ButtonDefinitions.h"
#include <cpl/Misc.h>
#include "Engine.h"
#include "PluginState.h"
#include "CConsole.h"
#include "ProjectEx.h"
#include <stdio.h>
#include "CQueueLabel.h"
#include <chrono>
#include "CodeEditor/CCodeEditor.h"
#include <typeinfo>

namespace ape 
{
	using namespace std::string_literals;

	/*********************************************************************************************
	 
		Some data about our buttons behaviour
	 
	 *********************************************************************************************/
	struct sButton { int tag;  const char * untoggled, *toggled; bool sticky; };

	sButton ButtonDefs[] = {
		{ tagConsole, "Console", "Hide", true},
		{ tagCompile, "Compile", "Compile", false },
		{ tagActiveState, "Activate", "Deactivate", true },
		{ tagEditor, "Show editor", "Hide editor", true },
		{ tagAbout, "About", "About", false }
	};

	/*********************************************************************************************

	 	Constructor of the class. Initializes nearly everything to a passive state, and links
		some of the engine's state to itself (button toggles, for instance.)

	 *********************************************************************************************/
	UIController::UIController(ape::Engine& effect)
		: projectName(cpl::programInfo.programAbbr)
		, editor(nullptr)
		, engine(effect)
		, bIsActive(effect.status.bActivated)
		, bUseBuffers(engine.status.bUseBuffers)
		, bUseFPUE(engine.status.bUseFPUE)
		, autoSaveCounter(0)
		, bFirstDraw(true)
		, incGraphicCounter(0)
		, consolePtr(std::make_unique<CConsole>())
	{
		
		auto& app = effect.getSettings().root()["application"];

		// create console
		if(app["log_console"])
			console().setLogging(true, cpl::Misc::DirFSPath() / "logs" / ("log"s + std::to_string(engine.instanceCounter()) + ".txt"));

		if (app["console_std_writing"])
			console().setStdWriting(true);

		autoSaveInterval = app["autosave_interval"];
		uiRefreshInterval = app["ui_refresh_interval"];
		// create the editor
		externEditor = MakeCodeEditor(*this, effect.getSettings(), effect.uniqueInstanceID());
		
		// 0.025 is pretty smooth
		clockData.pole = 0.025f;
		clockData.averageClocks = 0;
		clockData.lastSample = engine.clocksPerSample;
		
		// manually load resources in case a host is frantic and calls
		// serializing without initiating out editor (hello reaper bridge)
		CResourceManager::instance().loadResources();
		
		setStatusText("Ready");
	}
	/*********************************************************************************************
	 
		Constructor for the editor. Creates all graphical components. Notifies parent
	 
	 *********************************************************************************************/
	UIController::Editor::Editor(UIController& p)
		: parent(p)
		, AudioProcessorEditor(p.engine)
		, repaintCallBackCounter(0), bImage(CResourceManager::getImage("background"))
		, scope(p.engine.getOscilloscopeData())
		//,testImage(juce::Image::PixelFormat::ARGB, 800,300, false, *new juce::OpenGLImageType())
	{

		/*
		if (!approot["greeting_shown"])
		{
			cpl::Misc::MsgBox("Hello and welcome to " + cpl::programInfo.name + "! Before you start using this program, "
				"please take time to read the readme and agree to all licenses + disclaimers found in /licenses. "
				"Have fun!", cpl::programInfo.name, cpl::Misc::MsgIcon::iInfo);
			approot["greeting_shown"] = true;
		} */

		// get background
		background.setImage(bImage);
		addAndMakeVisible(background);
		// background and sizing off gui
		// everything is sized relative to the background image
		CPoint size(background.getWidth(), background.getHeight());
		setSize(size.x, size.y);

		// add buttons
		CButton * b = nullptr;
		for (int i = 0; i < std::extent<decltype(ButtonDefs)>::value; i++)
		{
			b = new CButton(ButtonDefs[i].toggled, ButtonDefs[i].untoggled, &parent);
			b->bSetTag(ButtonDefs[i].tag);
			controls[ButtonDefs[i].tag] = b;
			b->bSetPos(0, i * b->getHeight());
			if (ButtonDefs[i].sticky)
				b->setMultiToggle(true);
			this->addChildComponent(b);
			garbageCollection.push_back(b);
		}

		// create toggles
		// this is the protected buffer
		auto toggle = new CToggle();
		toggle->bSetSize(CRect(b->getWidth(), getHeight() - 20, 200, 20));
		if (parent.bUseBuffers)
			toggle->bSetValue(1);
		toggle->bSetListener(&parent);
		toggle->bSetText("Use protected buffers");
		toggle->bSetTag(kTags::tagUseBuffer);
		garbageCollection.push_back(toggle);
		addAndMakeVisible(toggle);
		// add exceptions toggle
		toggle = new CToggle();
		toggle->bSetSize(CRect(b->getWidth() + 200, getHeight() - 20, 200, 20));
		if (parent.bUseFPUE)
			toggle->bSetValue(1);
		toggle->bSetListener(&parent);
		toggle->bSetTag(kTags::tagUseFPU);
		toggle->bSetText("Use FPU exceptions");
		garbageCollection.push_back(toggle);
		addAndMakeVisible(toggle);

		// labels
		infoLabel = new CTextControl();
		infoLabel->bSetPos(b->getWidth() + 5, getHeight() - 40);
		infoLabel->setSize(220, 20);
		infoLabel->setColour(CColours::lightgoldenrodyellow);
		infoLabel->setFontSize(TextSize::smallText);
		addAndMakeVisible(infoLabel);
		garbageCollection.push_back(infoLabel);

		statusLabel = new CQueueLabel();
		statusLabel->setBounds(CRect(getWidth() - 300, getHeight() - 25 , 290, 20));
		statusLabel->setFontSize(TextSize::largeText);
		statusLabel->setJustification(juce::Justification::centredRight);
		statusLabel->setColour(CColours::lightgoldenrodyellow);
		addAndMakeVisible(statusLabel);
		garbageCollection.push_back(statusLabel);
		// create the editor
		// spawn console
		parent.console().create(CRect(b->getWidth(), 0, getWidth() - b->getWidth(), getHeight() - (getHeight() / 6)));
	
		/*
			set buttons according to engine
		*/
		
		controls[kTags::tagActiveState]->bSetInternal(parent.bIsActive ? 1.f : 0.f);
	}
	/*********************************************************************************************
	 
		Destructor. Delets graphic elements, stops timers and notifies parent
	 
	 *********************************************************************************************/
	UIController::Editor::~Editor()
	{
		oglc.detach();
		for (auto garbage : garbageCollection)
			delete garbage;
		parent.console().close();
		if (isTimerRunning())
			stopTimer();
		parent.editorClosed();
	}
	/*********************************************************************************************
	 
		Paint loop for the editor. Calls the paren't rendering loop
	 
	 *********************************************************************************************/
	void UIController::Editor::paint(juce::Graphics & g)
	{
		// draw background image
		//g.drawImage(background, 0, 0, getWidth(), getHeight(), 0, 0, background.getWidth(), background.getHeight());
		parent.render();
	}
	void UIController::Editor::initialize(bool useOpenGL)
	{
		if(useOpenGL)
			oglc.attachTo(*this);
		
		
		parent.editorOpened(this);	
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


	void UIController::onErrorMessage(const cpl::string_ref text)
	{
		console().printLine(CColours::red, "[Compiler] : %s", text.c_str());

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

	/*********************************************************************************************
	 
		Updates the info label with new data.
	 
	 *********************************************************************************************/
	void UIController::updateInfoLabel()
	{
		if (!editor)
		{
			console().printLine(CColours::red, "[GUI] : error! Request to update clock counter from invalid editor!");
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
	
	/*********************************************************************************************
	 
		Renders any objects marked as dirty. Calls autosave regularly
	 
	 *********************************************************************************************/
	void UIController::render()
	{
		incGraphicCounter++;
		if (autoSaveInterval > 0)
		{
			autoSaveCounter++;
			if ((autoSaveCounter * uiRefreshInterval / 1000.f) > autoSaveInterval)
			{
				externEditor->autoSave();
				autoSaveCounter = 0;
			}
		}

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
		
		// everything beyond here is drawn each frame
		
		/*
		 Dont render controls when the console is open
		 */
		if(this->editor->controls[tagConsole]->bGetValue() < 0.1f && engine.getCState())
			engine.getCState()->getCtrlManager().updateControls();
	}
	/*********************************************************************************************
	 
		Timer callback. Calls the parent's render loop
	 
	 *********************************************************************************************/
	void UIController::Editor::timerCallback()
	{
		statusLabel->updateMessage();
		// force a repaint every second (it wont itself, necessarily, even tho childs are set as dirty!! ugh)
		repaintCallBackCounter++;
		if ((repaintCallBackCounter * getTimerInterval() / 1000.f) > 1)
		{
			//repaint();
			repaintCallBackCounter = 0;
		}
		
		parent.render();
	}

	UIController::~UIController()
	{
	}
	/*********************************************************************************************
	 
		Called whenever the editor is opened and ready for action.
	 
	 *********************************************************************************************/
	void UIController::editorOpened(Editor * newEditor)
	{
		setStatusText();
		if (engine.getCState())
		{
			engine.getCState()->getCtrlManager().attach(newEditor);
			engine.getCState()->getCtrlManager().createPendingControls();
		}
		newEditor->startTimer(engine.getSettings().root()["application"]["ui_refresh_interval"]);
		bFirstDraw = true;
	}
	/*********************************************************************************************
	 
		Called when the editor closes
	 
	 *********************************************************************************************/
	void UIController::editorClosed()
	{
		editor = nullptr;
		if(engine.getCState())
			engine.getCState()->getCtrlManager().setParent(editor);
	}
	/*********************************************************************************************
	 
		Creates an instance of the graphical editor
	 
	 *********************************************************************************************/
	UIController::Editor * UIController::create()
	{
		if (editor)
			console().printLine(CColours::red, "[GUI] : error! Request to create new editor while old one still exists. "
			"Reference to old editor lost!");
		editor = new Editor(*this);

		bool bUseOpenGL(false);
		bUseOpenGL = engine.getSettings().root()["application"]["render_opengl"];

		
		editor->initialize(bUseOpenGL);
		
		setStatusText();
		return editor;
	}
	/*********************************************************************************************

		Requests the editor to show the error line

	 *********************************************************************************************/
	void UIController::setEditorError(int nLine) 
	{
		externEditor->setErrorLine(nLine);
	}

	/*********************************************************************************************

		Set the status text label.

	 *********************************************************************************************/
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
	/*********************************************************************************************

		Push a message for display for x milliseconds

	 *********************************************************************************************/
	void UIController::setStatusText(const std::string & text, CColour colour, int ms)
	{
		if (editor)
		{
			editor->statusLabel->pushMessage(text, colour, ms);
		}
	}
	/*********************************************************************************************

		Set the status text label to what it previously was.

	 *********************************************************************************************/
	void UIController::setStatusText()
	{

		cpl::CMutex lockGuard(this);
		if (editor)
		{
			editor->statusLabel->setDefaultMessage(statusLabel, statusColour);
		}
	}
	/*********************************************************************************************

		Get the status text. Not to be modified!

	 *********************************************************************************************/
	std::string UIController::getStatusText()
	{
		cpl::CMutex lockGuard(this);
		return statusLabel;
	}

	/*********************************************************************************************

		valueChanged - this is the main control logic of the program.
		we control everything through events, and in theory, all code
		should be called from here.

	 *********************************************************************************************/
	bool UIController::valueChanged(CBaseControl * control)
	{
		/*
			This really shouldn't happen (this function is called from editor)	
		*/
		if (!editor)
		{
			console().printLine(CColours::red, "[GUI] : error! Control events received with no editor available!");
			return false;
		}

		long tag = control->bGetTag();
		float value = control->bGetValue();
		switch(tag)
		{
		case kConsoleButton:
			/*
				console button was pressed - add it to the frame viewcontainer.
			*/
			if (value > 0.1f)
				editor->addAndMakeVisible(console().getView());
			else
				editor->removeChildComponent(console().getView());
			break;

		case kCompileButton:

			if(!compilerState.valid()) 
			{
				compilerState = createPlugin(externEditor->getProject());
				engine.disablePlugin(false);
			}
			else
			{
				switch (compilerState.wait_for(std::chrono::seconds(0)))
				{
				case std::future_status::ready:
					compilerState = createPlugin(externEditor->getProject());
					engine.disablePlugin(false);
					break;
				case std::future_status::deferred:
				case std::future_status::timeout:
					control->bSetInternal(0);
					setStatusText("Already compiling, please wait...", CColours::red, 2000);
					console().printLine(CColours::red, "[GUI] : cannot compiler while compiling.");
					break;

				}
			}
			break;

		case kActiveStateButton:
			/*
				Try to (de)activate the plugin.
			*/
				
			if(value > 0.1f) 
			{
				if (compilerState.valid())
				{
					switch (compilerState.wait_for(std::chrono::seconds(0)))
					{
					case std::future_status::ready:
						engine.exchangePlugin(compilerState.get());
						break;
					case std::future_status::deferred:
					case std::future_status::timeout:
						control->bSetInternal(0);
						setStatusText("Cannot activate plugin while compiling...", CColours::red, 2000);
						console().printLine(CColours::red, "[GUI] : cannot activate while compiling.");
						break;

					}
				}

				if (!engine.getCState())
				{
					control->bSetInternal(0);
					setStatusText("No compiled symbols found", CColours::red, 2000);
					console().printLine(CColours::red, "[GUI] : Failure to activate plugin, no compiled code available.");
					return false;
				}

				if (engine.activatePlugin())
				{
					// success
					setStatusText("Plugin activated", CColours::green);
					engine.getCState()->getCtrlManager().attach(editor);
					engine.getCState()->getCtrlManager().createPendingControls();
					engine.getCState()->getCtrlManager().callListeners();
				}
				else
				{
					console().printLine(CColours::red, "[GUI] : Error activating plugin.", 2000);
					setStatusText("Error activating plugin.", CColours::red);
					editor->controls[kActiveStateButton]->bSetValue(0);
				}
			} 
			else 
			{
				setStatusText("Plugin disabled", CColours::lightgoldenrodyellow, 1000);
				engine.getCState()->getCtrlManager().detach();
				engine.disablePlugin(true);
			}
			break;
		case kEditorButton:
			// show the editor
			
			if(value > 0.1f) {
				if(!externEditor->exists())
					cpl::Misc::MsgBox("No code editor available!", cpl::programInfo.programAbbr + " error!");
				externEditor->openEditor();
			}
			else
				externEditor->closeEditor();
			break;

		case kAboutButton:
			// show about box
			about();
			break;
		case tagUseBuffer:
			engine.useProtectedBuffers( value > 0.1f ? true : false );
			if(value > 0.1f)
				console().printLine(CColours::black, "[GUI] : Activated protected buffers.");
			else
				console().printLine(CColours::black, "[GUI] : Disabled protected buffers.");
			break;
		case tagUseFPU:
			auto proxy = value > 0.1f ? true : false;
			if(proxy) {
				auto ret = cpl::Misc::MsgBox("Warning: Enabling floating-point exceptions can result in errornous behaviour, since "
					"most DAW's have concurrent threads not checking exceptions. Resume?", cpl::programInfo.programAbbr,
					cpl::Misc::MsgStyle::sYesNoCancel | cpl::Misc::MsgIcon::iQuestion, this->getSystemWindow(), true);
				if(ret == cpl::Misc::MsgButton::bYes) {
					bUseFPUE = true;
					control->bSetInternal(1.0f);
					console().printLine(CColours::black, "[GUI] : Activated floating-point unit exceptions.");
				}
				else
				{
					bUseFPUE = false;
					control->bSetInternal(0.0f);
				}
			}
			else
			{
				console().printLine(CColours::black, "[GUI] : Disabled floating-point unit exceptions.");
				bUseFPUE = false;
			}
		}
		// sighh
		editor->repaint();
		// tells the control that sent the event that it should handle it itself
		return false;
	}
	void UIController::setParameter(int index, float value)
	{
		if (!editor)
		{
			console().printLine(CColours::red, "[GUI] : Request to alter parameter denied; no editor exists.");
			return;
		}
		auto ctrl = editor->controls[index];
		if (!ctrl)
		{
			console().printLine(CColours::red, "[GUI] : Request to alter parameter denied; index out of bounds.");
			return;
		}
		ctrl->bSetValue(value);
	}
	void UIController::about()
	{
		static std::string sDialogMessage =
			cpl::programInfo.name + " is written by Janus Lynggard Thorborg"
			" in the period of 2012-2017. " + cpl::programInfo.programAbbr + " utilizes "
			"juce as central program structure. All rights reserved to their respective owners,"
			" see /licenses/ for licenses for using this program. Thanks to everyone"
			" that has made this project possible; thanks for the great libraries and I hope"
			" you do enjoy using this program. See more at www.jthorborg.com/index.html?ipage=ape";

		static std::string sTitleMessage =
			"About " + cpl::programInfo.programAbbr + " " + cpl::programInfo.version.toString() + " project";
		cpl::Misc::MsgBox(sDialogMessage, sTitleMessage, cpl::Misc::MsgStyle::sOk | cpl::Misc::MsgIcon::iInfo, getSystemWindow(),true);
	}

	std::future<std::unique_ptr<PluginState>> UIController::createPlugin()
	{
		return createPlugin(externEditor->getProject());
	}


	std::future<std::unique_ptr<PluginState>> UIController::createPlugin(std::unique_ptr<ProjectEx> project)
	{
		if (!project)
		{
			console().printLine(CColours::red, "[GUI] : Compilation error - "
				"invalid project or no text recieved from editor.");
			setStatusText("No code to compile", CColours::red, 3000);
			return {};
		}

		setProjectName(project->projectName);
		setStatusText("Compiling...", CColours::red, 500);

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

					console().printLine(CColours::black, "[GUI] : Compiled successfully (%f ms).", time);
					setStatusText("Compiled OK!", CColours::green, 2000);

				}
				catch (const std::exception& e)
				{
					console().printLine(CColours::red, "[GUI] : Error compiling project (%s: %s).", typeid(e).name(), e.what());
					setStatusText("Error while compiling (see console)!", CColours::red, 5000);
				}

				return ret;
			},
			std::move(project)
		);
	}

	/*********************************************************************************************

		This function is the main passage of events to the user-created controls. This passes the
		events to the core and the subsystem. If the event isn't handled there, the event is passed
		to the default handler of the control.

	 *********************************************************************************************/

};

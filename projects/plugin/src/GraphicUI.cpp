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

#include "GraphicUI.h"
#include "ButtonDefinitions.h"
#include <cpl/Misc.h>
#include "Engine.h"
#include "CState.h"
#include "CConsole.h"
#include "MacroConstants.h"
#include <cpl/CThread.h>
#include "ProjectEx.h"
#include <stdio.h>
#include "CQueueLabel.h"
#include <chrono>
#include "CCodeEditor.h"

namespace APE 
{
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
	GraphicUI::GraphicUI(APE::Engine * effect)
		: console(nullptr), externEditor(nullptr), projectName(cpl::programInfo.programAbbr),
		bIsCompiling(false), editor(nullptr), engine(effect), bIsCompiled(false), bIsActive(effect->status.bActivated),
		bStatusLock(false), bUseBuffers(engine->status.bUseBuffers), bUseFPUE(engine->status.bUseFPUE),
		ctrlNotifier(this), ctrlManager(&ctrlNotifier, CRect(138, 0, 826 - 138, 245), kTagEnd),
		autoSaveCounter(0), bFirstDraw(true), incGraphicCounter(0)
	{
		
		// create console
		console = new CConsole();
		// create the editor
		externEditor = MakeCodeEditor(effect);
		
		// 0.025 is pretty smooth
		clockData.pole = 0.025f;
		clockData.averageClocks = 0;
		clockData.lastSample = engine->clocksPerSample;
		
		// manually load resources in case a host is frantic and calls
		// serializing without initiating out editor (hello reaper bridge)
		CResourceManager::instance().loadResources();
		
		setStatusText("Ready");
	}
	/*********************************************************************************************
	 
		Constructor for the editor. Creates all graphical components. Notifies parent
	 
	 *********************************************************************************************/
	GraphicUI::Editor::Editor(GraphicUI & p)
		: parent(p), AudioProcessorEditor(p.engine),
		repaintCallBackCounter(0), bImage(CResourceManager::getImage("background"))
		//,testImage(juce::Image::PixelFormat::ARGB, 800,300, false, *new juce::OpenGLImageType())
	{
		// get background
		background.setImage(bImage);
		addAndMakeVisible(background);
		// background and sizing off gui
		// everything is sized relative to the background image
		CPoint size(background.getWidth(), background.getHeight());
		setSize(size.x, size.y);

		// add buttons
		CButton * b = nullptr;
		for (int i = 0; i < ArraySize(ButtonDefs); i++)
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
		parent.console->create(CRect(b->getWidth(), 0, getWidth() - b->getWidth(), getHeight() - (getHeight() / 6)));
		
		
		/*
			set buttons according to engine
		*/
		
		controls[kTags::tagActiveState]->bSetInternal(parent.bIsActive ? 1.f : 0.f);
	}
	/*********************************************************************************************
	 
		Destructor. Delets graphic elements, stops timers and notifies parent
	 
	 *********************************************************************************************/
	GraphicUI::Editor::~Editor()
	{
		oglc.detach();
		for (auto garbage : garbageCollection)
			delete garbage;
		parent.console->close();
		if (isTimerRunning())
			stopTimer();
		parent.editorClosed();
	}
	/*********************************************************************************************
	 
		Paint loop for the editor. Calls the paren't rendering loop
	 
	 *********************************************************************************************/
	void GraphicUI::Editor::paint(juce::Graphics & g)
	{
		// draw background image
		//g.drawImage(background, 0, 0, getWidth(), getHeight(), 0, 0, background.getWidth(), background.getHeight());
		parent.render();
	}
	void GraphicUI::Editor::initialize(bool useOpenGL)
	{
		if(useOpenGL)
			oglc.attachTo(*this);
		
		
		parent.editorOpened(this);	
	}
	/*********************************************************************************************
	 
		Updates the info label with new data.
	 
	 *********************************************************************************************/
	void GraphicUI::updateInfoLabel()
	{
		if (!editor)
		{
			console->printLine(CColours::red, "[GUI] : error! Request to update clock counter from invalid editor!");
		}
		
		clockData.lastSample = engine->clocksPerSample;
		clockData.averageClocks = clockData.averageClocks + clockData.pole * (clockData.lastSample - clockData.averageClocks);
		char buf[200];
		
		sprintf_s(buf, "Instance %d - accps: ~%d (%d)",
				  engine->instanceID.instanceCounter,
				  (unsigned)clockData.averageClocks,
				  clockData.lastSample);;
		
		editor->infoLabel->setText(buf);
	}
	
	/*********************************************************************************************
	 
		Renders any objects marked as dirty. Calls autosave regularly
	 
	 *********************************************************************************************/
	void GraphicUI::render()
	{
		incGraphicCounter++;
		int autoSaveInterval = engine->autoSaveInterval;
		if (autoSaveInterval)
		{
			autoSaveCounter++;
			if ((autoSaveCounter * engine->uiRefreshInterval / 1000.f) > autoSaveInterval)
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
		if(this->editor->controls[tagConsole]->bGetValue() < 0.1f)
			ctrlManager.updateControls();
	}
	/*********************************************************************************************
	 
		Timer callback. Calls the parent's render loop
	 
	 *********************************************************************************************/
	void GraphicUI::Editor::timerCallback()
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
	/*********************************************************************************************

		Destructor, cleans up memory. Waits for any compilation. 

	 *********************************************************************************************/
	GraphicUI::~GraphicUI()
	{
		cpl::Misc::SpinLock(10000, bIsCompiling);
	}
	/*********************************************************************************************
	 
		Called whenever the editor is opened and ready for action.
	 
	 *********************************************************************************************/
	void GraphicUI::editorOpened(Editor * newEditor)
	{
		setStatusText();
		ctrlManager.attach(newEditor);
		ctrlManager.createPendingControls();
		newEditor->startTimer(engine->uiRefreshInterval);
		bFirstDraw = true;
	}
	/*********************************************************************************************
	 
		Called when the editor closes
	 
	 *********************************************************************************************/
	void GraphicUI::editorClosed()
	{
		editor = nullptr;
		ctrlManager.setParent(editor);
	}
	/*********************************************************************************************
	 
		Creates an instance of the graphical editor
	 
	 *********************************************************************************************/
	GraphicUI::Editor * GraphicUI::create()
	{
		if (editor)
			console->printLine(CColours::red, "[GUI] : error! Request to create new editor while old one still exists. "
			"Reference to old editor lost!");
		editor = new Editor(*this);
		bool bUseOpenGL(false);
		try {
			bUseOpenGL = engine->getRootSettings()["application"]["render_opengl"];
			
		} catch (const std::exception & e)
		{
			console->printLine(CColours::red, "[GUI] : Error reading graphic settings (%s)", e.what());
		}
		
		editor->initialize(bUseOpenGL);
		
		setStatusText();
		return editor;
	}
	/*********************************************************************************************

		Requests the editor to show the error line

	 *********************************************************************************************/
	void GraphicUI::setEditorError(int nLine) 
	{
		externEditor->setErrorLine(nLine);
	}

	/*********************************************************************************************

		Set the status text label.

	 *********************************************************************************************/
	void GraphicUI::setStatusText(const std::string & text, CColour colour)
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
	void GraphicUI::setStatusText(const std::string & text, CColour colour, int ms)
	{
		if (editor)
		{
			editor->statusLabel->pushMessage(text, colour, ms);
		}
	}
	/*********************************************************************************************

		Set the status text label to what it previously was.

	 *********************************************************************************************/
	void GraphicUI::setStatusText()
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
	std::string GraphicUI::getStatusText()
	{
		cpl::CMutex lockGuard(this);
		return statusLabel;
	}

	/*********************************************************************************************

		valueChanged - this is the main control logic of the program.
		we control everything through events, and in theory, all code
		should be called from here.

	 *********************************************************************************************/
	bool GraphicUI::valueChanged(CBaseControl * control)
	{
		/*
			This really shouldn't happen (this function is called from editor)	
		*/
		if (!editor)
		{
			console->printLine(CColours::red, "[GUI] : error! Control events received with no editor available!");
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
				editor->addAndMakeVisible(console->getView());
			else
				editor->removeChildComponent(console->getView());
			break;

		case kCompileButton:
			/*
				compilebutton was pressed - compiled whatever we can find.
				start this in a new thread because we can't know if it hangs or whatever.
			*/

			if(!bIsCompiling) {
				bIsCompiling = true;
				if(bIsCompiled) {
					engine->disablePlugin(false);
					ctrlManager.reset();
				}
				cpl::CThread compileThread(GraphicUI::startCompilation);
				compileThread.run(engine);
			}
			break;

		case kActiveStateButton:
			/*
				Try to (de)activate the plugin.
			*/
				
			if(value > 0.1f) {
				if(bIsCompiling) {
					control->bSetInternal(0);
					setStatusText("Cannot activate plugin while compiling...", CColours::red, 2000);
					console->printLine(CColours::red, "[GUI] : cannot activate while compiling.");
				} else {
					if(!bIsCompiled) {
						control->bSetInternal(0);
						setStatusText("No compiled symbols found", CColours::red, 2000);
						console->printLine(CColours::red, "[GUI] : Failure to activate plugin, no compiled code available.");
					} else {
						if(engine->activatePlugin()) {
							// success
							setStatusText("Plugin activated", CColours::green);
							ctrlManager.createPendingControls();
							ctrlManager.callListeners();
						} else {
							console->printLine(CColours::red, "[GUI] : Error activating plugin.", 2000);
							setStatusText("Error activating plugin.", CColours::red);
							editor->controls[kActiveStateButton]->bSetValue(0);
						}
						
					}
				}
			} else {
				setStatusText("Plugin disabled", CColours::lightgoldenrodyellow, 1000);
				engine->disablePlugin(true);
				ctrlManager.reset();
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
			engine->useProtectedBuffers( value > 0.1f ? true : false );
			if(value > 0.1f)
				console->printLine(CColours::black, "[GUI] : Activated protected buffers.");
			else
				console->printLine(CColours::black, "[GUI] : Disabled protected buffers.");
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
					console->printLine(CColours::black, "[GUI] : Activated floating-point unit exceptions.");
				}
				else
				{
					bUseFPUE = false;
					control->bSetInternal(0.0f);
				}
			}
			else
			{
				console->printLine(CColours::black, "[GUI] : Disabled floating-point unit exceptions.");
				bUseFPUE = false;
			}
		}
		// sighh
		editor->repaint();
		// tells the control that sent the event that it should handle it itself
		return false;
	}
	void GraphicUI::setParameter(int index, float value)
	{
		if (!editor)
		{
			console->printLine(CColours::red, "[GUI] : Request to alter parameter denied; no editor exists.");
			return;
		}
		auto ctrl = editor->controls[index];
		if (!ctrl)
		{
			console->printLine(CColours::red, "[GUI] : Request to alter parameter denied; index out of bounds.");
			return;
		}
		ctrl->bSetValue(value);
	}
	void GraphicUI::about()
	{
		static std::string sDialogMessage =
			cpl::programInfo.name + " is written by " _PROGRAM_AUTHOR
			" in the period of " _TIME_OF_WRITING ". " + cpl::programInfo.programAbbr + " utilizes "
			_HOST_TARGET_TECH " as central program structure. All rights reserved to their respective owners,"
			" see /licenses/ for licenses for using this program. Thanks to everyone"
			" that has made this project possible; thanks for the great libraries and I hope"
			" you do enjoy using this program." _HOMEPAGE_SENTENCE _ADDITIONAL_NOTES;

		static std::string sTitleMessage =
			"About " + cpl::programInfo.programAbbr + " " + cpl::programInfo.version.toString() + " project";
		cpl::Misc::MsgBox(sDialogMessage, sTitleMessage, cpl::Misc::MsgStyle::sOk | cpl::Misc::MsgIcon::iInfo, getSystemWindow(),true);
	}
	/*********************************************************************************************

		This is where we pass the contents to the c compiler.
		Fired _only_ from valueChanged.
		This function assumes a clean slate; that is - no plugin code must be running and the last
		project must have been cleaned up.

	 *********************************************************************************************/
	void * GraphicUI::startCompilation(void * effectPointer)
	{

		Engine * _this = reinterpret_cast<Engine*>(effectPointer);

		// messages should be explanatory
		if(!_this)
			return 0;
		APE::GraphicUI * _gui = _this->getGraphicUI();
		auto start = std::chrono::high_resolution_clock::now();

		APE::ProjectEx * project = _gui->externEditor->getProject();
		if(!project)
		{
			_gui->console->printLine(CColours::red, "[GUI] : Compilation error - "
				"invalid project or no text recieved from editor.");
			_gui->setStatusText("No code to compile", CColours::red, 3000);
			_gui->bIsCompiling = false;
			_gui->bIsCompiled = false;
			return 0;
		}


		_gui->setProjectName(project->projectName);
		_gui->setStatusText("Compiling...", CColours::red, 500);
		_this->csys->setNewProject(project);

		if(_this->csys->compileCurrentProject()) {
			auto delta = std::chrono::high_resolution_clock::now() - start;
			auto time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(delta);
			_gui->console->printLine(CColours::black, "[GUI] : Compiled successfully (%f ms).", time);
			_gui->setStatusText("Compiled OK!", CColours::green, 2000);
			_gui->bIsCompiling = false;
			_gui->bIsCompiled = true;
		}
		else
		{
			_gui->console->printLine(CColours::red, "[GUI] : Error compiling project.");
			_gui->setStatusText("Error while compiling (see console)!", CColours::red, 5000);
			_gui->bIsCompiling = false;
			_gui->bIsCompiled = false;
		}
		
		return (void*)1;
	}

	/*********************************************************************************************

		This function is the main passage of events to the user-created controls. This passes the
		events to the core and the subsystem. If the event isn't handled there, the event is passed
		to the default handler of the control.

	 *********************************************************************************************/
	bool GraphicUI::CCtrlDelegateListener::valueChanged(CBaseControl * control)
	{
		if(!control) 
		{
			parent->console->printLine(CColours::red,
				"[GUI::ctrlNotifier] Error: ctrlNotifier was notified of an event, it couldn't handle (control isn't owned!)");
			return false;
		}

		if(STATUS_HANDLED != parent->engine->onCtrlEvent(control)) 
		{
			// event was not handled by core/plugin, default to controls' own handler.
			return false;
		}
		control->bRedraw();
		return true;
	}
};

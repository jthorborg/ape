/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2018 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:MainEditor.cpp
		
		Implementation of the primary editor

*************************************************************************************/

#include "MainEditor.h"
#include "../UIController.h"
#include "../CQueueLabel.h"
#include <string>
#include "../Engine.h"
#include "../CConsole.h"
#include "../PluginState.h"

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


	Editor::Editor(UIController& p)
		: parent(p)
		, AudioProcessorEditor(p.engine)
		, repaintCallBackCounter(0), bImage(CResourceManager::getImage("background"))
		, scope(p.engine.getOscilloscopeData())
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
			b = new CButton(ButtonDefs[i].toggled, ButtonDefs[i].untoggled, this);
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
		toggle->bSetListener(this);
		toggle->bSetText("Use protected buffers");
		toggle->bSetTag(kTags::tagUseBuffer);
		garbageCollection.push_back(toggle);
		addAndMakeVisible(toggle);
		// add exceptions toggle
		toggle = new CToggle();
		toggle->bSetSize(CRect(b->getWidth() + 200, getHeight() - 20, 200, 20));
		if (parent.bUseFPUE)
			toggle->bSetValue(1);
		toggle->bSetListener(this);
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

	Editor::~Editor()
	{
		oglc.detach();
		for (auto garbage : garbageCollection)
			delete garbage;
		parent.console().close();
		if (isTimerRunning())
			stopTimer();
		parent.editorClosed();
	}

	bool Editor::valueChanged(CBaseControl* control)
	{
		using Cmd = UIController::Commands;
		bool toggled = control->bGetValue() > 0.5f;
		bool result = toggled;

		switch (control->bGetTag())
		{
		case tagConsole: 
			if(toggled)
				addAndMakeVisible(parent.console().getView());
			else
				removeChildComponent(parent.console().getView());
			break;
		case tagCompile: 
			parent.performCommand(Cmd::Recompile); 
			break;
		case tagActiveState: 
		{
			if (!toggled)
			{
				if (parent.performCommand(Cmd::Deactivate))
					control->bSetInternal(0.0f);
			}
			else
			{
				if (parent.performCommand(Cmd::Activate))
					control->bSetInternal(1.0f);
			}
			break;
		}
		case tagEditor: 
		{
			if (toggled)
			{
				if (parent.performCommand(Cmd::OpenSourceEditor))
					control->bSetInternal(1.0f);
			}
			else
			{
				if (parent.performCommand(Cmd::CloseSourceEditor))
					control->bSetInternal(0.0f);
			}
			break;
		}
		case tagAbout: 
			about(); 
			break;

		}

		// TODO: Remove
		repaint();
		return false;
	}

	void Editor::onPluginStateChanged(PluginState& plugin, bool activated)
	{
		if (activated)
		{
			plugin.getCtrlManager().attach(this);
			plugin.getCtrlManager().createPendingControls();
			plugin.getCtrlManager().callListeners();
			controls[tagActiveState]->bSetInternal(1.0f);
		}
		else
		{
			plugin.getCtrlManager().detach();
			controls[tagActiveState]->bSetInternal(0.0f);

		}
	}


	void Editor::paint(juce::Graphics & g)
	{
		parent.render();
	}

	void Editor::initialize(bool useOpenGL)
	{
		if(useOpenGL)
			oglc.attachTo(*this);
		
		parent.editorOpened(this);	
	}


	void Editor::timerCallback()
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

	void Editor::about()
	{
		static std::string sDialogMessage =
			cpl::programInfo.name + " is written by Janus Lynggard Thorborg"
			" in the period of 2012-2018. " + cpl::programInfo.programAbbr + " utilizes "
			"juce as central program structure. All rights reserved to their respective owners,"
			" see /licenses/ for licenses for using this program. Thanks to everyone"
			" that has made this project possible; thanks for the great libraries and I hope"
			" you do enjoy using this program. See more at www.jthorborg.com/index.html?ipage=ape";

		static std::string sTitleMessage =
			"About " + cpl::programInfo.programAbbr + " " + cpl::programInfo.version.toString() + " project";

		cpl::Misc::MsgBox(sDialogMessage, sTitleMessage, cpl::Misc::MsgStyle::sOk | cpl::Misc::MsgIcon::iInfo, this->getWindowHandle(), true);
	}

}

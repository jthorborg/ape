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
#include "../Plugin/PluginSurface.h"
#include "../SignalizerWindow.h"
#include <dockable-windows\Source\MainComponent.h>

namespace ape
{
	using namespace std::string_literals;

	struct ButtonDefinition { const char * untoggled, *toggled; bool sticky; };

	constexpr int ButtonsColumnSpace = 100;
	constexpr int NumButtons = 3;

	std::array<ButtonDefinition, NumButtons> ButtonDefs {
		{
			{ "Compile", "Compiling...", true },
			{ "Activate", "Deactivate", true },
			{ "Show editor", "Hide editor", true },
		}
	};

	class DockWindow : public juce::DocumentWindow
	{
	public:

		DockWindow(MainEditor& parent, juce::Component& child)
			: juce::DocumentWindow(child.getName(), juce::Colours::red, juce::DocumentWindow::allButtons)
			, editor(parent)
			, child(child)
		{

		}

		void closeButtonPressed() override
		{
			editor.dockManager.attachComponentToDock(child, editor.tabs);
		}

	private:

		MainEditor& editor;
		juce::Component& child;
	};

	MainEditor::MainEditor(UIController& p)
		: CTopView(this, "APE editor")
		, parent(p)
		, state(p.getUICommandState())
		, AudioProcessorEditor(p.engine)
		, repaintCallBackCounter(0)
		, compilation(&state.compile)
		, activation(&state.activationState)
		, editor(&state.editor)
		, tabs(dockManager)
	{
		dockManager.setHeavyWeightGenerator([this](auto& c) { return std::make_unique<DockWindow>(*this, c); });
		consoleWindow = parent.console().create();
		tabs.addComponentToDock(consoleWindow.get());
		
		scopeWindow = p.engine.getOscilloscopeData().createWindow();
		scopeSettingsWindow = p.engine.getOscilloscopeData().createEditor();

		tabs.addComponentToDock(scopeWindow.get());
		tabs.addComponentToDock(scopeSettingsWindow.get());

		int i = 0;
		for (auto button : { &compilation, &activation, &editor })
		{
			button->setTexts(ButtonDefs[i].untoggled, ButtonDefs[i].toggled);
			button->setToggleable(ButtonDefs[i].sticky);
			addAndMakeVisible(button);
			i++;
		}

		for (auto value : { &state.console, &state.scope })
			value->addListener(this);

		/*
		if (!approot["greeting_shown"])
		{
			cpl::Misc::MsgBox("Hello and welcome to " + cpl::programInfo.name + "! Before you start using this program, "
				"please take time to read the readme and agree to all licenses + disclaimers found in /licenses. "
				"Have fun!", cpl::programInfo.name, cpl::Misc::MsgIcon::iInfo);
			approot["greeting_shown"] = true;
		} */

		// get background
		//addAndMakeVisible(background);
		// background and sizing off gui
		// everything is sized relative to the background image
		CPoint size(800, 300);
		setSize(size.x, size.y);


		// labels
		infoLabel = new CTextControl();
		infoLabel->setBounds(ButtonsColumnSpace + 5, getHeight() - 40, 220, 20);
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

		tabs.setBounds(getContentArea());
		addAndMakeVisible(tabs);

	}


	MainEditor::~MainEditor()
	{
		for (auto value : { &state.console, &state.scope })
			value->removeListener(this);

		oglc.detach();
		for (auto garbage : garbageCollection)
			delete garbage;
		if (isTimerRunning())
			stopTimer();
		parent.editorClosed();

		scopeWindow = nullptr;
	}

	juce::Rectangle<int> MainEditor::getContentArea()
	{
		return getLocalBounds().withLeft(ButtonsColumnSpace).withBottom(getHeight() - 40);
	}

	void MainEditor::valueEntityChanged(cpl::ValueEntityBase::Listener * sender, cpl::ValueEntityBase * value)
	{
		bool toggled = value->getNormalizedValue() > 0.5f;

	}

	juce::Component * MainEditor::getWindow()
	{
		return this;
	}

	void MainEditor::onPluginStateChanged(PluginState& plugin, bool activated)
	{
		if (pluginSurface)
			tabs.removeChildComponent(pluginSurface.get());

		if (activated)
		{
			pluginSurface = plugin.getOrCreateSurface();
			pluginSurface->setBounds(getBounds());
			tabs.addComponentToDock(pluginSurface.get());
		}
		else
		{
			pluginSurface = nullptr;
		}
	}

	void MainEditor::resized()
	{
		int i = 0;
		auto heightPerButton = getHeight() / NumButtons;
		auto y = 0;
		for (auto button : { &compilation, &activation, &editor })
		{
			button->setBounds(0, y, ButtonsColumnSpace, heightPerButton);
			y += heightPerButton;
		}
	}


	void MainEditor::paint(juce::Graphics & g)
	{
		juce::ColourGradient gradient(juce::Colours::black, 0, 0, juce::Colour(37, 3, 55), getWidth() * 0.5f, getHeight() * 2, false);
		g.setGradientFill(gradient);
		g.fillAll();

	}

	void MainEditor::initialize(bool useOpenGL)
	{
		if(useOpenGL)
			oglc.attachTo(*this);
		
		parent.editorOpened(this);	
	}


	void MainEditor::timerCallback()
	{
		statusLabel->updateMessage();
		// force a repaint every second (it wont itself, necessarily, even tho childs are set as dirty!! ugh)
		repaintCallBackCounter++;
		if ((repaintCallBackCounter * getTimerInterval() / 1000.f) > 1)
		{
			//repaint();
			repaintCallBackCounter = 0;
		}
	
		if (pluginSurface)
			pluginSurface->repaintActiveAreas();

		parent.render();
	}

	void MainEditor::about()
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

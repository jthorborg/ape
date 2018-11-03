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
#include "../UI/DockWindow.h"
#include "../CodeEditor/SourceManager.h"

namespace ape
{
	using namespace std::string_literals;
	using Rect = juce::Rectangle<int>;
	constexpr int BottomSpace = 50;

	MainEditor::MainEditor(UIController& p)
		: CTopView(this, "APE editor")
		, parent(p)
		, state(p.getUICommandState())
		, AudioProcessorEditor(p.engine)
		, repaintCallBackCounter(0)
		, compilation(state.compile, state.activationState)
		, activation(state.activationState)
		, dockManager(true, false)
		, tabs(dockManager)
		, infoLabel(std::make_unique<CTextControl>())
		, statusLabel(std::make_unique<CQueueLabel>())
		, rcc(this, nullptr)
	{
		dockManager.setHeavyWeightGenerator(
			[this](auto& c) 
			{
				auto window = &c == codeWindow.get() ? parent.getSourceManager().createSuitableCodeEditorWindow() : std::make_unique<DockWindow>();
				window->setPrefix("APE " + std::to_string(parent.engine.instanceCounter()));
				window->injectDependencies(*this, c);
				return std::unique_ptr<juce::ResizableWindow>(window.release());
			}
		);

		codeWindow = parent.getSourceManager().createCodeEditorComponent();

		consoleWindow = parent.console().create();
		tabs.addComponentToDock(consoleWindow.get());
		
		scopeWindow = p.engine.getOscilloscopeData().createWindow();
		scopeSettingsWindow = p.engine.getOscilloscopeData().createEditor();

		tabs.addComponentToDock(scopeWindow.get());
		tabs.addComponentToDock(scopeSettingsWindow.get());
		tabs.addComponentToDock(codeWindow.get());

		addAndMakeVisible(activation);

		addAndMakeVisible(compilation);

		/*
		if (!approot["greeting_shown"])
		{
			cpl::Misc::MsgBox("Hello and welcome to " + cpl::programInfo.name + "! Before you start using this program, "
				"please take time to read the readme and agree to all licenses + disclaimers found in /licenses. "
				"Have fun!", cpl::programInfo.name, cpl::Misc::MsgIcon::iInfo);
			approot["greeting_shown"] = true;
		} */

		// labels
		infoLabel->setColour(CColours::lightgoldenrodyellow);
		infoLabel->setFontSize(TextSize::smallText);
		addAndMakeVisible(*infoLabel);

		statusLabel->setFontSize(TextSize::largeText);
		statusLabel->setJustification(juce::Justification::centredRight);
		statusLabel->setColour(CColours::lightgoldenrodyellow);
		addAndMakeVisible(*statusLabel);

		addAndMakeVisible(tabs);
		addAndMakeVisible(rcc);
		setSize(800, 300);
	}

	MainEditor::~MainEditor()
	{
		oglc.detach();

		if (isTimerRunning())
			stopTimer();

		parent.editorClosed();

		scopeWindow = nullptr;
	}

	juce::Rectangle<int> MainEditor::getContentArea()
	{
		return getLocalBounds().withBottom(getHeight() - BottomSpace - 1);
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
		constexpr int size = BottomSpace - 10;

		auto buttonRect = Rect(5, getHeight() - BottomSpace + 5, size, size);

		compilation.setBounds(buttonRect);
		buttonRect.translate(size + 10, 0);

		activation.setBounds(buttonRect);

		infoLabel->setBounds(buttonRect.getRight() + 20, getHeight() - BottomSpace + 30, 220, 20);
		statusLabel->setBounds(Rect(getWidth() - 300, getHeight() - 25, 280, 20));

		tabs.setBounds(getContentArea());
		rcc.setBounds(getLocalBounds().withLeft(getWidth() - BottomSpace * 0.33f).withTop(getHeight() - BottomSpace * 0.33f));
	}


	void MainEditor::paint(juce::Graphics & g)
	{
		constexpr int size = BottomSpace - 10;

		juce::ColourGradient backgroundGradient(juce::Colours::black, 0, 0, juce::Colour(37, 3, 55), getWidth() * 0.5f, getHeight() * 2, false);
		juce::ColourGradient lineGradient(juce::Colours::lightgoldenrodyellow, 0, getHeight() - BottomSpace, juce::Colours::lightgoldenrodyellow.withAlpha(0.0f), getWidth(), getHeight(), false);

		g.setGradientFill(backgroundGradient);
		g.fillAll();
		g.setGradientFill(lineGradient);
		g.drawLine(0, getHeight() - 50, getWidth(), getHeight() - 50, 2);
		g.drawLine(size * 2 + 5 * 4, getHeight() - BottomSpace, size * 2 + 5 * 4, getHeight(), 2);

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

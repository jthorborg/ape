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
 
	 file:SourceProjectManager.cpp
	 
		Implementation of the juce editor
 
 *************************************************************************************/


#include "JuceEditorWindow.h"

namespace ape
{
	namespace fs = std::experimental::filesystem;

	#ifdef __MAC__
		#define CTRLCOMMANDKEY juce::ModifierKeys::Flags::commandModifier
	#else
		#define CTRLCOMMANDKEY juce::ModifierKeys::Flags::ctrlModifier
	#endif

	const MenuEntry CommandTable[][6] =
	{
		// File
		{
			{"",			0,		0,				Command::InvalidCommand}, // dummy element - commands are 1-based index cause of juce
			{ "New File",	'n',	CTRLCOMMANDKEY, Command::FileNew },
			{ "Open...",	'o',	CTRLCOMMANDKEY, Command::FileOpen },
			{ "Save",		's',	CTRLCOMMANDKEY,	Command::FileSave },
			{ "Save As...",	0,		0,				Command::FileSaveAs },
			{ "Exit",		0,		0,				Command::FileExit}
		},
		// Edit

	};

	JuceEditorWindow::JuceEditorWindow(juce::CodeDocument& cd)
		: DocumentWindow(cpl::programInfo.name + " editor", juce::Colours::white, DocumentWindow::TitleBarButtons::allButtons)
		, cec(cd, &tokeniser)
		, appCM(nullptr)
	{
		cec.setVisible(true);
		setMenuBar(this);
		setResizable(true, true);
		setBounds(100, 100, 400, 400);
		setUsingNativeTitleBar(true);
		setContentNonOwned(&cec, false);
		cec.setLineNumbersShown(true);
	}

	JuceEditorWindow::~JuceEditorWindow()
	{
		setMenuBar(nullptr);
	}

	juce::PopupMenu JuceEditorWindow::getMenuForIndex(int topLevelMenuIndex, const juce::String & menuName)
	{
		juce::PopupMenu ret;

		switch (topLevelMenuIndex)
		{
			case Menus::File:
			{
				for (int i = Command::Start; i < Command::End; ++i)
					ret.addCommandItem(appCM, i);
				break;
			}
			case Menus::Edit:
			{

				break;
			}
		}
		return ret;
	}

	void JuceEditorWindow::menuItemSelected(int menuItemID, int topLevelMenuIndex) { }

	void JuceEditorWindow::resized()
	{
		/*
			temporary code: when the switch is done to setContentOwned() (see JuceEditorWindow::JuceEditorWindow),
			resize() shouldn't be overloaded anymore - ResizableWindow automagically resizes child
			components.
		*/
		// call our resizing firstly
		DocumentWindow::resized();
	}


	juce::StringArray JuceEditorWindow::getMenuBarNames()
	{
		juce::StringArray ret;
		ret.add("File");
		//ret.add("Edit");
		return ret;
	}

	void JuceEditorWindow::closeButtonPressed()
	{
		setVisible(false);
	}

	void JuceEditorWindow::setAppCM(juce::ApplicationCommandManager* acm)
	{
		// set the application command manager that is associated with this editorWindow
		appCM = acm;
		if (appCM)
			addKeyListener(appCM->getKeyMappings());
	}

}
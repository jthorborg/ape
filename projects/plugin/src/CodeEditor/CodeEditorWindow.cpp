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


#include "CodeEditorWindow.h"

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

	CodeEditorWindow::CodeEditorWindow(juce::CodeDocument& cd)
		: DocumentWindow(cpl::programInfo.name + " editor", juce::Colours::grey, DocumentWindow::TitleBarButtons::allButtons)
		, codeEditor(cd)
		, appCM(nullptr)
	{
		codeEditor.setVisible(true);
		setMenuBar(this);
		setResizable(true, true);
		setUsingNativeTitleBar(true);
		setContentNonOwned(&codeEditor, false);

		setBounds(getBounds().withPosition(100, 100));
	}

	void CodeEditorWindow::paint(juce::Graphics& g)
	{
		g.fillAll({ 0x1E, 0x1E, 0x1E });
	}


	CodeEditorWindow::~CodeEditorWindow()
	{
		setMenuBar(nullptr);
	}

	juce::PopupMenu CodeEditorWindow::getMenuForIndex(int topLevelMenuIndex, const juce::String & menuName)
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

	void CodeEditorWindow::menuItemSelected(int menuItemID, int topLevelMenuIndex) { }


	juce::StringArray CodeEditorWindow::getMenuBarNames()
	{
		juce::StringArray ret;
		ret.add("File");
		//ret.add("Edit");
		return ret;
	}

	void CodeEditorWindow::closeButtonPressed()
	{
		setVisible(false);
	}

	void CodeEditorWindow::setAppCM(juce::ApplicationCommandManager* acm)
	{
		// set the application command manager that is associated with this editorWindow
		appCM = acm;
		if (appCM)
			addKeyListener(appCM->getKeyMappings());
	}

}
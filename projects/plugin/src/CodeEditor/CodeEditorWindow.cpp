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

	const MenuEntry CommandTable[][5] =
	{
		// File
		{
			{"",			0,		0,				SourceManagerCommand::InvalidCommand}, // dummy element - commands are 1-based index cause of juce
			{ "New File",	'n',	CTRLCOMMANDKEY, SourceManagerCommand::FileNew },
			{ "Open...",	'o',	CTRLCOMMANDKEY, SourceManagerCommand::FileOpen },
			{ "Save",		's',	CTRLCOMMANDKEY,	SourceManagerCommand::FileSave },
			{ "Save As...",	0,		0,				SourceManagerCommand::FileSaveAs },
		},
		// Edit

	};

	CodeEditorWindow::CodeEditorWindow(const Settings& setting)
		: appCM(nullptr)
	{
		setMenuBar(this);
		setResizable(true, true);
		setUsingNativeTitleBar(true);

		auto x = setting.lookUpValue(100, "editor", "x_offset");
		auto y = setting.lookUpValue(100, "editor", "y_offset");
		auto width = setting.lookUpValue(800, "editor", "width");
		auto height = setting.lookUpValue(900, "editor", "height");

		setBackgroundColour(setting.lookUpValue(juce::Colour{ 0x1E, 0x1E, 0x1E }, "editor", "colours", "background"));

		setBounds(x, y, width, height);
	}


	CodeEditorWindow::~CodeEditorWindow()
	{
		setMenuBar(nullptr);
		notifyDestruction();
	}

	juce::PopupMenu CodeEditorWindow::getMenuForIndex(int topLevelMenuIndex, const juce::String & menuName)
	{
		juce::PopupMenu ret;

		switch (topLevelMenuIndex)
		{
			case Menus::File:
			{
				for (int i = SourceManagerCommand::Start; i < SourceManagerCommand::End; ++i)
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

	void CodeEditorWindow::menuItemSelected(int menuItemID, int topLevelMenuIndex) 
	{ 
	
	}


	juce::StringArray CodeEditorWindow::getMenuBarNames()
	{
		juce::StringArray ret;
		ret.add("File");
		//ret.add("Edit");
		return ret;
	}

	void CodeEditorWindow::setAppCM(juce::ApplicationCommandManager* acm)
	{
		// set the application command manager that is associated with this editorWindow
		appCM = acm;
		if (appCM)
			addKeyListener(appCM->getKeyMappings());
	}

}
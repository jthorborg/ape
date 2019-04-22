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

	const MenuEntry CommandTable[] =
	{
		// File
		{"",					0,		0,				SourceManagerCommand::InvalidCommand}, // dummy element - commands are 1-based index cause of juce
		{ "New File",			'n',	CTRLCOMMANDKEY, SourceManagerCommand::FileNew },
		{ "New from Template",	0,		0,				SourceManagerCommand::FileNewFromTemplate },
		{ "Open...",			'o',	CTRLCOMMANDKEY, SourceManagerCommand::FileOpen },
		{ "Save",				's',	CTRLCOMMANDKEY,	SourceManagerCommand::FileSave },
		{ "Save As...",			0,		0,				SourceManagerCommand::FileSaveAs },
		{ "Open home...",		0,		0,				SourceManagerCommand::FileOpenScriptsHome },

		{ "Edit externally",	juce::KeyPress::F10Key,	0, SourceManagerCommand::EditExternally },

		// Edit
		// Build
		{ "Compile",			juce::KeyPress::F7Key,	0, SourceManagerCommand::BuildCompile },
		{ "Compile and Run",	juce::KeyPress::F5Key,	0, SourceManagerCommand::BuildCompileAndActivate },
		{ "Activate",			juce::KeyPress::F3Key,	0, SourceManagerCommand::BuildActivate },
		{ "Deactivate",			juce::KeyPress::F4Key,	0, SourceManagerCommand::BuildDeactivate },
		{ "Clean",				juce::KeyPress::F8Key,	0,	SourceManagerCommand::BuildClean },
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
				for (int i = SourceManagerCommand::FileStart; i < SourceManagerCommand::FileEnd; ++i)
					ret.addCommandItem(appCM, i);
				break;
			}

			case Menus::Edit:
			{
				for (int i = SourceManagerCommand::EditStart; i < SourceManagerCommand::EditEnd; ++i)
					ret.addCommandItem(appCM, i);
				break;
			}
			case Menus::Build:
			{
				for (int i = SourceManagerCommand::BuildStart; i < SourceManagerCommand::BuildEnd; ++i)
					ret.addCommandItem(appCM, i);
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
		ret.add("Edit");
		ret.add("Build");

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
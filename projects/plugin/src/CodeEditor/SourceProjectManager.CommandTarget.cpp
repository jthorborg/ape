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
 
	 file:SourceProjectManager.CommandTarget.cpp
	 
		Functionality for commands on the code editor
 
 *************************************************************************************/


#include "SourceProjectManager.h"
#include "../UIController.h"
#include "../CConsole.h"

namespace ape
{
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
        { "Open recent...",		0,	    0,              SourceManagerCommand::FileOpenRecent },
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


	void SourceProjectManager::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo & result)
	{
		auto & cDesc = CommandTable[commandID];
		juce::ApplicationCommandInfo aci(cDesc.command);
		if(userHotKeys[commandID].size())
		{
			aci.defaultKeypresses.add(juce::KeyPress::createFromDescription(userHotKeys[commandID]));
		}
		else
			aci.addDefaultKeypress(cDesc.key, cDesc.modifier);
		aci.shortName = cDesc.name;
		aci.flags = 0;
		aci.setActive(true);
		result = aci;
	}

	void SourceProjectManager::getAllCommands(juce::Array<juce::CommandID> & commands)
	{
		for (int c = SourceManagerCommand::Start; c < SourceManagerCommand::End; ++c)
		{
			commands.add(c);
		}
	}

	bool SourceProjectManager::perform(const InvocationInfo & info)
	{
		switch (info.commandID)
		{
		case SourceManagerCommand::FileNew:
			if (saveIfUnsure() != cpl::Misc::MsgButton::bCancel) 
			{
				newDocument();
			}
			break;

		case SourceManagerCommand::FileOpen:
			if (saveIfUnsure() != cpl::Misc::MsgButton::bCancel) 
			{
				openAFile();
			}
			break;

        case SourceManagerCommand::FileOpenRecent:
            CPL_RUNTIME_EXCEPTION("unreachable");
            break;

		case SourceManagerCommand::FileSave:
			saveCurrentFile();
			break;

		case SourceManagerCommand::FileSaveAs:
			saveAs();
			break;

		case SourceManagerCommand::FileNewFromTemplate:
			if (saveIfUnsure() != cpl::Misc::MsgButton::bCancel)
			{
				if (!openTemplate())
				{
					cpl::Misc::MsgBox(
						"Error opening template, check path in settings", 
						cpl::programInfo.name, 
						cpl::Misc::MsgIcon::iInfo | cpl::Misc::MsgStyle::sOk,
						getParentWindow(), 
						true
					);
				}
			}
			break;

		case SourceManagerCommand::FileOpenScriptsHome:
			openHomeDirectory();
			break;

		case SourceManagerCommand::EditExternally:
			editExternally();
			break;

		case SourceManagerCommand::BuildCompile:
			controller.recompile(false);
			break;

		case SourceManagerCommand::BuildCompileAndActivate:
			controller.recompile(true);
			break;

		case SourceManagerCommand::BuildActivate:
			controller.performCommand(UICommand::Activate);
			break;

		case SourceManagerCommand::BuildDeactivate:
			controller.performCommand(UICommand::Deactivate);
			break;

		case SourceManagerCommand::BuildClean:
			controller.performCommand(UICommand::Clean);
			break;
		}
		return true;
	}

	bool SourceProjectManager::loadHotkeys()
	{
		// try to read the hotkeys from editor {}
		try
		{
			std::string temp;
			const auto& root = settings.root()["editor"];

			if(root.lookupValue("hkey_save", temp))
				userHotKeys[SourceManagerCommand::FileSave] = temp;

			if(root.lookupValue("hkey_new", temp))
				userHotKeys[SourceManagerCommand::FileNew] = temp;

			if(root.lookupValue("hkey_open", temp))
				userHotKeys[SourceManagerCommand::FileOpen] = temp;

			if (root.lookupValue("hkey_compile", temp))
				userHotKeys[SourceManagerCommand::BuildCompile] = temp;

			if (root.lookupValue("hkey_run", temp))
				userHotKeys[SourceManagerCommand::BuildCompileAndActivate] = temp;

			if (root.lookupValue("hkey_activate", temp))
				userHotKeys[SourceManagerCommand::BuildActivate] = temp;

			if (root.lookupValue("hkey_deactivate", temp))
				userHotKeys[SourceManagerCommand::BuildDeactivate] = temp;

			if (root.lookupValue("hkey_clean", temp))
				userHotKeys[SourceManagerCommand::BuildClean] = temp;

			if (root.lookupValue("hkey_externaledit", temp))
				userHotKeys[SourceManagerCommand::EditExternally] = temp;
		}
		catch (const std::exception & e)
		{
			controller.getConsole().printLine(CConsole::Error, "[Editor] : Error reading editor hotkeys from config... %s", e.what());
			return false;
		}
		
		appCM.registerAllCommandsForTarget(this);
		appCM.setFirstCommandTarget(this);
		
		return true;
	}
}
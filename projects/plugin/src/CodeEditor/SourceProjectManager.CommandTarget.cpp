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

	void SourceProjectManager::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo & result)
	{
		auto & cDesc = CommandTable[Menus::File][commandID];
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
		for (int c = SourceManagerCommand::FileNew; c < SourceManagerCommand::End; ++c)
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
		case SourceManagerCommand::FileSave:
			saveCurrentFile();
			break;
		case SourceManagerCommand::FileSaveAs:
			saveAs();
			break;
		}
		return true;
	}




	bool SourceProjectManager::loadHotkeys()
	{
		/*
		 try to read the hotkeys from editor {}
		 */
		try
		{
			std::string temp;
			const auto& root = settings.root();
			if(root["editor"].lookupValue("hkey_save", temp))
				userHotKeys[SourceManagerCommand::FileSave] = temp;
			if(root["editor"].lookupValue("hkey_new", temp))
				userHotKeys[SourceManagerCommand::FileNew] = temp;
			if(root["editor"].lookupValue("hkey_open", temp))
				userHotKeys[SourceManagerCommand::FileOpen] = temp;
		}
		catch (const std::exception & e)
		{
			controller.console().printLine(CConsole::Error, "[Editor] : Error reading editor hotkeys from config... %s", e.what());
			return false;
		}
		
		appCM.registerAllCommandsForTarget(this);
		appCM.setFirstCommandTarget(this);
		
		return true;
	}
}
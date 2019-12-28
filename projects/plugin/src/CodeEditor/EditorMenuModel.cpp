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
 
	 file:EditorMenuModel.cpp
	 
		Implementation of the menu handling for text editors
 
 *************************************************************************************/

#include <cpl/Common.h>
#include "EditorMenuModel.h"
#include "SourceManager.h"
#include "SourceProjectManager.h"
#include "RecentFilesManager.h"

namespace ape
{
    namespace fs = cpl::fs;

	#ifdef __MAC__
		#define CTRLCOMMANDKEY juce::ModifierKeys::Flags::commandModifier
	#else
		#define CTRLCOMMANDKEY juce::ModifierKeys::Flags::ctrlModifier
	#endif

    constexpr int kRecentOffset = 1 << 16;

	juce::PopupMenu EditorMenuModel::getMenuForIndex(int topLevelMenuIndex, const juce::String & menuName)
	{
		juce::PopupMenu ret;
        auto* cm = &manager.getCommandManager();

		switch (topLevelMenuIndex)
		{
			case Menus::File:
			{
                for (int i = SourceManagerCommand::FileStart; i < SourceManagerCommand::FileEnd; ++i)
                {
                    if (i == SourceManagerCommand::FileOpenRecent)
                    {
                        juce::PopupMenu recents;
                        RecentFilesManager::get().createPopupMenuItems(recents, kRecentOffset, false, true, nullptr);
                        ret.addSubMenu("Open recent...", recents);
                    }
                    else
                    {
                        ret.addCommandItem(cm, i);
                    }
                }

				break;
			}

			case Menus::Edit:
			{
				for (int i = SourceManagerCommand::EditStart; i < SourceManagerCommand::EditEnd; ++i)
					ret.addCommandItem(cm, i);
				break;
			}
			case Menus::Build:
			{
				for (int i = SourceManagerCommand::BuildStart; i < SourceManagerCommand::BuildEnd; ++i)
					ret.addCommandItem(cm, i);
				break;
			}
		}
		return ret;
	}

	void EditorMenuModel::menuItemSelected(int menuItemID, int topLevelMenuIndex)
	{ 
        // No easy integration of dynamic menus && application command targets.
        // Handle the ugly way.
        if (topLevelMenuIndex == Menus::File && menuItemID >= kRecentOffset)
        {
            int recentID = menuItemID - kRecentOffset;
            manager.openFile(RecentFilesManager::get().getFile(recentID).getFullPathName().toStdString());
        }
	}


    EditorMenuModel::EditorMenuModel(SourceProjectManager & manager)
        : manager(manager)
    {
    }

    juce::StringArray EditorMenuModel::getMenuBarNames()
	{
		juce::StringArray ret;
		ret.add("File");
		ret.add("Edit");
		ret.add("Build");

		return ret;
	}

}

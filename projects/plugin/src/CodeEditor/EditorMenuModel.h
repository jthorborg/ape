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

	file:EditorMenuModel.h
		
		Class managing an editor menu

*************************************************************************************/

#ifndef APE_EDITORMENUMODEL_H
	#define APE_EDITORMENUMODEL_H

	#include "../Common.h"

    namespace ape
	{
        class SourceProjectManager;

        class EditorMenuModel : public juce::MenuBarModel
        {
        public:
            EditorMenuModel(SourceProjectManager& manager);

        protected:

            juce::StringArray getMenuBarNames() override;
            juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
            void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

            SourceProjectManager& manager;

        };
	}
#endif
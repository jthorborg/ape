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

	file:CodeEditorWindow.h
		
		The JUCE code editor editorWindow

*************************************************************************************/

#ifndef APE_CODEEDITORWINDOW_H
	#define APE_CODEEDITORWINDOW_H

	#include "../Common.h"
	#include <string>
	#include "../Settings.h"
	#include "SourceManager.h"
	#include "../UI/DockWindow.h"
	
	namespace ape
	{
		extern const MenuEntry CommandTable[];

		class CodeEditorWindow 
			: public DockWindow
			, private juce::MenuBarModel
		{
		public:

			CodeEditorWindow(const Settings& settings);
			virtual ~CodeEditorWindow();
			void setAppCM(juce::ApplicationCommandManager* acm);

		private:

			juce::StringArray getMenuBarNames() override;
			juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
			void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

			// instance data
			juce::ApplicationCommandManager* appCM;
		};

	}
#endif
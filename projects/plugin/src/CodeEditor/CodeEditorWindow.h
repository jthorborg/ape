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

#ifndef APE_JUCEEDITORWINDOW_H
	#define APE_JUCEEDITORWINDOW_H

	#include "../Common.h"
	#include <cpl/CExclusiveFile.h>
	#include <string>
	#include "CLangCodeTokeniser.h"
	#include <set>
	#include <cpl/state/Serialization.h>
	#include "../Settings.h"
	#include "SourceManager.h"
	#include "cpl/gui/Tools.h"

	namespace ape
	{
		extern const MenuEntry CommandTable[][6];

		class InternalCodeEditorComponent;

		class CodeEditorWindow 
			: public juce::DocumentWindow
			, private juce::MenuBarModel
			, public cpl::SafeSerializableObject
			, public cpl::DestructionNotifier
		{
		public:

			class BreakpointListener
			{
			public:
				virtual void onBreakpointsChanged(const std::set<int>& breakpoints) = 0;
				virtual ~BreakpointListener() {}
			};

			CodeEditorWindow(const Settings& settings, std::shared_ptr<juce::CodeDocument> cd);
			virtual ~CodeEditorWindow();
			void closeButtonPressed() override;
			void setAppCM(juce::ApplicationCommandManager* acm);

			void addBreakpointListener(BreakpointListener* listener);
			void removeBreakpointListener(BreakpointListener* listener);

			const std::set<int>& getBreakpoints();
			void setBreakpoints(std::set<int> breakpoints);

			void serialize(cpl::CSerializer::Archiver & ar, cpl::Version version) override;
			void deserialize(cpl::CSerializer::Builder & ar, cpl::Version version) override;

		private:

			juce::StringArray getMenuBarNames() override;
			juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
			void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

			// instance data
			juce::ApplicationCommandManager* appCM;
			std::unique_ptr<InternalCodeEditorComponent> codeEditor;
		};

	}
#endif
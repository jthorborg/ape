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

	file:JuceEditorWindow.h
		
		The JUCE code editor editorWindow

*************************************************************************************/

#ifndef APE_JUCEEDITORWINDOW_H
	#define APE_JUCEEDITORWINDOW_H

	#include "../Common.h"
	#include <cpl/CExclusiveFile.h>
	#include <string>
	#include "CodeTokeniser.h"

	namespace ape
	{
		enum Command
		{
			InvalidCommand = -1,
			Start = 1,
			FileNew = Start,
			FileOpen,
			FileSave,
			FileSaveAs,
			FileExit,
			/* -- following are natively supported
			EditCut,
			EditCopy,
			EditUndo,
			EditRedo,
			EditPaste,
			EditDelete,
			EditSelectAll
			*/

			End
		};

		/// <summary>
		/// Enumerator to index arrays after name
		/// </summary>
		enum Menus
		{
			File,
			Edit
		};

		/// <summary>
		/// Describes an entry in the menu, with optional shortcut
		/// </summary>
		struct MenuEntry
		{
			MenuEntry(const std::string & name, int key = 0, juce::ModifierKeys modifier = juce::ModifierKeys(), Command c = InvalidCommand)
				: name(name)
				, key(key)
				, modifier(modifier)
				, command(c)
			{

			}

			bool hasShortCut() { return key || modifier.testFlags(juce::ModifierKeys::noModifiers); }

			std::string name;
			int key;
			juce::ModifierKeys modifier;
			Command command;

		};

		extern const MenuEntry CommandTable[][6];

		class CodeEditorComponent : public juce::CodeEditorComponent
		{
			using juce::CodeEditorComponent::CodeEditorComponent;
		};

		class LineTraceComponent : public juce::Component
		{
			void paint(juce::Graphics& g) override
			{
				g.fillAll(juce::Colours::green);
			}
		};

		class InternalCodeEditorComponent : public juce::Component
		{
		public:

			InternalCodeEditorComponent(juce::CodeDocument& doc)
				: cec(doc, &tokeniser)
			{
				addChildComponent(cec);
				addChildComponent(tracer);
				cec.setLineNumbersShown(true);

				cec.setVisible(true);
				tracer.setVisible(true);
			}

			void resized() override
			{
				auto bounds = getBounds().withPosition({ 0, 5 });

				tracer.setBounds(bounds.withRight(20));
				cec.setBounds(bounds.withLeft(20));
			}

		private:
			CodeTokeniser tokeniser;
			CodeEditorComponent cec;
			LineTraceComponent tracer;

		};



		class JuceEditorWindow : public juce::DocumentWindow, public juce::MenuBarModel
		{
		public:

			JuceEditorWindow(juce::CodeDocument& cd);
			virtual ~JuceEditorWindow();
			void closeButtonPressed() override;
			void setAppCM(juce::ApplicationCommandManager* acm);

		private:

			juce::StringArray getMenuBarNames() override;
			juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
			void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

			// instance data
			juce::ApplicationCommandManager* appCM;
			InternalCodeEditorComponent codeEditor;
		};

	}; // class ape
#endif
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

	file:CConsole.h
		
		An implementation of CCodeEditor which is based on juce classes DocumentWindow
		and CodeEditorComponent.

*************************************************************************************/

#ifndef APE_JUCEEDITOR_H
	#define APE_JUCEEDITOR_H

 #include <cpl/MacroConstants.h>
	#include "Common.h"
	#include "CCodeEditor.h"
	#include <cpl/CExclusiveFile.h>
	#include "ProjectEx.h"
	#include <string>
	#include <map>

	namespace ape
	{
		/*
			The various commands we support in the menu
		*/
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

		/*
			Enumerator to index arrays after name
		*/
		enum Menus
		{
			File,
			Edit
		};

		/*
			Describes an entry in the menu, with optional shortcut
		*/
		struct MenuEntry
		{
			MenuEntry(const std::string & name,
					int key = 0,
					juce::ModifierKeys modifier = juce::ModifierKeys(),
					Command c = InvalidCommand)
				: name(name), key(key), modifier(modifier), command(c)
			{

			}
			std::string name;
			int key;
			bool hasShortCut() { return key || modifier.testFlags(juce::ModifierKeys::noModifiers); }
			juce::ModifierKeys modifier;
			Command command;

		};
		/*
			this is mostly copypaste from CPlusPlusCodeTokenizer, HOWEVER
			it doesn't declare any methods virtual so we cant override anything.
			therefore, we reimplement it.
		*/

		class CTokeniser : public juce::CodeTokeniser
		{


			//==============================================================================
			int readNextToken(juce::CodeDocument::Iterator&) override;
			juce::CodeEditorComponent::ColourScheme getDefaultColourScheme() override;

			/** This is a handy method for checking whether a string is a c++ reserved keyword. */
			static bool isReservedKeyword(const juce::String& token) noexcept;
			
			/** The token values returned by this tokeniser. */
			enum TokenType
			{
				tokenType_error = 0,
				tokenType_comment,
				tokenType_keyword,
				tokenType_operator,
				tokenType_identifier,
				tokenType_integer,
				tokenType_float,
				tokenType_string,
				tokenType_bracket,
				tokenType_punctuation,
				tokenType_preprocessor
			};
		}; 

		// forward declaration
		struct AutoSaveInfo;
		/*
			This describes the complete window instance
		*/
		class CWindow : public juce::DocumentWindow, public juce::MenuBarModel
		{
			/*
				menuBarModel overloads
			*/
			juce::StringArray getMenuBarNames() override;
			juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String & menuName) override;
			void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;
			// instance data
			juce::ApplicationCommandManager * appCM;
			juce::CodeEditorComponent * cec;

			CTokeniser tokeniser;
		public:

			CWindow(juce::CodeDocument & cd);
			virtual ~CWindow();
			// override resize to size codeeditorcomponent
			void resized() override;
			void closeButtonPressed() override;
			void setAppCM(juce::ApplicationCommandManager * acm);
			juce::CodeEditorComponent * getCodeEditor() { return cec; }
		};

		/*
			This is the interface for ape that holds the editor window.
		*/
		class CJuceEditor : public CCodeEditor, public juce::ApplicationCommandTarget
		{
			/*
				ApplicationCommandTarget overloads	
			*/
			ApplicationCommandTarget * getNextCommandTarget() override { return nullptr; }
			void getAllCommands(juce::Array<juce::CommandID> & commands) override;
			void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo & result) override;
			bool perform(const InvocationInfo & info) override;

			/*
				instance variables	
			*/
			juce::ApplicationCommandManager appCM;
			CWindow * window;
			std::string fullPath, appName;
			juce::CodeDocument doc;
			cpl::CExclusiveFile autoSaveFile;
			bool isInitialized, isSingleFile, isActualFile, autoSaveChecked, wasRestored;
			std::map<int, std::string> userHotKeys;
		public:
			CJuceEditor(Engine * e);
			virtual ~CJuceEditor();
			/*
				CCodeEditor overrides
			*/
			void setErrorLine(int) override;
			bool getDocumentText(std::string &) override;
			void quit() override {};
			bool openEditor(bool initialVisibilty = true) override;
			bool closeEditor() override;
			bool exists() override { return true; }
			bool openFile(const std::string & fileName) override;
			std::unique_ptr<ProjectEx> getProject() override;
			std::string getDocumentName() override;
			std::string getDocumentPath() override;
			void autoSave() override;
			bool checkAutoSave() override;
			/*
				Utilty
			*/
		private:
			bool loadHotkeys();
			bool initEditor();
			void setTitle();
			void newDocument();
			bool isDirty();
			std::string getProjectName();
			std::string getDirectory();
			std::string getExtension();
			int saveIfUnsure();
			void saveAs();
			void doSaveFile(const std::string &);
			void saveCurrentFile();
			void openAFile();
			bool restoreAutoSave(AutoSaveInfo * info, juce::File &);
			void setContents(const juce::String &);
			void * getParentWindow();
			
		};
	}; // class ape
#endif
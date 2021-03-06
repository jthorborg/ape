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

	file:SourceManager.h
	
		Interface for the Code Editor, any code editor should inherit from this
		and at least provide the methods that are purely virtual (obviously).
		Since the class doesn't carry any parent, it cannot report error per se -
		therefore the class provides exists() method so the validity of the instance
		can be proven (return true if proper implementation exists).
 
		CDefaultCodeEditor can be used as a 'valid' implementation, that returns false
		on exists()

*************************************************************************************/

#ifndef APE_SOURCEMANAGER_H
	#define APE_SOURCEMANAGER_H

	#include <string>
	#include <memory>
	#include <cpl/Common.h>
	#include <cpl/state/Serialization.h>
	#include <cpl/Core.h>
	#include "../UI/DockWindow.h"
	#include "SourceFile.h"

	namespace ape
	{
		namespace fs = cpl::fs;

		enum SourceManagerCommand
		{
			InvalidCommand = -1,
			Start = 1,

			FileStart = Start,
			FileNew = Start,
			FileNewFromTemplate,
			FileOpen,
            FileOpenRecent,
			FileSave,
			FileSaveAs,
			FileOpenScriptsHome,
			FileEnd,

			EditStart = FileEnd,
			EditExternally = EditStart,
			EditEnd,

			BuildStart = EditEnd,
			BuildCompile = BuildStart,
			BuildCompileAndActivate,
			BuildActivate,
			BuildDeactivate,
			BuildClean,
			BuildEnd,
			
			End = BuildEnd
		};

		/// <summary>
		/// Enumerator to index arrays after name
		/// </summary>
		enum Menus
		{
			File,
			Edit,
			Build
		};

		/// <summary>
		/// Describes an entry in the menu, with optional shortcut
		/// </summary>
		struct MenuEntry
		{
			MenuEntry(const std::string & name, int key = 0, juce::ModifierKeys modifier = juce::ModifierKeys(), SourceManagerCommand c = InvalidCommand)
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
			SourceManagerCommand command;

		};

		struct ProjectEx;
		class UIController;
		class Settings;

		class SourceManager : public cpl::CSerializer::Serializable
		{
		public:

			SourceManager(UIController& c, const Settings& s, int instanceID) 
				: controller(c)
				, settings(s)
				, instanceID(instanceID) 
			{

			}

			virtual std::unique_ptr<ProjectEx> createProject() = 0;
			virtual std::unique_ptr<juce::Component> createCodeEditorComponent() = 0;

			virtual ~SourceManager() {};
			virtual void setErrorLine(int nLine) = 0;
			virtual bool getDocumentText(std::string & buffer) = 0;
			virtual const SourceFile& getSourceFile() = 0;
			virtual bool isDirty() = 0;
			virtual bool openFile(const fs::path& fileName) { return false; }
			virtual void autoSave() {};
			// true: a project was restored, false: nothing happened
			virtual bool checkAutoSave() { return false; }

		protected:
			UIController& controller;
			const Settings& settings;
			int instanceID;

		};

		std::unique_ptr<SourceManager> MakeSourceManager(UIController& c, const Settings& s, int instanceID);


	};
#endif
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

	namespace ape
	{
		namespace fs = cpl::fs;

		enum SourceManagerCommand
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

			virtual ~SourceManager() {};
			virtual void setErrorLine(int nLine) = 0;
			virtual bool getDocumentText(std::string & buffer) = 0;
			virtual std::unique_ptr<ProjectEx> getProject() = 0;
			virtual bool setEditorVisibility(bool visible) = 0;
			virtual std::string getDocumentName() { return ""; }
			virtual fs::path getDocumentPath() = 0;
			virtual bool isDirty() = 0;
			virtual bool openFile(const fs::path& fileName) { return false; }
			virtual void autoSave() {};
			// true: a project was restored, false: nothing happened
			virtual bool checkAutoSave() { return false; }
			virtual bool exists() = 0;
			virtual void* getParentWindow() = 0;

		protected:
			UIController& controller;
			const Settings& settings;
			int instanceID;

		};

		std::unique_ptr<SourceManager> MakeSourceManager(UIController& c, const Settings& s, int instanceID);


	};
#endif
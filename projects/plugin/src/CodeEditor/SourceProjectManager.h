/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2018 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:SourceProjectManager.h
		
		Implementation of the source project manager

*************************************************************************************/

#ifndef APE_SOURCEPROJECTMANAGER_H
	#define APE_SOURCEPROJECTMANAGER_H

	#include <cpl/MacroConstants.h>
	#include "../Common.h"
	#include "SourceManager.h"
	#include <cpl/CExclusiveFile.h>
	#include "../ProjectEx.h"
	#include <string>
	#include <map>
	#include "CodeEditorWindow.h"
	#include <cpl/state/DecoupledStateObject.h>

	namespace ape
	{
		
		struct AutoSaveInfo;
		class CodeEditorWindow;

		class SourceProjectManager 
			: public SourceManager
			, public juce::ApplicationCommandTarget
			, private CodeEditorWindow::BreakpointListener
			, public cpl::CSerializer::Serializable
		{

		public:
			SourceProjectManager(UIController& ui, const Settings& s, int instanceID);
			virtual ~SourceProjectManager();

			// SourceManager overrides
			void setErrorLine(int) override;
			bool getDocumentText(std::string &) override;
			bool setEditorVisibility(bool visible) override;
			bool exists() override { return true; }
			bool openFile(const std::string & fileName) override;
			std::unique_ptr<ProjectEx> getProject() override;
			std::string getDocumentName() override;
			std::string getDocumentPath() override;
			void autoSave() override;
			bool checkAutoSave() override;

			void serialize(cpl::CSerializer::Archiver & ar, cpl::Version version) override;
			void deserialize(cpl::CSerializer::Builder & builder, cpl::Version version) override;

		protected:

			void onBreakpointsChanged(const std::set<int>& breakpoints) override;

		private:

			std::unique_ptr<CodeEditorWindow> createWindow();
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

			//ApplicationCommandTarget overloads
			ApplicationCommandTarget * getNextCommandTarget() override { return nullptr; }
			void getAllCommands(juce::Array<juce::CommandID> & commands) override;
			void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo & result) override;
			bool perform(const InvocationInfo & info) override;

			// instance stuff
			juce::ApplicationCommandManager appCM;
			cpl::UniqueHandle<CodeEditorWindow> editorWindow;
			cpl::SerializableStateObject<CodeEditorWindow> editorWindowState;
			std::string fullPath, appName;
			juce::CodeDocument doc;
			cpl::CExclusiveFile autoSaveFile;
			bool isSingleFile, isActualFile, autoSaveChecked, wasRestored;
			std::map<int, std::string> userHotKeys;
			std::set<int> breakpoints;
		};
	}; // class ape
#endif
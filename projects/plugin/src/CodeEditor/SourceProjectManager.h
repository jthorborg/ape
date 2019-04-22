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

	#include <optional>
	#include <string>
	#include <map>
	#include <memory>

	#include <cpl/state/DecoupledStateObject.h>
	#include <cpl/MacroConstants.h>
	#include <cpl/CExclusiveFile.h>

	#include "SourceManager.h"
	#include "../ProjectEx.h"
	#include "CodeEditorWindow.h"
	#include "CodeEditorComponent.h"
	#include "BreakpointComponent.h"
	#include "CodeDocumentListener.h"

	namespace ape
	{
		namespace fs = cpl::fs;

		struct AutoSaveInfo;
		class CodeEditorWindow;

		class SourceProjectManager final
			: public SourceManager
			, public juce::ApplicationCommandTarget
			, private BreakpointComponent::Listener
			, private juce::CodeDocument::Listener
			, private CodeDocumentSource
		{

		public:

			SourceProjectManager(UIController& ui, const Settings& s, int instanceID);
			virtual ~SourceProjectManager();

			std::unique_ptr<juce::Component> createCodeEditorComponent() override;
			std::unique_ptr<DockWindow> createSuitableCodeEditorWindow() override;
			std::unique_ptr<ProjectEx> createProject() override;

			// SourceManager overrides
			void setErrorLine(int) override;
			bool getDocumentText(std::string &) override;
			bool openFile(const fs::path& fileName) override;
			bool isDirty() override;
			SourceFile getSourceFile() override;

			void serialize(cpl::CSerializer::Archiver & ar, cpl::Version version) override;
			void deserialize(cpl::CSerializer::Builder & builder, cpl::Version version) override;

			void addListener(CodeDocumentListener& listener) override;
			void removeListener(CodeDocumentListener& listener) override;

		protected:

			void onBreakpointsChanged(const std::set<int>& breakpoints) override;
			void* getParentWindow();

			virtual void codeDocumentTextInserted(const juce::String& newText, int insertIndex) override;
			virtual void codeDocumentTextDeleted(int startIndex, int endIndex) override;

		private:

			void validateInvariants();
			void checkDirtynessState();

			std::unique_ptr<CodeEditorComponent> createWindow();
			bool loadHotkeys();
			void setTitle();
			void newDocument();
			std::string getCurrentLanguageID();
			cpl::Misc::MsgButton saveIfUnsure();
			void saveAs();
			void doSaveFile(const fs::path&);
			void saveCurrentFile();
			void openAFile();
			void setContents(const juce::String &);
			void cacheValidFileTypes(const Settings& s);
			bool openTemplate();
			void openHomeDirectory();

			//ApplicationCommandTarget overloads
			ApplicationCommandTarget * getNextCommandTarget() override { return nullptr; }
			void getAllCommands(juce::Array<juce::CommandID> & commands) override;
			void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo & result) override;
			bool perform(const InvocationInfo & info) override;

			// instance stuff
			juce::ApplicationCommandManager appCM;
			cpl::SerializableStateObject<CodeEditorComponent> textEditorDSO;

			std::shared_ptr<juce::CodeDocument> doc;
			SourceFile sourceFile;
			bool enableScopePoints, shouldCheckContentsAgainstDisk;
			std::optional<bool> lastDirtyState;

			std::map<int, std::string> userHotKeys;
			std::set<int> breakpoints;
			std::set<CodeDocumentListener*> listeners;

			std::vector<std::string> validFileTypes;
			std::string validFileTypePattern;
			fs::path templateFile;
			fs::path homeDirectory;
			std::string defaultLanguageExtension;
		};
	}
#endif
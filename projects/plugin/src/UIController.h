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

	file:UIController.h
	
		Implements the graphic user interface, and all logic associated with the program
		flow and interaction.

*************************************************************************************/

#ifndef APE_UICONTROLLER_H
	#define APE_UICONTROLLER_H

	#include "Common.h"
	#include "GraphicComponents.h"
	#include <vector>
	#include "SignalizerWindow.h"
	#include <cpl/Misc.h>
	#include <cpl/CMutex.h>
	#include <future>
	#include <memory>
	#include "UI/UICommands.h"
	#include "UI/LabelQueue.h"
	#include <ape/APE.h>
	#include <cpl/state/DecoupledStateObject.h>
	#include "Engine/EngineStructures.h"

	namespace ape 
	{

		class Engine;
		class CConsole;
		struct CompilerBinding;
		class SourceManager;
		class CSerializer;
		class PluginState;
		struct ProjectEx;
		class MainEditor;
		class AutosaveManager;
		class UICommandState;

		class UIController
			: public cpl::CMutex::Lockable
			, public cpl::DestructionNotifier
			, public cpl::CSerializer::Serializable
		{

		  public:
			
			friend class SourceManager;
			friend class MainEditor;
			friend class Engine;

			UIController(ape::Engine& effect);
			virtual ~UIController();

			void pulseUI();
			MainEditor * create();
			ape::CConsole& getConsole() noexcept { return *console.get();	};
			SourceManager& getSourceManager() noexcept { return *sourceManager; }
			UICommandState& getUICommandState() noexcept { return *commandStates; }
			LabelQueue& getLabelQueue() noexcept { return labelQueue; }
			const std::string& getProjectName() const noexcept;
			void * getSystemWindow();

			void setEditorError(int nLine);
			void onBreakpointsChanged(const std::set<int>& newTraces);
			void recompile(bool hotReload = true);

			// TODO: Make private.
			std::future<std::unique_ptr<PluginState>> createPlugin(bool enableHotReload = true);

			bool performCommand(UICommand command);

			void serialize(cpl::CSerializer::Archiver & ar, cpl::Version version) override;
			void deserialize(cpl::CSerializer::Builder & ar, cpl::Version version) override;
			void externalDiagnostic(Diagnostic level, const cpl::string_ref text);

			void setPlugin(std::shared_ptr<PluginState> newPlugin);

		private:

			void pluginExchanged(std::shared_ptr<PluginState> plugin, PluginExchangeReason reason);

			bool activatePlugin(bool sync = true);

			void editorOpened(MainEditor * newEditor);
			void editorClosed();

			std::future<std::unique_ptr<PluginState>> createPlugin(std::unique_ptr<ProjectEx> project, bool enableHotReload = true);
			void setProjectName(std::string name);
			void setupProject(ProjectEx& project);

			std::unique_ptr<AutosaveManager> autosaveManager;
			std::unique_ptr<CConsole> console;
			std::unique_ptr<SourceManager> sourceManager;
			std::unique_ptr<UICommandState> commandStates;

			ape::Engine& engine;
			std::shared_ptr<PluginState> currentPlugin;
			std::unique_ptr<cpl::SerializableStateObject<MainEditor>> editorSSO;
			std::future<std::unique_ptr<PluginState>> compilerState;
			std::future<bool> activationState;

			LabelQueue labelQueue;			
			std::string projectName;	
		};
	};
#endif

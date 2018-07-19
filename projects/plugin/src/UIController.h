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
	#include "CControlManager.h"
	#include "ButtonDefinitions.h"
	#include "SignalizerWindow.h"
	#include <cpl/Misc.h>
	#include <cpl/CMutex.h>
	#include <future>
	#include <memory>

	namespace ape 
	{

		class Engine;
		class CConsole;
		struct CCompiler;
		class SourceManager;
		class CSerializer;
		class CQueueLabel;
		class PluginState;
		struct ProjectEx;
		class Editor;
		class AutosaveManager;

		class UIController
			: public cpl::CMutex::Lockable
			, public cpl::DestructionNotifier
			, public cpl::CSerializer::Serializable
		{
			enum class Commands
			{
				Invalid,
				Recompile,
				Activate,
				Deactivate,
				OpenSourceEditor,
				CloseSourceEditor
			};

			static constexpr std::size_t magic_value = 0xDEADBEEF;
			const std::size_t magic = magic_value;

		  public:
			
			friend class SourceManager;
			friend class Editor;

			UIController(ape::Engine& effect);
			virtual ~UIController();

			void setParameter(int index, float value);
			void editorOpened(Editor * newEditor);
			void editorClosed();
			void render();
			Editor * create();
			ape::CConsole& console() noexcept { return *consolePtr.get();	};
			SourceManager& getSourceManager() noexcept { return *sourceManager; }
			void setStatusText(const std::string &, CColour color = juce::Colours::lightgoldenrodyellow);
			void setStatusText(const std::string &, CColour color, int timeout);
			void setStatusText();
			std::string getStatusText();
			void setEditorError(int nLine);
			void updateInfoLabel();
			void * getSystemWindow();
			void onBreakpointsChanged(const std::set<int>& newTraces);
			void recompile();
			std::future<std::unique_ptr<PluginState>> createPlugin(bool enableHotReload = true);
			const std::string& getProjectName() const noexcept { return projectName; }

			bool performCommand(Commands command);

			void serialize(cpl::CSerializer::Archiver & ar, cpl::Version version) override;
			void deserialize(cpl::CSerializer::Builder & ar, cpl::Version version) override;

			static void errorPrint(void * data, const char * text);


		private:

			void onErrorMessage(const cpl::string_ref text);

			std::future<std::unique_ptr<PluginState>> createPlugin(std::unique_ptr<ProjectEx> project, bool enableHotReload = true);
			void setProjectName(const std::string & name) { projectName = name; }

			std::unique_ptr<AutosaveManager> autosaveManager;
			std::unique_ptr<CConsole> consolePtr;
			std::unique_ptr<SourceManager> sourceManager;

			ape::Engine& engine;
			Editor * editor;
			std::future<std::unique_ptr<PluginState>> compilerState;
			std::string projectName, statusLabel;
			CColour statusColour;
			bool bFirstDraw;
			// these are refererences to the engine
			volatile bool & bUseBuffers;
			volatile bool & bUseFPUE;
			volatile bool & bIsActive;
			int uiRefreshInterval;
			unsigned int incGraphicCounter;
			
			struct
			{
				unsigned lastSample;
				float pole;
				float averageClocks;
			} clockData;

		};
	};
#endif
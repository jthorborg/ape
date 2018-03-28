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

	file:GUI.h
	
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
		class CCodeEditor;
		class CSerializer;
		class CQueueLabel;
		class PluginState;
		struct ProjectEx;

		class UIController
			: public CCtrlListener, public cpl::CMutex::Lockable
		{
			static constexpr std::size_t magic_value = 0xDEADBEEF;
			const std::size_t magic = magic_value;

		  public:
			
			class Editor;
			
			friend class ape::Engine;
			friend class CCodeEditor;
			friend class CSerializer;
			friend class Editor;

			/*
				We have to use composition instead of inheritance 
				due to the way the editor gets destructed on window
				closure, ie. no way to save state.
				Therefore, this proxy object delegates everything
				to the main editor.
			*/
			class Editor
				: public juce::AudioProcessorEditor, public juce::Timer
			{
				friend class UIController;
				std::vector<juce::Component *> garbageCollection;
				std::map<int, CBaseControl *> controls;
				CTextControl * infoLabel;
				CQueueLabel * statusLabel;
				juce::DrawableImage background;
				juce::Image bImage;
				juce::Image testImage;
				int repaintCallBackCounter;
				
				juce::OpenGLContext oglc;
				SignalizerWindow scope;
			public:
				UIController & parent;
				void initialize(bool useOpenGL = false);
				void paint(juce::Graphics & g);
				void timerCallback();
				Editor(UIController & parent);
				virtual ~Editor();
			};

			UIController(ape::Engine& effect);
			virtual ~UIController();

			void setParameter(int index, float value);
			void about();
			void editorOpened(Editor * newEditor);
			void editorClosed();
			void render();
			Editor * create();
			ape::CConsole& console() noexcept { return *consolePtr.get();	};
			void setStatusText(const std::string &, CColour color = juce::Colours::lightgoldenrodyellow);
			void setStatusText(const std::string &, CColour color, int timeout);
			void setStatusText();
			std::string getStatusText();
			void setEditorError(int nLine);
			void updateInfoLabel();
			void * getSystemWindow() { return editor ? editor->getWindowHandle() : nullptr; }

			std::future<std::unique_ptr<PluginState>> createPlugin();

			static void errorPrint(void * data, const char * text);

		private:
			
			void onErrorMessage(const cpl::string_ref text);

			std::future<std::unique_ptr<PluginState>> createPlugin(std::unique_ptr<ProjectEx> project);
			void setProjectName(const std::string & name) { projectName = name; }
			virtual bool valueChanged(CBaseControl *);
			
			std::unique_ptr<CConsole> consolePtr;
			std::unique_ptr<CCodeEditor> externEditor;
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
			int autoSaveInterval;
			int uiRefreshInterval;
			unsigned int autoSaveCounter;
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
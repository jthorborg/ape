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

#ifndef _GUI_H
	#define _GUI_H

	#include "Common.h"
	#include "GraphicComponents.h"
	#include <vector>
	#include "CControlManager.h"
	#include "ButtonDefinitions.h"
	#include "Misc.h"

	namespace Scintilla {

		struct SciLexer;

	};
	namespace APE {

		class Engine;
		class CConsole;
		struct CCompiler;
		class CCodeEditor;
		class CSerializer;
		class CQueueLabel;

		class GraphicUI
		#ifdef APE_VST
			: public AEffGUIEditor, public CControlListener, public CMutex::Lockable
		#elif defined(APE_JUCE)
			: public CCtrlListener, public CMutex::Lockable
		#endif
		{
		  public:
			
			class Editor;
			
			friend class APE::Engine;
			friend struct Scintilla::SciLexer;
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
				friend class GraphicUI;
				std::vector<juce::Component *> garbageCollection;
				std::map<int, CBaseControl *> controls;
				CTextControl * infoLabel;
				CQueueLabel * statusLabel;
				juce::DrawableImage background;
				juce::Image bImage;
				juce::Image testImage;
				int repaintCallBackCounter;
				
				juce::OpenGLContext oglc;
				
			public:
				GraphicUI & parent;
				void initialize(bool useOpenGL = false);
				void paint(juce::Graphics & g);
				void timerCallback();
				Editor(GraphicUI & parent);
				virtual ~Editor() __llvm_DummyNoExcept;
			};
			/*
			this struct handles incoming events from the VSTGUI lib specific for the plugins'
			controls. it delegates the event onto the core, to give the plugin a chance
			to react and change appearance. if the plugin decides to not handle the event,
			the default handler from the control is used.
			*/
			struct CCtrlDelegateListener // wanted to be unnamed, but cannot create a constructor then.
				: public CCtrlListener
			{
				GraphicUI * parent;
				CCtrlDelegateListener(GraphicUI * p) : parent(p) {};
				virtual bool valueChanged(CBaseControl * control);
				virtual ~CCtrlDelegateListener() {};
			} ctrlNotifier;

			GraphicUI(APE::Engine *effect);
			virtual ~GraphicUI();

			void setParameter(int index, float value);
			void about();
			void editorOpened(Editor * newEditor);
			void editorClosed();
			void render();
			Editor * create();
			APE::CConsole * console;
			void setStatusText(const std::string &, CColour color = juce::Colours::lightgoldenrodyellow);
			void setStatusText(const std::string &, CColour color, int timeout);
			void setStatusText();
			std::string getStatusText();
			void setEditorError(int nLine);
			void updateInfoLabel();
			void * getSystemWindow() { return editor ? editor->getWindowHandle() : nullptr; }
			CPluginCtrlManager ctrlManager;
		  protected:
			
			static void * startCompilation(void * effect);
			void setProjectName(const std::string & name) { projectName = name; }
			virtual bool valueChanged(CBaseControl *);
			
			CCodeEditor * externEditor;
			APE::Engine * engine;
			Editor * editor;

			std::string projectName, statusLabel;
			CColour statusColour;
			volatile bool bIsCompiling;
			volatile bool bIsCompiled;
			bool bShouldReset;
			bool bFirstDraw;
			volatile bool bStatusLock;
			// these are refererences to the engine
			volatile bool & bUseBuffers;
			volatile bool & bUseFPUE;
			volatile bool & bIsActive;
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
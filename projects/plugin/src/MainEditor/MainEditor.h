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

#ifndef APE_MAINEDITOR_H
	#define APE_MAINEDITOR_H

	#include "../Common.h"
	#include "../GraphicComponents.h"
	#include <vector>
	#include <cpl/Misc.h>
	#include <cpl/CMutex.h>
	#include <cpl/gui/CViews.h>
	#include <cpl/gui/controls/Controls.h>
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
		class PluginSurface;
		class SignalizerWindow;
		class UICommandState;

		class MainEditor final
			: public juce::AudioProcessorEditor
			, private juce::Timer
			, public cpl::CTopView
			, private cpl::ValueEntityBase::Listener
		{
			friend class UIController;
		public:

			MainEditor(UIController& parent);

			void initialize(bool useOpenGL = false);
			void paint(juce::Graphics & g) override;
			void timerCallback() override;
			virtual ~MainEditor();
			void about();
			void onPluginStateChanged(PluginState& state, bool activated);
			void resized() override;

		private:
			void valueEntityChanged(cpl::ValueEntityBase::Listener * sender, cpl::ValueEntityBase * value) override;

			// Inherited via CTopView
			virtual juce::Component * getWindow() override;

			UICommandState& state;
			UIController& parent;

			cpl::CButton console, compilation, activation, editor, scope;


			std::vector<juce::Component *> garbageCollection;
			std::map<int, CBaseControl *> controls;
			std::shared_ptr<PluginSurface> pluginSurface;

			CTextControl * infoLabel;
			CQueueLabel * statusLabel;
			juce::DrawableImage background;
			juce::Image bImage;
			juce::Image testImage;
			int repaintCallBackCounter;

			juce::OpenGLContext oglc;
			std::unique_ptr<SignalizerWindow> scopeWindow;


		};

	};
#endif
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

	file:MainEditor.h
	
		Implements the graphic user interface, and all logic associated with the program
		flow and interaction.

*************************************************************************************/

#ifndef APE_MAINEDITOR_H
	#define APE_MAINEDITOR_H

	#include "../Common.h"
	#include "../GraphicComponents.h"
	#include <cpl/Misc.h>
	#include <cpl/CMutex.h>
	#include <cpl/gui/CViews.h>
	#include <cpl/gui/controls/Controls.h>
	#include <memory>
	#include <dockable-windows/Source/JDockableWindows.h>
	#include "../UI/PlayStateButton.h"
	#include "../UI/StopButton.h"
	#include "../UI/CleanButton.h"

	namespace ape 
	{

		class Engine;
		class CConsole;
		struct CompilerBinding;
		class SourceManager;
		class CSerializer;
		class LabelQueueDisplay;
		class PluginState;
		struct ProjectEx;
		class PluginSurface;
		class SignalizerWindow;
		class UICommandState;
        class UIController;

		class MainEditor final
			: public juce::AudioProcessorEditor
			, private juce::Timer
			, public cpl::CTopView
		{
			friend class UIController;
			friend class DockWindow;

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

			void serialize(cpl::CSerializer::Archiver & ar, cpl::Version version) override;
			void deserialize(cpl::CSerializer::Builder & ar, cpl::Version version) override;

			juce::Rectangle<int> getContentArea();

			// Inherited via CTopView
			virtual juce::Component * getWindow() override;

			UICommandState& state;
			UIController& parent;
			PlayStateButton compilation;
			StopButton activation;
			CleanButton clean;
			cpl::CValueComboBox precision;

			std::shared_ptr<PluginSurface> pluginSurface;

			std::unique_ptr<CTextControl> infoLabel;
			std::unique_ptr<LabelQueueDisplay> statusLabel;

			juce::OpenGLContext oglc;
			std::unique_ptr<juce::Component> 
				consoleWindow,
				scopeSettingsWindow,
				scopeWindow,
				codeWindow;

			jcredland::DockableWindowManager dockManager;
			jcredland::TabDock tabs;

			juce::ResizableCornerComponent rcc;
		};

	};
#endif

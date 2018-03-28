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

	file:Common.h
	
		Implements common constants, types & macroes used throughout the program.
		Compilier specific tunings.
		Also #includes commonly used headers.

*************************************************************************************/

#ifndef _COMMON_H
	#define _COMMON_H
 #include <cpl/MacroConstants.h>

	#ifdef APE_IPLUG
		#include "IPlug_include_in_plug_hdr.h"
		#include "IPlugSupport.h"
	#elif defined(APE_VST)
		#include "audioeffectx.h"
		#include "VSTGUISupport.h"
	#elif defined(APE_JUCE)
		#define DONT_SET_USING_JUCE_NAMESPACE 1
		#include "../JuceLibraryCode/JuceHeader.h"
	#endif

	namespace ape
	{

		enum GUI_RESOURCE
		{
			kBack = 128,
			kConsoleButton,
			kCompileButton,
			kActiveStateButton,
			kTextEdit, //bit misleading but w/e
			kEditorButton,
			kAboutButton,
			kKnob,
			kKnobIndi,
			kCheckBox,
			kMeter,
			kButton,
			TagCEdit


		};
		enum kTags
		{
			tagConsole = 129,
			tagCompile,
			tagActiveState,
			tagTextBox,
			tagEditor,
			tagAbout,
			tagUseBuffer,
			tagUseFPU,
			kTagEnd
		};

		enum {
			kTextBufSize = 24,
		};

		#ifdef APE_JUCE
			typedef juce::Colour CColour;
			typedef juce::Colours CColours;
			typedef juce::Point<int> CPoint;
			typedef juce::Rectangle<int> CRect;
			typedef juce::Component GraphicComponent;
			typedef int CCoord;
		#elif defined (APE_VST)
			typedef VSTGUI::CRect CRect;
			typedef VSTGUI::CPoint CPoint;
			typedef CColor CColour;
			typedef CFrame * GraphicComponent;
		#endif
		class CBaseControl;

		#define APE_DEPRECATED_CONTROL_SIZE 80

		enum TextSize
		{
			smallerText = 11,
			smallText = 12,
			normalText = 14,
			largeText = 17
		};
		// enum emulation


		// TODO: move to common
		struct IOConfig
		{
			std::size_t inputs = 0, outputs = 0, blockSize = 0;
			double sampleRate = 0;

			bool operator == (const IOConfig& other) const noexcept
			{
				return inputs == other.inputs && outputs == other.outputs && blockSize == other.blockSize && sampleRate == other.sampleRate;
			}

			bool operator != (const IOConfig& other) const noexcept { return !(*this == other); }
		};

	};
#endif
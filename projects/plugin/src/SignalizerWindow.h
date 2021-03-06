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

	file:CApi.h

		The API the C program can utilize, definitions for exported functions/symbols.

*************************************************************************************/

#ifndef SIGNALIZERWINDOW_H
	#define SIGNALIZERWINDOW_H

	#include "SignalizerConfiguration.h"
	#include "Oscilloscope/Oscilloscope.h"
	#include <cpl/CPresetManager.h>
	
	namespace ape
	{
		class Engine;

		struct OscilloscopeData : private Signalizer::ParameterSet::AutomatedProcessor
		{
			OscilloscopeData()
				: stream(4, true)
				, view(stream, *this)
				, content(0, false, view)
			{
				behaviour.hideWidgetsOnMouseExit = true;
				behaviour.stopProcessingOnSuspend = true;

				cpl::CCheckedSerializer state("oscilloscope");

				juce::File location;
				auto& manager = cpl::CPresetManager::instance();

				if (manager.loadPreset(manager.getPresetDirectory() + "init.oscilloscope.sgn", state, location))
				{
					// HACK: Use Signalizer::SentientViewState here properly
					content.deserializeObject(state.getBuilder()["Parameters"], state.getBuilder().getLocalVersion());
				}

				initializeColours(Signalizer::OscilloscopeContent::NumColourChannels);
			}
			
			void setTriggeringChannel(int index)
			{
				content
					.triggeringChannel
					.getParameterView()
					.updateFromProcessorTransformed(index);
			}

			void initializeColours(std::size_t count)
			{
				constexpr std::uint32_t multiplier = 0x34729;
				constexpr double scale = 0.4;
				constexpr double offset = 0.2;

				std::uint32_t colourIndex = 1;

				count = std::min(count, Signalizer::OscilloscopeContent::NumColourChannels);

				for (std::size_t i = 0; i < Signalizer::OscilloscopeContent::NumColourChannels; ++i)
				{
					auto& colour = content.getColour(i);
					auto prototype = juce::Colour::fromHSV(i / (count - 0.0f), 0.7f, 0.7f, 1.0f);

					colour.getValueIndex(cpl::ColourValue::A).setNormalizedValue(1);
					colour.getValueIndex(cpl::ColourValue::R).setNormalizedValue(prototype.getFloatRed());
					colour.getValueIndex(cpl::ColourValue::G).setNormalizedValue(prototype.getFloatGreen());
					colour.getValueIndex(cpl::ColourValue::B).setNormalizedValue(prototype.getFloatBlue());

				}
			}

		public:

			Signalizer::AudioStream& getStream() noexcept { return stream; }
			Signalizer::SharedBehaviour& getBehaviour() noexcept { return behaviour; }
			Signalizer::OscilloscopeContent& getContent() noexcept { return content; }
			const std::string& getName() const noexcept { return name; }

			std::unique_ptr<juce::Component> createEditor();
			std::unique_ptr<juce::Component> createWindow();

		private:

			virtual void automatedTransmitChangeMessage(int parameter, Signalizer::ParameterSet::FrameworkType value) override {}
			virtual void automatedBeginChangeGesture(int parameter) override {}
			virtual void automatedEndChangeGesture(int parameter) override {}

			const std::string name = "scope";
			Signalizer::SharedBehaviour behaviour;
			Signalizer::AudioStream stream;
			Signalizer::SystemView view;
			Signalizer::OscilloscopeContent content;

		};
	};

#endif

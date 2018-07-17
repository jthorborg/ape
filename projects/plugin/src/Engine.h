/*************************************************************************************

	Audio Programming Environment VST. 
		
		VST is a trademark of Steinberg Media Technologies GmbH.

    Copyright (C) 2013 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:ape.h
		
		Implements the interface for the VST.

*************************************************************************************/

#ifndef APE_ENGINE_H
	#define APE_ENGINE_H

	#include "Common.h"
	#include "CApi.h"
	#include "CMemoryGuard.h"
	#include <string>
	#include "Settings.h"
	#include <cpl/CMutex.h>
	#include <memory>
	#include <cpl/ConcurrentServices.h>
	#include <shared_mutex>
	#include "CCodeGenerator.h"
	#include "Engine/EngineStructures.h"
	// TODO: remove
	#include "SignalizerWindow.h"

	namespace ape 
	{

		class UIController;
		class PluginState;
		class CBaseControl;
		class CSerializer;
		class ParameterManager;

		class Engine 
			: public juce::AudioProcessor
			, private Settings::Listener
		{

			friend class PluginState;
			friend class UIController;
			friend class CSerializer;
			friend class ParameterManager;

		public:

			Engine();
			virtual ~Engine();

			OscilloscopeData& getOscilloscopeData() { return scopeData; }
			UIController& getController() { return *controller; }
			PluginState * getCurrentPluginState() { return pluginState.get(); }
			CCodeGenerator& getCodeGenerator() noexcept { return codeGenerator; }
			Settings& getSettings() noexcept { return settings; }
			const Settings& getSettings() const noexcept { return settings; }
			ParameterManager& getParameterManager() noexcept { return *params; }

			void exchangePlugin(std::unique_ptr<PluginState> plugin);
			void disablePlugin(bool fromEditor = true);
			bool activatePlugin();
			void useProtectedBuffers(bool bValue) { status.bUseBuffers = bValue; }
			std::int32_t uniqueInstanceID() const noexcept;
			std::int32_t instanceCounter() const noexcept;
			void changeInitialDelay(long samples) noexcept;
			void handleTraceCallback(const char** names, std::size_t nameCount, const float * values, std::size_t valueCount);

		protected:

			void prepareToPlay(double sampleRate, int samplesPerBlock) override;
			void releaseResources() override;
			void processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;
			juce::AudioProcessorEditor* createEditor() override;
			const juce::String getName() const override;
			bool hasEditor() const override;
			//==============================================================================
			int getNumParameters() override;
			float getParameter(int index) override;
			void setParameter(int index, float newValue) override;
			const juce::String getParameterName(int index) override;
			const juce::String getParameterText(int index) override;
			//==============================================================================
			const juce::String getInputChannelName(int channelIndex) const override;
			const juce::String getOutputChannelName(int channelIndex) const override;
			bool isInputChannelStereoPair(int index) const override;
			bool isOutputChannelStereoPair(int index) const override;
			//==============================================================================
			bool acceptsMidi() const override;
			bool producesMidi() const override;
			bool silenceInProducesSilenceOut() const override;
			double getTailLengthSeconds() const override;
			//==============================================================================
			int getNumPrograms() override;
			int getCurrentProgram() override;
			void setCurrentProgram(int index) override;
			const juce::String getProgramName(int index) override;
			void changeProgramName(int index, const juce::String& newName) override;
			//==============================================================================
			void getStateInformation(juce::MemoryBlock& destData) override;
			void setStateInformation(const void* data, int sizeInBytes) override;
			//==============================================================================

		private:

			void consumeTracerChanges(TracerState& state);

			void loadSettings();
			std::string engineType() const noexcept;
			
			void onSettingsChanged(const Settings& s, const libconfig::Setting& setting) override;

			struct {
				volatile bool bActivated;
				volatile bool bUseBuffers;
				volatile bool bUseFPUE;

			} status;
			struct {
				volatile bool bDelayChanged;
				long initialDelay;
				long newDelay;
			} delay;
			unsigned numBuffers;
			
			std::int32_t instanceID;
			CCodeGenerator codeGenerator;
			cpl::ConcurrentObjectSwapper<PluginState> rtPlugin;
			std::unique_ptr<UIController> controller;
			std::unique_ptr<PluginState> pluginState;
			std::unique_ptr<ParameterManager> params;

			TracerState tracerState;
			std::string programName;
			Settings settings;
			IOConfig ioConfig;
			bool isPlaying = false;
			std::atomic<double> clocksPerSample;
			std::shared_mutex pluginMutex;
			OscilloscopeData scopeData;
			AuxMatrix auxMatrix;
		};
	}
#endif
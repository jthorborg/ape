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
	#include <memory>
	#include <cpl/ConcurrentServices.h>
	#include "CCodeGenerator.h"
	#include "Engine/EngineStructures.h"
	#include <vector>
	// TODO: remove
	#include "SignalizerWindow.h"
	#include <cpl/lib/LockFreeQueue.h>
	
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
			//friend class CSerializer;
			friend class ParameterManager;

		public:

			struct ProfilerData
			{
				double smoothedCPUUsage;
				double clocksPerSample;
				double smoothedClocksPerSample;
				double sampleRate;
			};

			Engine();
			virtual ~Engine();

			OscilloscopeData& getOscilloscopeData() { return scopeData; }
			UIController& getController() { return *controller; }
			CCodeGenerator& getCodeGenerator() noexcept { return codeGenerator; }
			Settings& getSettings() noexcept { return settings; }
			const Settings& getSettings() const noexcept { return settings; }
			ParameterManager& getParameterManager() noexcept { return *params; }
			ProfilerData getProfilingData() const noexcept;
			const IOConfig& getConfig() const noexcept { return ioConfig; }
			bool getPlayState() const noexcept { return isPlaying; }
			bool isProcessingAPlugin() const noexcept { return pluginStates.size() > 0; }

			std::int32_t uniqueInstanceID() const noexcept;
			std::int32_t instanceCounter() const noexcept;
			void changeInitialDelay(long samples) noexcept;
			void handleTraceCallback(const char** names, std::size_t nameCount, const float * values, std::size_t valueCount);
			void pulse();

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

			bool processPlugin(PluginState& plugin, TracerState& state, std::size_t numSamples, const float* const* inputs, std::size_t* numTraces);
			void processReturnQueue();
			void exchangePlugin(std::shared_ptr<PluginState> plugin);

			void onInitialTracerChanges(TracerState& state);

			void loadSettings();
			std::string engineType() const noexcept;
			
			void onSettingsChanged(const Settings& s, const libconfig::Setting& setting) override;

			struct {
				std::atomic<bool> delayChanged;
				long initialDelay;
				long newDelay;
			} delay;
			
			bool 
				isPlaying = false, 
				fadePlugins = true, 
				useFPE = false, 
				preserveParameters = true;


			std::int32_t instanceID;
			CCodeGenerator codeGenerator;
			std::unique_ptr<UIController> controller;
			std::vector<std::shared_ptr<PluginState>> pluginStates;
			std::unique_ptr<ParameterManager> params;

			Settings settings;
			IOConfig ioConfig;
			OscilloscopeData scopeData;
			AuxMatrix auxMatrix;

			std::atomic<double> averageClocks, clocksPerSample;

			// ----
			PluginState* currentPlugin;
			TracerState* currentTracer;
			cpl::CLockFreeQueue<EngineCommand> incoming, outgoing;
			AuxMatrix tempBuffer;
		};
	}
#endif
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
	
	namespace ape {

		// forward declaration of the GUI class.
		class UIController;
		class PluginState;
		class CBaseControl;
		class CSerializer;

		/*
			Main engine class
		*/
		class Engine : public juce::AudioProcessor
		{
			/*
				friends
			*/
			friend class PluginState;
			friend class UIController;
			friend class CSerializer;
			typedef unsigned fpumask;
			// protect copy constructor
			Engine(const Engine &);
		private:
			void static errPrint(void * data, const char * text);
			bool copyInput(std::vector<float *> & in, std::vector<float *> & out, juce::AudioSampleBuffer & buffer);
			bool copyOutput(std::vector<float *> & out, juce::AudioSampleBuffer & buffer);
		public:

			virtual ~Engine();
			/*
				overloads
			*/

			Engine();
			//==============================================================================
			void prepareToPlay(double sampleRate, int samplesPerBlock);
			void releaseResources();

			void processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages);

			//==============================================================================
			juce::AudioProcessorEditor* createEditor();
			bool hasEditor() const;

			//==============================================================================
			const juce::String getName() const;

			int getNumParameters();

			float getParameter(int index);
			void setParameter(int index, float newValue);

			const juce::String getParameterName(int index);
			const juce::String getParameterText(int index);

			const juce::String getInputChannelName(int channelIndex) const;
			const juce::String getOutputChannelName(int channelIndex) const;
			bool isInputChannelStereoPair(int index) const;
			bool isOutputChannelStereoPair(int index) const;

			bool acceptsMidi() const;
			bool producesMidi() const;
			bool silenceInProducesSilenceOut() const;
			double getTailLengthSeconds() const;

			//==============================================================================
			int getNumPrograms();
			int getCurrentProgram();
			void setCurrentProgram(int index);
			const juce::String getProgramName(int index);
			void changeProgramName(int index, const juce::String& newName);

			//==============================================================================
			void getStateInformation(juce::MemoryBlock& destData);
			void setStateInformation(const void* data, int sizeInBytes);

			void exchangePlugin(std::unique_ptr<PluginState> plugin);
			void changeInitialDelay(long samples);
			void disablePlugin(bool fromEditor = true);
			bool activatePlugin();
			bool pluginCrashed();
			UIController& getController() { return *controller.get(); }
			PluginState * getCState() { return pluginState.get(); }
			CCodeGenerator& getCodeGenerator() noexcept { return codeGenerator; }
			void useProtectedBuffers(bool bValue) { status.bUseBuffers = bValue; }
			libconfig::Setting& getRootSettings();
			void loadSettings();
			int uniqueInstanceID();
			int instanceCounter();
			std::string engineType();

			bool isInProcessingCallback() const noexcept { return isProcessing.load(std::memory_order_acquire); }

			/*
				public data
			*/

		private:
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
			unsigned uiRefreshInterval;
			unsigned autoSaveInterval;
			unsigned numBuffers;
			
			union
			{
				unsigned int ID;
				struct
				{
					unsigned char instanceCounter;
					unsigned int pID : 24;
				};
				
			} instanceID;

			CCodeGenerator codeGenerator;
			cpl::ConcurrentObjectSwapper<PluginState> rtPlugin;
			std::unique_ptr<UIController> controller;
			std::unique_ptr<PluginState> pluginState;
			std::string programName;
			libconfig::Config config;
			std::atomic<double> clocksPerSample;
			std::atomic<bool> isProcessing;
			std::shared_mutex pluginMutex;
		}; // class ape
	} //namespace ape'
#endif
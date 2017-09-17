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

	file:APE.h
		
		Implements the interface for the VST.

*************************************************************************************/

#ifndef APE_ENGINE_H
	#define APE_ENGINE_H

	#include "Common.h"
	#include "CApi.h"
	#include "CMemoryGuard.h"
	#include <string>
	#include "Settings.h"
	#include "CMutex.h"
	#include <memory>
	
	namespace APE {

		// forward declaration of the GUI class.
		class GraphicUI;
		class CState;
		class CBaseControl;
		class CSerializer;

		#ifdef APE_IPLUG
			typedef AudioProcessor implementationDriver;
		#elif defined(APE_VST)
			typedef AudioEffectX implementationDriver;
		#elif defined(APE_JUCE)
			typedef juce::AudioProcessor implementationDriver;
			typedef juce::AudioSampleBuffer audioBuffer;
		#endif

		namespace Globals
		{
			extern bool ApplyTCCConvHack;
			extern bool CheckForTCC;
		};

		/*
			Main engine class
		*/
		class Engine : public implementationDriver, public CMutex::Lockable
		{
			/*
				friends
			*/
			friend class CState;
			friend class GraphicUI;
			friend class CSerializer;
			typedef unsigned fpumask;
			// protect copy constructor
			Engine(const Engine &);
		private:
			void static errPrint(void * data, const char * text);
			void initMem(int am = 2);
			bool copyInput(std::vector<float *> & in, std::vector<float *> & out, audioBuffer & buffer);
			bool copyOutput(std::vector<float *> & out, audioBuffer & buffer);
		public:
			virtual ~Engine() __llvm_DummyNoExcept; //can't hurt
			/*
				overloads
			*/
			#ifdef APE_VST
				Engine(audioMasterCallback master);
				virtual void process (float **inputs, float **outputs, VstInt32 sampleFrames);
				virtual void processReplacing (float **inputs, float **outputs, VstInt32 sampleFrames);
				virtual void processDoubleReplacing (double **inputs, double **outputs, VstInt32 sampleFrames);
				virtual void resume();

				virtual void setProgramName (char *name);
				virtual void setProgram(VstInt32 index);
				virtual VstInt32 getProgram();
				virtual void getProgramName (char *name);
				virtual void setParameter (VstInt32 index, float value);
				virtual bool getEffectName (char * name);
				virtual bool getVendorString (char * text);
				virtual bool getProductString (char * text);
				virtual VstInt32 getVendorVersion () { return 1000; }
				virtual VstPlugCategory getPlugCategory () { return kPlugCategEffect; }
			#elif defined(APE_JUCE)
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
			#endif
			/*
				public functions
			*/

			Status onCtrlEvent(CBaseControl * base);

			void changeInitialDelay(long samples);
			Status requestStatusChange(Status);
			void about();
			void disablePlugin(bool fromEditor = true);
			Status requestLinkage();
			bool activatePlugin();
			bool pluginCrashed();
			GraphicUI * getGraphicUI() { return gui.get(); }
			CState * getCState() { return csys.get(); }
			void useProtectedBuffers(bool bValue) { status.bUseBuffers = bValue; }
			libconfig::Setting & getRootSettings();
			void loadSettings();
			int uniqueInstanceID();
			int instanceCounter();
			std::string engineType();

			bool isInProcessingCallback() const { return status.bIsProcessing; }

			/*
				public data
			*/

		protected:
			/*
				protected data
			*/
			struct {
				volatile bool bActivated;
				volatile bool bIsProcessing;
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

			std::unique_ptr<GraphicUI> gui;
			Status state;
			std::unique_ptr<CState> csys;
			std::string programName;
			libconfig::Config config;
			volatile long clocksPerSample;
		}; // class APE
	} //namespace APE'
#endif
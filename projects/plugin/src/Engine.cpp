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

	file:ape.cpp
	
		Implementation of the audio effect interface.

*************************************************************************************/

#include <cpl/Common.h>
#include "Engine.h"
#include "CApi.h"
#include "PluginState.h"
#include "UIController.h"
#include "CConsole.h"
#include "Settings.h"
#include <ape/Project.h>
#include <cpl/Misc.h>
#include "CSerializer.h"
#include "Engine/ParameterManager.h"
#include "UI/UICommands.h"

namespace cpl
{
	#ifndef APE_TESTS
	const ProgramInfo programInfo
	{
		"Audio Programming Environment",
		cpl::Version::fromParts(APE_MAJOR, APE_MINOR, APE_BUILD),
		"Janus Thorborg",
		"sgn",
		false,
		nullptr,
		APE_BUILD_INFO
	};
	#endif
};

namespace ape 
{
	/*********************************************************************************************

		Implementation for the constructor.

	 *********************************************************************************************/
	Engine::Engine() 
		: numBuffers(2)
		, status()
		, delay()
		, programName("Default")
		, clocksPerSample(0)
		, codeGenerator(*this)
		, settings(true, cpl::Misc::DirFSPath() / "config.cfg")
	{

		// some variables...
		status.bUseBuffers = true;
		status.bUseFPUE = false;
		programName = "Default";
		
		instanceID = cpl::Misc::AcquireUniqueInstanceID();
		
		// rest of program
		controller = std::make_unique<UIController>(*this);
		codeGenerator.setErrorFunc(&UIController::errorPrint, controller.get());
		params = std::make_unique<ParameterManager>(*this, 50);

		// settings
		loadSettings();
		controller->console().printLine(CColours::black,
			("[Engine] : Audio Programming Environment <%s> (instance %d) " + cpl::programInfo.version.toString() + " (%s) %s loaded.").c_str(),
			engineType().c_str(), instanceID,
			sizeof(void*) == 8 ? "64-bit" : "32-bit",
			#if defined(_DEBUG) || defined(DEBUG)
				"debug");
			#else
				"release");
			#endif
	}
	
	std::string Engine::engineType() const noexcept
	{
		switch(wrapperType)
		{
			case wrapperType_VST:
				return "VST 2.4";
			case wrapperType_VST3:
				return "VST 3";
			case wrapperType_AudioUnit:
				return "Audio Unit";
			case wrapperType_RTAS:
				return "RTAS";
			case wrapperType_AAX:
				return "AAX";
			case wrapperType_Standalone:
				return "Stand-alone";
			default:
				return "Unknown";
		}
		
	}

	void Engine::loadSettings()
	{
		try 
		{

			bool useBuffers = settings.root()["application"]["use_buffers"];
			status.bUseBuffers = useBuffers;

			bool usefpe = settings.root()["application"]["use_fpe"];
			status.bUseFPUE = usefpe;
			
		}
		catch(std::exception & e)
		{
			controller->console().printLine(CColours::red, "[Engine] : Unknown error occured while reading settings! (%s)", e.what());
		}

	}

	std::int32_t Engine::uniqueInstanceID() const noexcept
	{
		return instanceID;
	}

	std::int32_t Engine::instanceCounter() const noexcept
	{
		return instanceID & 0xFF;
	}

	Engine::~Engine() 
	{
		disablePlugin();
		cpl::Misc::ReleaseUniqueInstanceID(instanceID);
	}

	void Engine::exchangePlugin(std::unique_ptr<PluginState> newPlugin)
	{
		std::unique_lock<std::shared_mutex> lock(pluginMutex);

		pluginState = std::move(newPlugin);
		if (pluginState)
		{
			pluginState->setBounds(ioConfig);
			pluginState->setPlayState(isPlaying);
			tracerState = TracerState();
		}
	}


	void Engine::changeInitialDelay(long samples) noexcept
	{
		delay.newDelay = samples;
		delay.bDelayChanged = true;
	}


	void Engine::disablePlugin(bool fromEditor)
	{
		std::shared_lock<std::shared_mutex> lock(pluginMutex);
		
		if (!pluginState)
			return;
		
		status.bActivated = false;

		auto result = pluginState->disableProject();

		controller->console().printLine(CColours::black, result == STATUS_OK ?
			"[Engine] : Plugin disabled without error." :
			"[Engine] : Unexpected return value from onUnload(), plugin disabled.");
		
		status.bActivated = false;

		if (!fromEditor)
		{
			// (main thread considerations)
			jassertfalse;
			//controller->getUICommandState().activationState.setNormalizedValue(0);
		}

		if (result == STATUS_OK)
			controller->setStatusText("Plugin disabled", CColours::lightgoldenrodyellow);

		changeInitialDelay(0);
	}

	bool Engine::activatePlugin()
	{
		std::shared_lock<std::shared_mutex> lock(pluginMutex);

		if (pluginState && pluginState->getState() != STATUS_DISABLED)
			return false;

		auto result = pluginState->activateProject();

		status.bActivated = false;

		switch(result)
		{
		case STATUS_DISABLED:
		case STATUS_ERROR:
			controller->console().printLine(CColours::red, "[Engine] : An error occured while loading the plugin.");
			break;
		case STATUS_SILENT:
		case STATUS_WAIT:
			controller->console().printLine(CColours::red, "[Engine] : Plugin is not ready or is silent.");
			break;
		case STATUS_READY:
			controller->console().printLine(CColours::black, "[Engine] : Plugin is loaded and reports no error.");
			status.bActivated = true;
			break;
		default:
			controller->console().printLine(CColours::red,
				"[ape] : Unexpected return value from onLoad (%d), assuming plugin is ready.", result);
			status.bActivated = true;
		}

		if (status.bActivated)
			updateHostDisplay();

		return status.bActivated;
	}

	void Engine::processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
	{
		std::shared_lock<std::shared_mutex> lock(pluginMutex);

		if (status.bActivated && pluginState)
		{
			const std::size_t numSamples = buffer.getNumSamples();
			std::size_t profiledClocks = 0;
			auxMatrix.softBufferResize(numSamples);

			if (status.bUseFPUE)
				pluginState->useFPUExceptions(true);

			auxMatrix.copy(buffer.getArrayOfReadPointers(), 0, ioConfig.inputs);

			tracerState.beginPhase(&auxMatrix, ioConfig.inputs + ioConfig.outputs);

			pluginState->processReplacing(buffer.getArrayOfReadPointers(), buffer.getArrayOfWritePointers(), numSamples, &profiledClocks);

			if (tracerState.changesPending())
				consumeTracerChanges(tracerState);

			tracerState.endPhase();

			if (status.bUseFPUE)
				pluginState->useFPUExceptions(false);

			clocksPerSample = static_cast<double>(profiledClocks) / numSamples;

			auxMatrix.copy(buffer.getArrayOfReadPointers(), ioConfig.inputs, ioConfig.outputs);

			scopeData.getStream().processIncomingRTAudio(auxMatrix.data(), ioConfig.inputs + ioConfig.outputs + tracerState.getTraceCount(), numSamples, *getPlayHead());
		}

		// In case we have more outputs than inputs, we'll clear any output
		// channels that didn't contain input data, (because these aren't
		// guaranteed to be empty - they may contain garbage).
		for (int i = getNumInputChannels(); i < getNumOutputChannels(); ++i)
		{
			buffer.clear(i, 0, buffer.getNumSamples());
		}

	}

	void Engine::consumeTracerChanges(TracerState& state)
	{
		if (state.getTraceCount() != 0)
		{
			auto& stream = scopeData.getStream();
			auto info = stream.getInfo();
			info.anticipatedChannels = ioConfig.inputs + ioConfig.outputs + state.getTraceCount();
			stream.initializeInfo(info);

			for (std::size_t i = 0; i < state.getTraceCount(); ++i)
				stream.enqueueChannelName(ioConfig.inputs + ioConfig.outputs + i, state.dequeueName());
		}

		// TODO: Move to another place
		scopeData.initializeColours(ioConfig.inputs + ioConfig.outputs + state.getTraceCount());
	}


	bool Engine::hasEditor() const
	{
		return true;
	}

	void Engine::getStateInformation(juce::MemoryBlock& destData)
	{
		std::shared_lock<std::shared_mutex> lock(pluginMutex);

		CSerializer::serialize(this, destData);
	}

	void Engine::setStateInformation(const void* data, int sizeInBytes)
	{
		bool ret = false;
		try
		{
			ret = CSerializer::restore(this, data, sizeInBytes);
		}
		catch (std::exception & e)
		{

			controller->console().printLine(CColours::red, "[Engine] : Exception while serializing: %s", e.what());
		}
		if (!ret)
			controller->console().printLine(CColours::red, "[Engine] : Error serializing state!");
		else
			controller->console().printLine(CColours::black, "[Engine] : Succesfully serialized state!");
	}

	const juce::String Engine::getName() const
	{
		#ifdef JucePlugin_Name
			return JucePlugin_Name;
		#else
			return _PROGRAM_NAME;
		#endif
	}

	void Engine::onSettingsChanged(const Settings& s, const libconfig::Setting& changed)
	{

	}

	int Engine::getNumParameters()
	{
		return static_cast<int>(params->numParams());
	}

	float Engine::getParameter(int index)
	{
		return params->getParameter(index);
	}

	void Engine::setParameter(int index, float newValue)
	{
		params->setParameter(index, newValue);
	}

	const juce::String Engine::getParameterName(int index)
	{
		return params->getParameterName(index);
	}

	const juce::String Engine::getParameterText(int index)
	{
		return params->getParameterText(index);
	}

	const juce::String Engine::getInputChannelName(int channelIndex) const
	{
		return juce::String(channelIndex + 1);
	}

	const juce::String Engine::getOutputChannelName(int channelIndex) const
	{
		return juce::String(channelIndex + 1);
	}

	bool Engine::isInputChannelStereoPair(int index) const
	{
		return true;
	}

	bool Engine::isOutputChannelStereoPair(int index) const
	{
		return true;
	}

	bool Engine::acceptsMidi() const
	{
		#if JucePlugin_WantsMidiInput
			return true;
		#else
			return false;
		#endif
	}

	bool Engine::producesMidi() const
	{
		#if JucePlugin_ProducesMidiOutput
			return true;
		#else
			return false;
		#endif
	}

	bool Engine::silenceInProducesSilenceOut() const
	{
		return true;
	}

	double Engine::getTailLengthSeconds() const
	{
		return 0.0;
	}

	int Engine::getNumPrograms()
	{
		return 1;
	}

	int Engine::getCurrentProgram()
	{
		return 0;
	}

	void Engine::setCurrentProgram(int index)
	{
	}

	const juce::String Engine::getProgramName(int index)
	{
		return juce::String::empty;
	}

	void Engine::changeProgramName(int index, const juce::String& newName)
	{
	}
	
	void Engine::prepareToPlay(double sampleRate, int samplesPerBlock)
	{

		// Use this method as the place to do any pre-playback
		// initialisation that you need..
		if (delay.bDelayChanged)
		{
			controller->console().printLine(CColours::black, "initialDelay is changed to %d and reported to host.", delay.newDelay);
			if (delay.newDelay != delay.initialDelay)
			{
				this->setLatencySamples(delay.newDelay);
				delay.initialDelay = delay.newDelay;
				//ioChanged();
			}
			
			delay.bDelayChanged = false;
		}		
		
		std::shared_lock<std::shared_mutex> lock(pluginMutex);
		
		isPlaying = true;
		ioConfig.blockSize = samplesPerBlock;
		ioConfig.sampleRate = sampleRate;
		ioConfig.inputs = getNumInputChannels();
		ioConfig.outputs = getNumOutputChannels();

		if (pluginState)
		{
			pluginState->setBounds(ioConfig);
			pluginState->setPlayState(isPlaying);
		}

		auto info = scopeData.getStream().getInfo();
		info.anticipatedChannels = ioConfig.inputs + ioConfig.outputs;
		info.anticipatedSize = ioConfig.blockSize;
		info.sampleRate = ioConfig.sampleRate;
		info.callAsyncListeners = true;
		info.isFrozen = info.isSuspended = false;
		info.audioHistorySize = sampleRate;
		info.audioHistoryCapacity = sampleRate;
		scopeData.getStream().initializeInfo(info);
		
		std::size_t counter = 0;

		for (std::size_t i = 0; i < ioConfig.inputs; ++i, counter++)
		{
			scopeData.getStream().enqueueChannelName(counter, "input " + std::to_string(i));
		}

		for (std::size_t i = 0; i < ioConfig.outputs; ++i, counter++)
		{
			scopeData.getStream().enqueueChannelName(counter, "output " + std::to_string(i));
		}


		scopeData.getContent().triggeringChannel = ioConfig.inputs;

		auxMatrix.resizeChannels(Signalizer::OscilloscopeContent::NumColourChannels);
	}

	void Engine::releaseResources()
	{
		isPlaying = false;
		std::shared_lock<std::shared_mutex> lock(pluginMutex);
		if(pluginState)
			pluginState->setPlayState(isPlaying);
	}

	void Engine::handleTraceCallback(const char** names, std::size_t nameCount, const float * values, std::size_t valueCount)
	{
		tracerState.handleTrace(names, nameCount, values, valueCount);
	}


}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	ape::Engine * effect = nullptr;
	try
	{
		effect = new ape::Engine();
	}
	catch (const std::exception & e)
	{
		std::stringstream csf;
		csf << "Exception while creating effect: " << e.what() << ".\nCheck /logs/ for more information ";
		csf << "(logging can be enabled in config.cfg.application.log_console)";
		fputs(csf.str().c_str(), stderr);
		cpl::Misc::MsgBox(csf.str(), cpl::programInfo.programAbbr + " construction error!",
			cpl::Misc::MsgStyle::sOk | cpl::Misc::MsgIcon::iStop, nullptr, true);
	}
		
	return effect;
		
}

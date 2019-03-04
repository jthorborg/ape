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

	file:engine.cpp

		Implementation of engine.

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
#include <cpl/system/SysStats.h>

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
	Engine::Engine()
		: delay()
		, clocksPerSample(0)
		, averageClocks(0)
		, codeGenerator(*this)
		, settings(true, cpl::Misc::DirFSPath() / "config.cfg")
		, incoming(0xF, 0xF)
		, outgoing(0xF, 0xF)
		, currentPlugin(nullptr)
		, currentTracer(nullptr)
	{
		instanceID = cpl::Misc::AcquireUniqueInstanceID();

		// rest of program
		controller = std::make_unique<UIController>(*this);
		params = std::make_unique<ParameterManager>(*this, 50);

		// settings
		loadSettings();
		controller->getConsole().printLine("[Engine] : Audio Programming Environment <%s> (instance %d) %s (%s) %s loaded.",
			engineType().c_str(), instanceID,
			cpl::programInfo.version.toString().c_str(),
			sizeof(void*) == 8 ? "64-bit" : "32-bit",
#if defined(_DEBUG) || defined(DEBUG)
			"debug");
#else
			"release");
#endif
	}

	Engine::~Engine()
	{
		processReturnQueue();
		cpl::Misc::ReleaseUniqueInstanceID(instanceID);
		// these need to be deleted before other members in the engine
		controller = nullptr;
		pluginStates.clear();
	}

	std::string Engine::engineType() const noexcept
	{
		switch (wrapperType)
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
		useFPE = settings.lookUpValue(false, "application", "use_fpe");
		preserveParameters = settings.lookUpValue(true, "application", "preserve_parameters");
	}

	std::int32_t Engine::uniqueInstanceID() const noexcept
	{
		return instanceID;
	}

	std::int32_t Engine::instanceCounter() const noexcept
	{
		return instanceID & 0xFF;
	}

	Engine::ProfilerData Engine::getProfilingData() const noexcept
	{
		const auto smoothedClocks = averageClocks.load(std::memory_order_acquire);
		return {
			1e-6 * (smoothedClocks * getSampleRate() / cpl::system::CProcessor::getMHz()),
			clocksPerSample.load(std::memory_order_acquire),
			smoothedClocks,
			getSampleRate()
		};
	}

	void Engine::exchangePlugin(std::shared_ptr<PluginState> newPlugin)
	{
		auto plugin = newPlugin.get();

		if (plugin)
		{
			pluginStates.emplace_back(std::move(newPlugin));
			plugin->setBounds(ioConfig);
			plugin->setPlayState(isPlaying);
		}

		incoming.pushElement<true, true>(EngineCommand::TransferPlugin::Create(plugin));
	}

	void Engine::changeInitialDelay(long samples) noexcept
	{
		delay.newDelay = samples;
		delay.delayChanged = true;
	}

	void Engine::processReturnQueue()
	{
		EngineCommand command;
		while (outgoing.popElement(command))
		{
			switch (command.type)
			{
			case EngineCommand::Type::Transfer:
				updateHostDisplay();

				for (std::size_t i = 0; i < pluginStates.size(); ++i)
				{
					if (pluginStates[i].get() == command.transfer.state)
					{
						controller->pluginExchanged(std::move(pluginStates[i]), command.transfer.reason);
						pluginStates.erase(pluginStates.begin() + i);
					}
				}

			}
		}
	}

	bool Engine::processPlugin(PluginState& plugin, TracerState& tracer, const std::size_t numSamples, const float * const * inputs)
	{
		const auto pole = std::pow(0.95, getSampleRate() / numSamples);
		std::size_t profiledClocks = 0;

		if (useFPE)
			plugin.useFPUExceptions(true);

		tracer.beginPhase(&auxMatrix, ioConfig.inputs + ioConfig.outputs);

		auto ret = plugin.processReplacing(inputs, tempBuffer.data(), numSamples, &profiledClocks);

		if (tracer.changesPending())
			onInitialTracerChanges(tracer);

		tracer.endPhase();

		if (useFPE)
			plugin.useFPUExceptions(false);

		const auto normalizedClocks = static_cast<double>(profiledClocks) / numSamples;

		clocksPerSample.store(normalizedClocks, std::memory_order_release);
		averageClocks.store(averageClocks + pole * (normalizedClocks - averageClocks), std::memory_order_release);

		return ret;
	}


	void Engine::processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
	{
		const std::size_t numSamples = buffer.getNumSamples();
		std::size_t numTraces = 0;
		auxMatrix.softBufferResize(numSamples);
		tempBuffer.softBufferResize(numSamples);

		auxMatrix.copy(buffer.getArrayOfReadPointers(), 0, ioConfig.inputs);
		auxMatrix.clear(ioConfig.inputs, ioConfig.outputs);

		bool newPluginArrived = false;
		bool hadOldPlugin = currentPlugin != nullptr;

		EngineCommand command;

		while (incoming.popElement(command))
		{
			switch (command.type)
			{
			case EngineCommand::Type::Transfer:
			{
				auto reason = PluginExchangeReason::Exchanged;

				if (currentPlugin)
				{
					if (!processPlugin(*currentPlugin, *currentTracer, numSamples, buffer.getArrayOfReadPointers()))
						reason = reason | PluginExchangeReason::Crash;

					auxMatrix.accumulate(tempBuffer.data(), ioConfig.inputs, ioConfig.outputs, 1.0f, 0.0f);
				}

				outgoing.pushElement(EngineCommand::TransferPlugin::Return(currentPlugin, currentTracer, reason));

				currentPlugin = command.transfer.state;
				newPluginArrived = true;
				currentTracer = command.transfer.tracer;
			}
			}
		}

		if (currentPlugin)
		{
			if (newPluginArrived)
			{
				// hot reloading, copy over parameters
				// TODO: Only do this if hash of old and new parameters match up
				currentPlugin->syncParametersToEngine(hadOldPlugin && preserveParameters);
			}

			if (!processPlugin(*currentPlugin, *currentTracer, numSamples, buffer.getArrayOfReadPointers()))
			{
				outgoing.pushElement(EngineCommand::TransferPlugin::Return(currentPlugin, currentTracer, PluginExchangeReason::Crash));
				currentPlugin = nullptr;
				currentTracer = nullptr;

			}
			else if (newPluginArrived && fadePlugins)
			{
				auxMatrix.accumulate(tempBuffer.data(), ioConfig.inputs, ioConfig.outputs, 0.0f, 1.0f);
			}
			else
			{
				auxMatrix.copy(tempBuffer.data(), ioConfig.inputs, ioConfig.outputs);
			}
		}

		scopeData.getStream().processIncomingRTAudio(auxMatrix.data(), ioConfig.inputs + ioConfig.outputs + numTraces, numSamples, *getPlayHead());

		for (int i = 0; i < getNumOutputChannels(); ++i)
		{
			buffer.copyFrom(i, 0, auxMatrix[ioConfig.inputs + i], numSamples);
		}

		// In case we have more outputs than inputs, we'll clear any output
		// channels that didn't contain input data, (because these aren't
		// guaranteed to be empty - they may contain garbage).
		for (int i = getNumInputChannels(); i < getNumOutputChannels(); ++i)
		{
			buffer.clear(i, 0, buffer.getNumSamples());
		}

	}

	void Engine::pulse()
	{
		processReturnQueue();
		params->pulse();
	}

	void Engine::onInitialTracerChanges(TracerState& state)
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

	void Engine::handleTraceCallback(const char** names, std::size_t nameCount, const float * values, std::size_t valueCount)
	{
		// TODO: Callback shouldn't be able to happen without an associated tracer
		if (currentTracer)
			currentTracer->handleTrace(names, nameCount, values, valueCount);
	}

	bool Engine::hasEditor() const
	{
		return true;
	}

	void Engine::getStateInformation(juce::MemoryBlock& destData)
	{
		CSerializer::serialize(*this, destData);
	}

	void Engine::setStateInformation(const void* data, int sizeInBytes)
	{
		bool ret = false;
		try
		{
			ret = CSerializer::restore(*this, data, sizeInBytes);
		}
		catch (std::exception & e)
		{

			controller->getConsole().printLine(CConsole::Error, "[Engine] : Exception while serializing: %s", e.what());
		}
		if (!ret)
			controller->getConsole().printLine(CConsole::Error, "[Engine] : Error serializing state!");
		else
			controller->getConsole().printLine("[Engine] : Succesfully serialized state!");
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

		if (delay.delayChanged)
		{
			controller->getConsole().printLine("initialDelay is changed to %d and reported to host.", delay.newDelay);
			if (delay.newDelay != delay.initialDelay)
			{
				this->setLatencySamples(delay.newDelay);
				delay.initialDelay = delay.newDelay;
			}

			delay.delayChanged = false;
		}

		isPlaying = true;
		ioConfig.blockSize = samplesPerBlock;
		ioConfig.sampleRate = sampleRate;
		ioConfig.inputs = getNumInputChannels();
		ioConfig.outputs = getNumOutputChannels();

		for (std::size_t i = 0; i < pluginStates.size(); ++i)
		{
			pluginStates[i]->setBounds(ioConfig);
			pluginStates[i]->setPlayState(isPlaying);
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
		tempBuffer.resizeChannels(ioConfig.outputs);
	}

	void Engine::releaseResources()
	{
		isPlaying = false;

		for (std::size_t i = 0; i < pluginStates.size(); ++i)
		{
			pluginStates[i]->setPlayState(isPlaying);
		}
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

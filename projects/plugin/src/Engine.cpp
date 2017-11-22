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
		, uiRefreshInterval(80)
		, clocksPerSample(0)
		, autoSaveInterval(0)
		, codeGenerator(this)
	{
		codeGenerator.setErrorFunc(&Engine::errPrint, this);
		// some variables...
		status.bUseBuffers = true;
		status.bUseFPUE = false;
		programName = "Default";
		
		instanceID.ID = cpl::Misc::ObtainUniqueInstanceID();
		
		// rest of program
		controller = std::make_unique<UIController>(this);

		// settings
		loadSettings();
		controller->console->printLine(CColours::black,
			("[Engine] : Audio Programming Environment <%s> (instance %d) " + cpl::programInfo.version.toString() + " (%s) %s loaded.").c_str(),
			engineType().c_str(), instanceID.ID,
			sizeof(void*) == 8 ? "64-bit" : "32-bit",
			#if defined(_DEBUG) || defined(DEBUG)
				"debug");
			#else
				"release");
			#endif
	}
	
	std::string Engine::engineType()
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
	/*********************************************************************************************

		Applies common settings found in confing.application to engine.

	 *********************************************************************************************/
	void Engine::loadSettings()
	{
		int unid = -1;
		try {
			config.readFile((cpl::Misc::DirectoryPath() + "/config.cfg").c_str());

			const libconfig::Setting & approot = getRootSettings()["application"];
			bool enableLogging = approot["log_console"];
			
			std::string log_path = cpl::Misc::DirectoryPath() + "/logs/log" + std::to_string(instanceID.ID) + ".txt";
			
			controller->console->setLogging(enableLogging, log_path);
			bool enableStdWriting = approot["console_std_writing"];
			controller->console->setStdWriting(enableStdWriting);
			bool useBuffers = approot["use_buffers"];
			status.bUseBuffers = useBuffers;

			bool usefpe = approot["use_fpe"];
			status.bUseFPUE = usefpe;
			int refreshTimer = approot["ui_refresh_interval"];
			if (refreshTimer > 10 && refreshTimer < 10000)
				this->uiRefreshInterval = refreshTimer;
			autoSaveInterval = approot["autosave_interval"];
			unid = approot["unique_id"];
			bool g_shown = approot["greeting_shown"];
			if(!g_shown) {
				cpl::Misc::MsgBox("Hello and welcome to " + cpl::programInfo.name + "! Before you start using this program, "
				"please take time to read the readme and agree to all licenses + disclaimers found in /licenses. "
				"Have fun!", cpl::programInfo.name, cpl::Misc::MsgIcon::iInfo);
				approot["greeting_shown"] = true;
			}
			
		}
		catch(libconfig::FileIOException & e)
		{
			controller->console->printLine(CColours::red, "[Engine] : Error reading config file (%s)! (%s)", (cpl::Misc::DirectoryPath() + "/config.cfg").c_str(), e.what());
		}
		catch(libconfig::SettingNotFoundException & e)
		{
			controller->console->printLine(CColours::red, "[Engine] : Error getting setting! (%s)", e.getPath());
		}
		catch(libconfig::ParseException & e)
		{
			controller->console->printLine(CColours::red, "[Engine] : Error parsing config! In file %s at line %d: %s", e.getFile(), e.getLine(), e.getError());
		}
		catch(std::exception & e)
		{
			controller->console->printLine(CColours::red, "[Engine] : Unknown error occured while reading settings! (%s)", e.what());
		}

	}

	int Engine::uniqueInstanceID()
	{
		return this->instanceID.ID;
	}

	int Engine::instanceCounter()
	{
		return this->instanceID.instanceCounter;
	}

	libconfig::Setting & Engine::getRootSettings() 
	{
		return config.getRoot();
	}

	Engine::~Engine() 
	{
		disablePlugin();
		cpl::Misc::ReleaseUniqueInstanceID(instanceID.ID);
	}


	bool Engine::pluginCrashed() 
	{

		return true;
	}


	void Engine::errPrint(void * data, const char * text)
	{
		// fetch this, as this function is static
		
		ape::Engine *_this = reinterpret_cast<ape::Engine*>(data);
		// print the message
		_this->controller->console->printLine(CColours::red, (std::string("[Compiler] : ") + text).c_str());
		int nLinePos(-1), i(0);
		auto nLen = std::strlen(text);
		for(; i < nLen; ++i) {
			// layout of error message: "<%file%>:%line%: error: %msg%
			if(text[i] == '>') {
				i += 2; // skip '>:'
				if (i >= nLen)
					break;
				sscanf(text + i, "%d", &nLinePos);
				break;
			}
		}
		// set the error if the editor is open (not our responsibility), defaults to -1 
		//which should be ignored by the func.
		_this->controller->setEditorError(nLinePos);
	}

	void Engine::exchangePlugin(std::unique_ptr<PluginState> newPlugin)
	{
		std::unique_lock<std::shared_mutex> lock(pluginMutex);

		pluginState = std::move(newPlugin);
		if (pluginState)
		{
			pluginState->setBounds(ioConfig);
			pluginState->setPlayState(isPlaying);
		}
	}


	void Engine::changeInitialDelay(long samples)
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

		controller->console->printLine(CColours::black, result == STATUS_OK ?
			"[Engine] : Plugin disabled without error." :
			"[Engine] : Unexpected return value from onUnload(), plugin disabled.");
		
		status.bActivated = false;
		if(!fromEditor)
			controller->setParameter(kActiveStateButton, 0.f);

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
			controller->console->printLine(CColours::red, "[Engine] : An error occured while loading the plugin.");
			break;
		case STATUS_SILENT:
		case STATUS_WAIT:
			controller->console->printLine(CColours::red, "[Engine] : Plugin is not ready or is silent.");
			break;
		case STATUS_READY:
			controller->console->printLine(CColours::black, "[Engine] : Plugin is loaded and reports no error.");
			status.bActivated = true;
			break;
		default:
			controller->console->printLine(CColours::red,
				"[ape] : Unexpected return value from onLoad (%d), assuming plugin is ready.", result);
			status.bActivated = true;
		}
		return status.bActivated;
	}

	void Engine::processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
	{
		std::shared_lock<std::shared_mutex> lock(pluginMutex);

		if (status.bActivated && pluginState)
		{

			std::size_t numSamples = buffer.getNumSamples();
			std::size_t profiledClocks = 0;

			if (status.bUseFPUE)
				pluginState->useFPUExceptions(true);

			isProcessing.store(true, std::memory_order_release);

			pluginState->processReplacing(buffer.getArrayOfReadPointers(), buffer.getArrayOfWritePointers(), numSamples, &profiledClocks);

			isProcessing.store(false, std::memory_order_release);


			if (status.bUseFPUE)
				pluginState->useFPUExceptions(false);

			clocksPerSample = static_cast<double>(profiledClocks) / numSamples;
		}

		// In case we have more outputs than inputs, we'll clear any output
		// channels that didn't contain input data, (because these aren't
		// guaranteed to be empty - they may contain garbage).
		for (int i = getNumInputChannels(); i < getNumOutputChannels(); ++i)
		{
			buffer.clear(i, 0, buffer.getNumSamples());
		}

	}

	bool Engine::hasEditor() const
	{
		return true;
	}

	juce::AudioProcessorEditor* Engine::createEditor()
	{
		return controller->create();
	}

	void Engine::getStateInformation(juce::MemoryBlock& destData)
	{
		std::shared_lock<std::shared_mutex> lock(pluginMutex);

		CSerializer::serialize(this, destData);
	}

	void Engine::setStateInformation(const void* data, int sizeInBytes)
	{
		std::shared_lock<std::shared_mutex> lock(pluginMutex);

		bool ret = false;
		try
		{
			ret = CSerializer::restore(this, data, sizeInBytes);
		}
		catch (std::exception & e)
		{

			controller->console->printLine(CColours::red, "[Engine] : Exception while serializing: %s", e.what());
		}
		if (!ret)
			controller->console->printLine(CColours::red, "[Engine] : Error serializing state!");
		else
			controller->console->printLine(CColours::black, "[Engine] : Succesfully serialized state!");
	}

	const juce::String Engine::getName() const
	{
		#ifdef JucePlugin_Name
			return JucePlugin_Name;
		#else
			return _PROGRAM_NAME;
		#endif
	}

	int Engine::getNumParameters()
	{
		return 0;
	}

	float Engine::getParameter(int index)
	{
		return 0.0f;
	}

	void Engine::setParameter(int index, float newValue)
	{
	}

	const juce::String Engine::getParameterName(int index)
	{
		return juce::String::empty;
	}

	const juce::String Engine::getParameterText(int index)
	{
		return juce::String::empty;
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
			controller->console->printLine(CColours::black, "initialDelay is changed to %d and reported to host.", delay.newDelay);
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

	}

	void Engine::releaseResources()
	{
		isPlaying = false;
		std::shared_lock<std::shared_mutex> lock(pluginMutex);
		if(pluginState)
			pluginState->setPlayState(isPlaying);
	}

}
#ifdef __WINDOWS__
	static bool searchChanged = false;
#endif

#ifdef APE_VST
	AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
	{
		/*
			This part is super crucial: Since we are hosted by another application, windows will search
			for our dependencies (dll's like scilexer and libtcc) in our host's folder - vstplugins folder
			is usually not inside that, so we add another path based off DirectoryPath.
		*/
		#ifdef __WINDOWS__
			if (!searchChanged) {
				SetDllDirectory(ape::Misc::DirectoryPath.c_str());
				searchChanged = true;
			}
		#endif
		return new ape::Engine(audioMaster);
	}
#elif defined(APE_JUCE)
	juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
	{
		#ifdef __WINDOWS__
			if (!searchChanged) {
				SetDllDirectory(cpl::Misc::DirectoryPath().c_str());
				searchChanged = true;
			}
		#endif
		
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
#endif
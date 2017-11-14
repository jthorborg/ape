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
		, state(Status::STATUS_DISABLED)
		, delay()
		, programName("Default")
		, uiRefreshInterval(80)
		, clocksPerSample(0)
		, autoSaveInterval(0)
	{
		// some variables...
		status.bUseBuffers = true;
		status.bUseFPUE = false;
		programName = "Default";
		
		instanceID.ID = cpl::Misc::ObtainUniqueInstanceID();
		
		// rest of program
		gui = std::make_unique<UIController>(this);
		csys = std::make_unique<PluginState>(this);

		// settings
		loadSettings();
		gui->console->printLine(CColours::black,
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
			
			gui->console->setLogging(enableLogging, log_path);
			bool enableStdWriting = approot["console_std_writing"];
			gui->console->setStdWriting(enableStdWriting);
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
			gui->console->printLine(CColours::red, "[Engine] : Error reading config file (%s)! (%s)", (cpl::Misc::DirectoryPath() + "/config.cfg").c_str(), e.what());
		}
		catch(libconfig::SettingNotFoundException & e)
		{
			gui->console->printLine(CColours::red, "[Engine] : Error getting setting! (%s)", e.getPath());
		}
		catch(libconfig::ParseException & e)
		{
			gui->console->printLine(CColours::red, "[Engine] : Error parsing config! In file %s at line %d: %s", e.getFile(), e.getLine(), e.getError());
		}
		catch(std::exception & e)
		{
			gui->console->printLine(CColours::red, "[Engine] : Unknown error occured while reading settings! (%s)", e.what());
		}

	}

	/*********************************************************************************************

		Returns a unique id, that represents this instance.

	 *********************************************************************************************/
	int Engine::uniqueInstanceID()
	{
		return this->instanceID.ID;
	}
	/*********************************************************************************************
	 
		Returns a unique counter, that represents this instance inside this process.
	 
	 *********************************************************************************************/
	int Engine::instanceCounter()
	{
		return this->instanceID.instanceCounter;
	}
	/*********************************************************************************************

		Returns the top-level setting

	 *********************************************************************************************/
	libconfig::Setting & Engine::getRootSettings() 
	{
		return config.getRoot();
	}
	/*********************************************************************************************

		Implementation for the destructor.

	 *********************************************************************************************/
	Engine::~Engine() 
	{
		disablePlugin();
		cpl::Misc::ReleaseUniqueInstanceID(instanceID.ID);
	}
	/*********************************************************************************************

		Inform the user that he cannot write code. This gets fired if a SEGFAULT outside of our protected code
		exception is caught: by default, we pass other exceptions as they may be more severe than
		what we can guarantee.

	 *********************************************************************************************/
	bool Engine::pluginCrashed() 
	{
		cpl::Misc::MsgBox("Your plugin has performed an illegal operation and has crashed! "
					 "it is adviced that you immediately save your project (perhaps in a new "
					 "file) and restart your "
					 "application, as memory might have been corrupted. Consider going through "
					 "your code and double-check it for errors, especially pointer dereferences "
					 "and loops that might cause segmentation faults.",
					 cpl::programInfo.programAbbr + " Fatal Error",
					 cpl::Misc::MsgStyle::sOk | cpl::Misc::MsgIcon::iStop,
					 gui->getSystemWindow(),
					 false);
		return true;
	}

	/*********************************************************************************************

		Callback function associated with the compiler:
		This will get called with the opaque data set as this,
		and the text which is specific for the compiler.
		Therefore, the parsing should be moved to the 
		CCompiler class instead.

	 *********************************************************************************************/
	void Engine::errPrint(void * data, const char * text)
	{
		// fetch this, as this function is static
		
		ape::Engine *_this = reinterpret_cast<ape::Engine*>(data);
		// print the message
		_this->gui->console->printLine(CColours::red, (std::string("[Compiler] : ") + text).c_str());
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
		_this->gui->setEditorError(nLinePos);
	}

	/*********************************************************************************************

		Sets a new delay to be set at the next resume() call.

	 *********************************************************************************************/
	void Engine::changeInitialDelay(long samples)
	{
		delay.newDelay = samples;
		delay.bDelayChanged = true;
	}
	/*********************************************************************************************

		Event handler for controls.

	 *********************************************************************************************/
	Status Engine::onCtrlEvent(CBaseControl * base)
	{
		/*
			everything in here should actually be delegated to ape::gui, 
			and this should only be a thin layer of indirection
		*/
		// create general event
		Event e;
		// set type to a change of ctrl value.
		e.eventType = CtrlValueChanged;
		// construct ctrlValueChange event object
		APE_Event_CtrlValueChanged aevent;
		::memset(&aevent, 0, sizeof aevent);
		// set new values
		aevent.value = base->bGetValue();
		aevent.tag = base->bGetTag();
		e.event.eCtrlValueChanged = &aevent;
		// run plugin's event handler
		Status ret = STATUS_OK;
		try 
		{
			ret = csys->onEvent(&e); 
		} 
		catch (const cpl::CProtected::CSystemException & e) 
		{
			gui->console->printLine(CColours::red, 
				"[Engine] : Exception 0x%X occured while calling eventHandler code: %s Plugin disabled.", 
				e.data.exceptCode, cpl::CProtected::formatExceptionMessage(e).c_str());
			state = STATUS_ERROR;
			gui->setStatusText("Plugin crashed!", CColours::red);
			disablePlugin(false);
			pluginCrashed();
			csys->projectCrashed();
		}
		catch (const AbortException& e)
		{
			state = STATUS_DISABLED;
			gui->setStatusText("Plugin aborted", CColours::orange);
			gui->console->printLine(CColours::red, "[Engine] : Plugin aborted while disabling plugin: %s.", e.what());
			disablePlugin(false);

		}
		// update event.
		if(ret == STATUS_HANDLED) {
			if(aevent.value != base->bGetValue()) 
				base->bSetValue(aevent.value);
			if(aevent.text[0])
				base->bSetText(aevent.text);
			if(aevent.title[0])
				base->bSetTitle(aevent.title);
		}

		return ret;
	}
	/*********************************************************************************************

		disables the plugin and if no error, calls the exit point. 
		depending on fromEditor, disables the activated button (if true, will not disable button as
		event might cause an infinite loop).

	 *********************************************************************************************/
	void Engine::disablePlugin(bool fromEditor)
	{
		// else the processor might start while we are doing this
		cpl::CMutex lockGuard(this);
		status.bActivated = false;
		try 
		{
			state = csys->disableProject(state != STATUS_READY);
		} 
		catch (const cpl::CProtected::CSystemException & e) 
		{
			gui->console->printLine(CColours::red,
				"[Engine] : Exception 0x%X occured while disabling plugin: %s Plugin disabled.",
				e.data.exceptCode, cpl::CProtected::formatExceptionMessage(e).c_str());
			gui->setStatusText("Plugin crashed!", CColours::red);
			state = STATUS_ERROR;
			pluginCrashed();
		}
		catch (const AbortException& e)
		{
			state = STATUS_DISABLED;
			gui->setStatusText("Plugin aborted", CColours::orange);
			gui->console->printLine(CColours::red, "[Engine] : Plugin aborted while disabling plugin: %s.", e.what());
		}
		gui->console->printLine(CColours::black, state == STATUS_OK ?
			"[Engine] : Plugin disabled without error." :
			"[Engine] : Unexpected return value from onUnLoad(), plugin disabled.");
		
		status.bActivated = false;
		if(!fromEditor)
			gui->setParameter(kActiveStateButton, 0.f);
		if (state == STATUS_OK)
			gui->setStatusText("Plugin disabled", CColours::lightgoldenrodyellow);
		state = STATUS_DISABLED;
		// clear all allocations made by plugin.
		// called twice?
		csys->getPluginAllocator().clear();
		changeInitialDelay(0);
	}
	/*********************************************************************************************

		activates the plugin and if no error, calls the entry point and sets the state 
		of the system accordingly.

	 *********************************************************************************************/
	bool Engine::activatePlugin()
	{
		if(state != STATUS_DISABLED)
			return false;
		status.bActivated = false;

		try 
		{
			state = csys->activateProject();
		} 
		catch (const cpl::CProtected::CSystemException & e)
		{
			gui->console->printLine(
				CColours::red, 
				"[Engine] : Exception 0x%X occured while activating plugin: %s Plugin disabled.", 
				e.data.exceptCode, cpl::CProtected::formatExceptionMessage(e).c_str()
			);
			state = STATUS_ERROR;
			gui->setStatusText("Plugin crashed!", CColours::red);
			disablePlugin(false);
			pluginCrashed();
			csys->projectCrashed();
		}
		catch (const AbortException& e)
		{
			state = STATUS_DISABLED;
			gui->setStatusText("Plugin aborted", CColours::orange);
			gui->console->printLine(CColours::red, "[Engine] : Plugin aborted while activating plugin: %s.", e.what());
		}
		switch(state)
		{
		case STATUS_DISABLED:
		case STATUS_ERROR:
			gui->console->printLine(CColours::red, "[Engine] : An error occured while loading the plugin.");
			break;
		case STATUS_SILENT:
		case STATUS_WAIT:
			gui->console->printLine(CColours::red, "[Engine] : Plugin is not ready or is silent.");
			break;
		case STATUS_READY:
			gui->console->printLine(CColours::black, "[Engine] : Plugin is loaded and reports no error.");
			status.bActivated = true;
			break;
		default:
			gui->console->printLine(CColours::red,
				"[ape] : Unexpected return value from onLoad (%d), assuming plugin is ready.", state);
			state = STATUS_READY;
			status.bActivated = true;
		}
		return status.bActivated;
	}

	/*********************************************************************************************
	 
		Our processor function
	 
	 *********************************************************************************************/
	void Engine::processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
	{
		if (status.bActivated)
		{
			cpl::CMutex lockGuard(this);
			unsigned numSamples = buffer.getNumSamples();
			std::vector<float *> in, out; 

			if(!copyInput(in, out, buffer))
			{
				gui->console->printLine(CColours::red, "[Engine] : Error copying input buffers!");
				lockGuard.release();
				disablePlugin(false);
				goto skip;
			}
			// ensure disablePlugin() wont be called while the plugin is processing
			status.bIsProcessing = true;
			if (status.bUseFPUE)
				csys->useFPUExceptions(true);
			long long start(0), stop(0);
			try
			{
				start = cpl::Misc::ClockCounter();
				csys->processReplacing(in.data(), out.data(), numSamples);
				stop = cpl::Misc::ClockCounter() - start;
			}
			catch (const cpl::CProtected::CSystemException & e)
			{
				if (status.bUseFPUE)
					csys->useFPUExceptions(false);

				switch (e.data.exceptCode) 
				{
				case cpl::CProtected::CSystemException::access_violation:
					// plugin code tried to access memory out of bounds - that is inside of our guarded code.
					if (e.data.aVInProtectedMemory) 
					{
						gui->console->printLine(CColours::red, "[Engine] : Plugin accessed memory out of bounds. "
							"Consider saving your project and restarting your application.");
					}
					else 
					{
						pluginCrashed();
					}
				default:
					/*
					Any exception that cannot be recovered will not be passed to this point; our program already crashed
					then. Therefore, we can ignore anything else that passes here (INT_DIVIDE_BY_ZERO for example)
					*/
					gui->console->printLine(CColours::red,
						"[Engine] : Exception 0x%X occured in plugin while processing: \"%s\". Plugin disabled.",
						e.data.exceptCode, cpl::CProtected::formatExceptionMessage(e).c_str());

				}
				// release lock of this, disablePlugin() needs to acquire it
				lockGuard.release();
				// release plugin code
				state = STATUS_ERROR;
				disablePlugin(false);
				gui->setStatusText("Plugin crashed!", CColours::red);
				// report crash to cstate
				csys->projectCrashed();
			}
			catch (const AbortException& e)
			{
				// release lock of this, disablePlugin() needs to acquire it
				lockGuard.release();
				disablePlugin(false);

				gui->setStatusText("Plugin aborted", CColours::orange);
				gui->console->printLine(CColours::red, "[Engine] : Plugin aborted while processing samples: %s.", e.what());
			};
			// if its still activated (ie. no error in previous code, we copy contents from plugin into 
			// buffer (this may seem weird to overwrite input with output, but thats how juce does it)
			if (status.bActivated)
			{
				clocksPerSample = static_cast<long>(stop / numSamples);
				if(!clocksPerSample)
					gui->console->printLine(CColours::red, "no counter? %d, %d, %d", stop, numSamples, cpl::Misc::ClockCounter());
				if (status.bUseFPUE)
					csys->useFPUExceptions(false);
				status.bIsProcessing = false;
				copyOutput(out, buffer);
			}
		}
	skip:
		// In case we have more outputs than inputs, we'll clear any output
		// channels that didn't contain input data, (because these aren't
		// guaranteed to be empty - they may contain garbage).
		for (int i = getNumInputChannels(); i < getNumOutputChannels(); ++i)
		{
			buffer.clear(i, 0, buffer.getNumSamples());
		}
	}
	/*********************************************************************************************

		ensures both safe buffers are large enough, 
		makes _in & _out valid buffers and copies inputs to in

	 *********************************************************************************************/
	bool inline Engine::copyInput(std::vector<float*> & in, std::vector<float*> & out, juce::AudioSampleBuffer & buffer)
	{
		// [0] is in, [1] is out
		unsigned size = buffer.getNumSamples();
		unsigned amountOfBuffers = buffer.getNumChannels();

		// reserve channels
		in.resize(amountOfBuffers);
		out.resize(amountOfBuffers);

		for (unsigned i = 0; i < amountOfBuffers; ++i) 
		{
			in[i] = csys->getPMemory()[0].get<float>() + size * i;
			out[i] = csys->getPMemory()[0].get<float>() + size * i;
			::memcpy(in[i], buffer.getSampleData(i), size * sizeof(float));
		}

		return true;
	}

	/*********************************************************************************************

		Copies the output from the plugin into the output provided by the host.

	 *********************************************************************************************/
	bool inline Engine::copyOutput(std::vector<float*> & out, juce::AudioSampleBuffer & buffer)
	{
		for(int i = 0; i < getNumOutputChannels(); ++i) {
			::memcpy(buffer.getSampleData(i), out[i], buffer.getNumSamples() * sizeof(float));
		}
		return true;
	}
	/*********************************************************************************************
	 
		Whether we have an editor
	 
	 *********************************************************************************************/
	bool Engine::hasEditor() const
	{
		return true; // (change this to false if you choose to not supply an editor)
	}
	/*********************************************************************************************
	 
		Creates an instance of our editor
	 
	 *********************************************************************************************/
	juce::AudioProcessorEditor* Engine::createEditor()
	{
		return gui->create();
	}

	/*********************************************************************************************
	 
		Serializes our plugin
	 
	 *********************************************************************************************/
	void Engine::getStateInformation(juce::MemoryBlock& destData)
	{
		CSerializer::serialize(this, destData);
	}
	/*********************************************************************************************
	 
		Restores our program from an earlier serialization
	 
	 *********************************************************************************************/
	void Engine::setStateInformation(const void* data, int sizeInBytes)
	{
		bool ret = false;
		try
		{
			ret = CSerializer::restore(this, data, sizeInBytes);
		}
		catch (std::exception & e)
		{

			gui->console->printLine(CColours::red, "[Engine] : Exception while serializing: %s", e.what());
		}
		if (!ret)
			gui->console->printLine(CColours::red, "[Engine] : Error serializing state!");
		else
			gui->console->printLine(CColours::black, "[Engine] : Succesfully serialized state!");
	}
	/*********************************************************************************************
	 
		Gets the name of the plugin
	 
	 *********************************************************************************************/
	const juce::String Engine::getName() const
	{
		#ifdef JucePlugin_Name
			return JucePlugin_Name;
		#else
			return _PROGRAM_NAME;
		#endif
	}
	/*********************************************************************************************
	 
		Returns number of parameters.
	 
	 *********************************************************************************************/
	int Engine::getNumParameters()
	{
		return 0;
	}
	/*********************************************************************************************
	 
		Gets a parameter from an index
	 
	 *********************************************************************************************/
	float Engine::getParameter(int index)
	{
		return 0.0f;
	}
	/*********************************************************************************************
		
		Sets a parameter from an index
	 
	 *********************************************************************************************/
	void Engine::setParameter(int index, float newValue)
	{
	}
	/*********************************************************************************************
	 
		Gets a parameter name
	 
	 *********************************************************************************************/
	const juce::String Engine::getParameterName(int index)
	{
		return juce::String::empty;
	}
	/*********************************************************************************************
	 
		Gets a parameters text
	 
	 *********************************************************************************************/
	const juce::String Engine::getParameterText(int index)
	{
		return juce::String::empty;
	}
	/*********************************************************************************************
	 
		Gets an inputchannelname
	 
	 *********************************************************************************************/
	const juce::String Engine::getInputChannelName(int channelIndex) const
	{
		return juce::String(channelIndex + 1);
	}
	/*********************************************************************************************
	 
		Gets an outputchannelname
	 
	 *********************************************************************************************/
	const juce::String Engine::getOutputChannelName(int channelIndex) const
	{
		return juce::String(channelIndex + 1);
	}
	/*********************************************************************************************
	 
		Is input channel a stereo pair? (yes)
	 
	 *********************************************************************************************/
	bool Engine::isInputChannelStereoPair(int index) const
	{
		return true;
	}
	/*********************************************************************************************
	 
		Is output channel a stereo pair? (yes)
	 
	 *********************************************************************************************/
	bool Engine::isOutputChannelStereoPair(int index) const
	{
		return true;
	}
	/*********************************************************************************************
	 
		Do we accept midi?
	 
	 *********************************************************************************************/
	bool Engine::acceptsMidi() const
	{
		#if JucePlugin_WantsMidiInput
			return true;
		#else
			return false;
		#endif
	}
	/*********************************************************************************************
	 
		Do we produce midi?
	 
	 *********************************************************************************************/
	bool Engine::producesMidi() const
	{
		#if JucePlugin_ProducesMidiOutput
			return true;
		#else
			return false;
		#endif
	}
	/*********************************************************************************************
	 
		Whether no input can produce an output
	 
	 *********************************************************************************************/
	bool Engine::silenceInProducesSilenceOut() const
	{
		return false;
	}
	/*********************************************************************************************
	 
		How long a decay we can produce
	 
	 *********************************************************************************************/
	double Engine::getTailLengthSeconds() const
	{
		return 0.0;
	}
	/*********************************************************************************************
		
		Number of programs
	 
	 *********************************************************************************************/
	int Engine::getNumPrograms()
	{
		return 1;
	}
	/*********************************************************************************************
	 
		Get index of current program
	 
	 *********************************************************************************************/
	int Engine::getCurrentProgram()
	{
		return 0;
	}
	/*********************************************************************************************
		
		Set index of current program
	 
	 *********************************************************************************************/
	void Engine::setCurrentProgram(int index)
	{
	}
	/*********************************************************************************************
	 
		Gets a program name
	 
	 *********************************************************************************************/
	const juce::String Engine::getProgramName(int index)
	{
		return juce::String::empty;
	}
	/*********************************************************************************************
	 
		Change a program name
	 
	 *********************************************************************************************/
	void Engine::changeProgramName(int index, const juce::String& newName)
	{
	}
	
	/*********************************************************************************************
	 
		Prepare to play, like resume()
		Here we change the initialDelay of the system
	 
	 *********************************************************************************************/
	void Engine::prepareToPlay(double sampleRate, int samplesPerBlock)
	{
		// Use this method as the place to do any pre-playback
		// initialisation that you need..
		if (delay.bDelayChanged)
		{
			gui->console->printLine(CColours::black, "initialDelay is changed to %d and reported to host.", delay.newDelay);
			if (delay.newDelay != delay.initialDelay)
			{
				this->setLatencySamples(delay.newDelay);
				delay.initialDelay = delay.newDelay;
				//ioChanged();
			}
			
			delay.bDelayChanged = false;
		}

		csys->setBounds(2, 2, samplesPerBlock);
	}
	/*********************************************************************************************
	 
		A chance to release some resources
	 
	 *********************************************************************************************/
	void Engine::releaseResources()
	{
		// When playback stops, you can use this as an opportunity to free up any
		// spare memory, etc.
	}

}
#ifdef __WINDOWS__
	static bool searchChanged = false;
#endif
/*********************************************************************************************

	This is our 'main'

*********************************************************************************************/
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
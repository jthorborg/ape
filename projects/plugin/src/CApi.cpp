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
 
	file:CApi.cpp
	
		Implementation of the API functions.

*************************************************************************************/

#include <cpl/Common.h>
#include "CApi.h"
#include "SharedInterfaceEx.h"
#include "UIController.h"
#include <cstdarg>
#include <string>
#include "CConsole.h"
#include "PluginState.h"
#include <cpl/Misc.h>
#include "Engine.h"
#include <cpl/Protected.h>
#include "Plugin/PluginCommandQueue.h"
#include "Plugin/PluginAudioFile.h"
#include "Plugin/PluginFFT.h"
#include "Plugin/PluginAudioWriter.h"

namespace ape::api
{
	using IEx = SharedInterfaceEx;

	thread_local int FaultLevel;

#define THROW(msg) \
	do { \
		if(FaultLevel) cpl::LogException("Double fault, level: " + std::to_string(FaultLevel)); \
		FaultLevel++; cpl::CProtected::instance().throwException<std::runtime_error>(std::string(__FUNCTION__) + ": " + (msg)); \
	} while(0)

	// TODO: Better exception type
#define APE_STRINGIFY(p) #p
#define REQUIRES_NOTNULL(param) \
	if(param == nullptr) \
		THROW(APE_STRINGIFY(param) " cannot be null");

#define REQUIRES_NOTZERO(param) \
	if(param == 0) \
			THROW(APE_STRINGIFY(param) " cannot be zero");

#define REQUIRES_TRUE(expression) \
	if(!(expression)) \
			THROW(APE_STRINGIFY(expression) " cannot be false");

	void clearThreadFaults()
	{
		FaultLevel = 0;
	}

	void APE_API abortPlugin(APE_SharedInterface * iface, const char * reason)
	{
		REQUIRES_NOTNULL(iface);
		cpl::CProtected::instance().throwException<PluginState::AbortException>(reason);
	}

	float APE_API getSampleRate(APE_SharedInterface * iface)
	{
		REQUIRES_NOTNULL(iface);
		auto& engine = IEx::downcast(*iface).getEngine();
		return static_cast<float>(engine.getSampleRate());
	}

	int APE_API_VARI printLine(APE_SharedInterface * iface, unsigned nColor, const char * fmt, ... ) 
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(fmt);

		auto& engine = IEx::downcast(*iface).getEngine();

		std::va_list args;
		va_start(args, fmt);
		std::string msg ("[Plugin] : ");
		msg += fmt;
		juce::Colour colour(nColor);
		int nRet = engine.getController().getConsole().printLine(colour.withAlpha(1.0f), msg.c_str(), args);

		va_end(args);

		return nRet;
	}

	int APE_API_VARI printThemedLine(APE_SharedInterface * iface, APE_TextColour color, const char * fmt, ...)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(fmt);

		auto& engine = IEx::downcast(*iface).getEngine();

		std::va_list args;
		va_start(args, fmt);
		std::string msg("[Plugin] : ");
		msg += fmt;
		int nRet = engine.getController().getConsole().printLine((APE_TextColour)color, msg.c_str(), args);

		va_end(args);

		return nRet;
	}

	int APE_API msgBox(APE_SharedInterface * iface, const char * text, const char * title, int nStyle, int nBlocking)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(text);
		REQUIRES_NOTNULL(title);

		auto& engine = IEx::downcast(*iface).getEngine();
		return cpl::Misc::MsgBox(text, title, nStyle, engine.getController().getSystemWindow(), nBlocking ? true : false);

	}

	int APE_API createMeter(APE_SharedInterface * iface, const char * name, const double* extVal, const double* peakVal)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(name);
		REQUIRES_NOTNULL(extVal);

		auto& pstate = IEx::downcast(*iface).getCurrentPluginState();
		auto queue = pstate.getCommandQueue();

		if (!queue)
			THROW("Cannot perform command at this point in time");

		return queue->enqueueCommand(MeterRecord(name, extVal, peakVal)).getClassCounter();
	}
	
	int APE_API_VARI createLabel(APE_SharedInterface * iface, const char * name, const char * fmt, ...)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(name);
		REQUIRES_NOTNULL(fmt);

		auto& pstate = IEx::downcast(*iface).getCurrentPluginState();
		auto queue = pstate.getCommandQueue();

		if (!queue)
			THROW("Cannot perform command at this point in time");

		va_list args;
		va_start(args, fmt);

		auto tag = queue->enqueueCommand(FormatLabelRecord(name, fmt, args)).getClassCounter();

		va_end(args);

		return tag;
	}

	long long APE_API timerGet(APE_SharedInterface * iface) 
	{
		REQUIRES_NOTNULL(iface);
		long long t = 0;

		#ifdef _WINDOWS_
			::QueryPerformanceCounter((LARGE_INTEGER*)&t);
		#elif defined(__MAC__)
			auto t1 = mach_absolute_time();
			*(decltype(t1)*)&t = t1;
		#elif defined(__CPP11__)
			using namespace std::chrono;
			auto tpoint = high_resolution_clock::now().time_since_epoch().count();
			(*(decltype(tpoint)*)&t) = tpoint;
		#endif

		return t;
	}

	double APE_API timerDiff(APE_SharedInterface * iface, long long time) 
	{
		REQUIRES_NOTNULL(iface);

		double ret = 0.f;

		#ifdef _WINDOWS_
			long long f;

			::QueryPerformanceFrequency((LARGE_INTEGER*)&f);

			long long t = timerGet(iface);
			ret = (t - time) * (1000.0/f);
		#elif defined(__MAC__)	
			auto t2 = mach_absolute_time();
			auto t1 = *(decltype(t2)*)&time;

			struct mach_timebase_info tinfo;
			if(mach_timebase_info(&tinfo) == KERN_SUCCESS)
			{
				double hTime2nsFactor = (double)tinfo.numer / tinfo.denom;
				ret = (((t2 - t1) * hTime2nsFactor) / 1000.0) / 1000.0;
			}
			
		#elif defined(__CPP11__)
			using namespace std::chrono;
		
			high_resolution_clock::rep t1, t2;
			auto now = timerGet(iface);
			t2 = *(high_resolution_clock::rep *)&now;
			t1 = *(high_resolution_clock::rep *)&time;
			milliseconds elapsed(now - time);
			ret = elapsed.count();
		#endif

		return ret;
	}

	void * APE_API alloc(APE_SharedInterface * iface, APE_AllocationLabel label, size_t size) 
	{
		REQUIRES_NOTNULL(iface);
		return IEx::downcast(*iface).getCurrentPluginState().getPluginAllocator().alloc(label, size);
	}

	void APE_API free(APE_SharedInterface * iface, void * ptr) 
	{
		REQUIRES_NOTNULL(iface);

		IEx::downcast(*iface).getCurrentPluginState().getPluginAllocator().free(ptr);
	}

	void APE_API setInitialDelay(APE_SharedInterface * iface, int samples) 
	{
		REQUIRES_NOTNULL(iface);
		auto& engine = IEx::downcast(*iface).getEngine();

		engine.changeInitialDelay(samples);
	}

	int APE_API getNumInputs(APE_SharedInterface * iface) 
	{
		REQUIRES_NOTNULL(iface);
		auto& engine = IEx::downcast(*iface).getEngine();

		return engine.getNumInputChannels();
	}

	int APE_API getNumOutputs(APE_SharedInterface * iface) 
	{
		REQUIRES_NOTNULL(iface);
		auto& engine = IEx::downcast(*iface).getEngine();

		return engine.getNumOutputChannels();
	}
	
	double APE_API getBPM(APE_SharedInterface * iface)
	{
		REQUIRES_NOTNULL(iface);
		auto& engine = IEx::downcast(*iface).getEngine();
		auto& pstate = IEx::downcast(*iface).getCurrentPluginState();

		if (!pstate.isProcessing())
			THROW("Can only be called from a processing callback");

		double ret = 0.0;
		#ifdef APE_VST
			VstTimeInfo * info;
	
			info = engine.getTimeInfo(kVstTempoValid);
	
			if(info && info->flags & kVstTempoValid)
				return info->tempo;
		#elif defined(APE_JUCE)

			auto ph = engine.getPlayHead();
			if(ph)
			{
				juce::AudioPlayHead::CurrentPositionInfo cpi;
				ph->getCurrentPosition(cpi);
				ret = cpi.bpm;			
			}

		#endif
		return ret;
	}
	
	int	APE_API	createPlot(APE_SharedInterface * iface, const char * name, 
		const double * const vals, const unsigned int numVals)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(name);
		REQUIRES_NOTNULL(vals);

		auto& pstate = IEx::downcast(*iface).getCurrentPluginState();
		auto queue = pstate.getCommandQueue();

		if (!queue)
			THROW("Cannot perform command at this point in time");

		return queue->enqueueCommand(PlotRecord(name, numVals, vals)).getClassCounter();
	}

	int	APE_API	presentTrace(APE_SharedInterface* iface, const char** nameTuple, size_t numNames, const float* const values, size_t numValues)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(nameTuple);
		REQUIRES_NOTNULL(values);
		REQUIRES_NOTZERO(numNames);
		REQUIRES_NOTZERO(numValues);

		auto& pstate = IEx::downcast(*iface).getCurrentPluginState();
		auto& engine = IEx::downcast(*iface).getEngine();

		if (!pstate.isProcessing())
			THROW("Can only be called from a processing callback");

		engine.handleTraceCallback(nameTuple, numNames, values, numValues);

		return 1;
	}

	int	APE_API	createNormalParameter(APE_SharedInterface * iface, const char * name, const char * unit, APE_Parameter* extVal, Transformer transformer, Normalizer normalizer, PFloat min, PFloat max)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(name);
		REQUIRES_NOTNULL(unit);
		REQUIRES_NOTNULL(extVal);

		auto& pstate = IEx::downcast(*iface).getCurrentPluginState();
		auto queue = pstate.getCommandQueue();

		if (!queue)
			THROW("Cannot perform command at this point in time");

		return queue->enqueueCommand(ParameterRecord::NormalParameter(name, unit, extVal, transformer, normalizer, min, max)).getClassCounter();
	}

	int APE_API createBooleanParameter(APE_SharedInterface * iface, const char * name, APE_Parameter* extVal)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(name);
		REQUIRES_NOTNULL(extVal);

		auto& pstate = IEx::downcast(*iface).getCurrentPluginState();
		auto queue = pstate.getCommandQueue();

		if(!queue)
			THROW("Cannot perform command at this point in time");

		return queue->enqueueCommand(ParameterRecord::BoolFlag(name, extVal)).getClassCounter();
	}

	int APE_API createListParameter(APE_SharedInterface * iface, const char * name, APE_Parameter* extVal, int numNames, const char* const* names)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(name);
		REQUIRES_NOTNULL(extVal);
		REQUIRES_NOTZERO(numNames);
		REQUIRES_NOTNULL(names);

		for (int i = 0; i < numNames; ++i)
			REQUIRES_NOTNULL(names[i]);

		auto& pstate = IEx::downcast(*iface).getCurrentPluginState();
		auto queue = pstate.getCommandQueue();

		if (!queue)
			THROW("Cannot perform command at this point in time");

		return queue->enqueueCommand(ParameterRecord::ValueList(name, extVal, numNames, names)).getClassCounter();
	}

	int	APE_API	destroyResource(APE_SharedInterface * iface, int resource, int reserved)
	{
		// Destruction allowed while unwinding a plugin.
		if (FaultLevel > 0)
			return 0;

		REQUIRES_NOTNULL(iface);
		auto& pstate = IEx::downcast(*iface).getCurrentPluginState();
		if (!pstate.isDisabling())
			THROW("Cannot release resources at this point in time!");

		return 0;
	}

	int APE_API loadAudioFile(APE_SharedInterface * iface, const char * path, double sampleRate, APE_AudioFile * result)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(path);
		REQUIRES_NOTNULL(result);

		auto& shared = IEx::downcast(*iface);
		auto& pstate = shared.getCurrentPluginState();
		auto& console = shared.getEngine().getController().getConsole();

		*result = {};

		try
		{
			const auto& project = pstate.getProject();
            juce::File workingDirectory = juce::String(project.workingDirectory);

			if (!workingDirectory.exists())
			{
				console.printLine(CConsole::Error, "[Plugin] : Error loading audio file, working directory doesn't exist: %s", workingDirectory.getFullPathName().toStdString().c_str());
				return 0;
			}

			auto candidate = workingDirectory.getChildFile(path);

			if (!candidate.existsAsFile())
			{
				console.printLine(CConsole::Error, "[Plugin] : Error loading audio file %s, canonical path doesn't exist: %s", path, candidate.getFullPathName().toStdString().c_str());
				return 0;
			}

			const auto currentSampleRate = shared.getEngine().getSampleRate();

			auto originalFiles = pstate.getOriginalFiles();
			auto originalFile = originalFiles.find(path);
			PluginAudioFile* source = nullptr;

			if (originalFile == originalFiles.end())
			{
				auto newFile = std::make_unique<PluginAudioFile>(candidate);
				source = pstate.getPluginAudioFiles().emplace_back(std::move(newFile)).get();
				originalFiles[path] = source;
			}
			else
			{
				source = originalFile->second;
			}

			// If the adopted sample rate is 0, just return the original.
			if (sampleRate == APE_SampleRate_Retain || sampleRate == source->getAudioFile().sampleRate)
			{
				*result = source->getAudioFile();
			}
			else if (sampleRate == APE_SampleRate_Adopt)
			{
				if (currentSampleRate == 0 || currentSampleRate == source->getAudioFile().sampleRate)
				{
					*result = source->getAudioFile();
					return 1;
				}
				else
				{
					auto newFile = std::make_unique<PluginAudioFile>(*source, currentSampleRate);
					*result = pstate.getPluginAudioFiles().emplace_back(std::move(newFile))->getAudioFile();
				}
			}
			else
			{
				auto newFile = std::make_unique<PluginAudioFile>(*source, sampleRate);
				*result = pstate.getPluginAudioFiles().emplace_back(std::move(newFile))->getAudioFile();
			}

			return 1;
		}
		catch(const std::exception& e)
		{
			console.printLine(CConsole::Error, "[Plugin] : Error loading audio file %s: %s", path, e.what());
			return 0;
		}
	}

	APE_FFT *APE_API createFFT(APE_SharedInterface * iface, APE_DataType type, size_t size)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_TRUE(type == APE_DataType_Single || type == APE_DataType_Double);
		REQUIRES_TRUE(size > 0);
		REQUIRES_TRUE((size & (size - 1)) == 0);

		auto& shared = IEx::downcast(*iface);
		auto& pstate = shared.getCurrentPluginState();

		return pstate.getPluginFFTs().emplace_back(APE_FFT::factory(size, type)).get();
	}

	void APE_API performFFT(APE_SharedInterface * iface, APE_FFT * fft, APE_FFT_Options options, const void * in, void * out)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(in);
		REQUIRES_NOTNULL(out);
		REQUIRES_NOTNULL(fft);

		return fft->transform(in, out, options);
	}

	void APE_API releaseFFT(APE_SharedInterface * iface, APE_FFT * fft)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(fft);

		auto& ffts = IEx::downcast(*iface).getCurrentPluginState().getPluginFFTs();

		bool done = false;

		for (std::size_t i = 0; i < ffts.size(); ++i)
		{
			if (fft == ffts[i].get())
			{
				ffts.erase(ffts.begin() + i);
				done = true;
				break;
			}
		}

		if(!done)
			THROW("Request to release non-owned FFT");
	}
	
	void APE_API setTriggeringChannel(APE_SharedInterface * iface, int triggerChannel)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_TRUE(triggerChannel > 0);
		
		auto& shared = IEx::downcast(*iface);
		
		shared.getCurrentPluginState().apiTriggerOverride();

		shared
			.getEngine()
			.getOscilloscopeData()
			.setTriggeringChannel(triggerChannel);
	}

	int APE_API createAudioOutputFile(APE_SharedInterface * iface, const char * relativePath, double sampleRate, int channels, int bits, float quality)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(relativePath);
		REQUIRES_TRUE(relativePath[0] != '\0');
		REQUIRES_TRUE(sampleRate > 0);
		REQUIRES_TRUE(channels > 0);
		REQUIRES_TRUE(quality >= 0 && quality <= 1);

		auto& shared = IEx::downcast(*iface);
		auto& engine = shared.getEngine();
		auto& pstate = shared.getCurrentPluginState();
		auto& console = engine.getController().getConsole();

		try
		{
			const auto& project = pstate.getProject();
			juce::File workingDirectory = juce::String(project.workingDirectory);

			if (!workingDirectory.exists())
			{
				console.printLine(CConsole::Error, "[Plugin] : Error creating audio file, working directory doesn't exist: %s", workingDirectory.getFullPathName().toStdString().c_str());
				return 0;
			}

			auto originalPath = workingDirectory.getChildFile(relativePath);
			auto path = originalPath;
			int uniqueCounter = 0;

			for (int i = 0; path.exists() && i < 100; ++i)
			{
				path = path.getParentDirectory().getChildFile(originalPath.getFileNameWithoutExtension() + std::to_string(i + 1) + path.getFileExtension());
			}

			if (path.exists())
			{
				console.printLine(CConsole::Error, "[Plugin] : Failed to create a unique name for audio file: %s", path.getFullPathName().toStdString().c_str());
				return 0;
			}

			auto format = OutputFileManager::selectFormatFor(path);

			if (!format)
			{
				console.printLine(CConsole::Error, "[Plugin] : Cannot understand audio format for : %s", path.getFileExtension().toStdString().c_str());
				return 0;
			}

			auto extension = path.getFileExtension();

			auto qualities = format->getQualityOptions();
			int qualityIndex = 0;
			bool hasQualities = qualities.size() > 0;

			if (qualities.size() > 0)
			{
				qualityIndex = cpl::Math::round<int>(qualities.size() * quality);
			}

			auto producer = OutputFileManager::createProducer(path, *format, sampleRate, channels, bits, qualityIndex);

			console.printLine(
				"[Plugin] : Streaming to audio file %s in %s @ %d channels %fHz %d bit %s quality",
				path.getFullPathName().toStdString().c_str(),
				format->getFormatName().toStdString().c_str(),
				channels,
				sampleRate,
				bits,
				hasQualities ? qualities[qualityIndex] : "default"
			);

			pstate.getOutputFiles().emplace_back(std::move(producer));

			return (int)(pstate.getOutputFiles().size() - 1);
		}
		catch (const std::exception& e)
		{
			console.printLine(CConsole::Error, "[Plugin] : Failed to create streamed output audio file: %s\n%s", relativePath, e.what());
		}

		return 0;
	}

	void APE_API writeAudioFile(APE_SharedInterface * iface, int file, unsigned int numSamples, const float * const * data)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_TRUE(file != 0);
		REQUIRES_NOTNULL(data);
		REQUIRES_TRUE(numSamples != 0);

		auto& shared = IEx::downcast(*iface);
		auto& stream = shared.getCurrentPluginState().getOutputFiles().at(file);

		if (!stream)
		{
			THROW("Writing to released audio output file");
		}

		if (!stream->writeAsync(numSamples, data))
			shared.getEngine().getController().getConsole().printLine("[Plugin] Buffer overrun writing to fd %d (produced %d)", file, (int)numSamples);
	}

	void APE_API closeAudioFile(APE_SharedInterface * iface, int file)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_TRUE(file != 0);

		IEx::downcast(*iface)
			.getCurrentPluginState()
			.getOutputFiles()
			.at(file)
			.reset(nullptr);
	}


}

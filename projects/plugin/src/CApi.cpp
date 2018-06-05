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

namespace ape 
{
	using IEx = SharedInterfaceEx;

#define THROW(msg) \
	cpl::CProtected::instance().throwException<std::runtime_error>(std::string(__FUNCTION__) + ": " + (msg))

// TODO: Better exception type
#define APE_STRINGIFY(p) #p
#define REQUIRES_NOTNULL(param) \
	if(param == nullptr) \
			THROW(APE_STRINGIFY(param) " cannot be null");

#define REQUIRES_NOTZERO(param) \
	if(param == 0) \
			THROW(APE_STRINGIFY(param) " cannot be zero");

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

	Status APE_API setStatus(APE_SharedInterface * iface, Status status) 
	{
		abortPlugin(iface, "setStatus is a deprecated API.");

		return Status::STATUS_ERROR;
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
		int nRet = engine.getController().console().printLine(colour.withAlpha(1.0f), msg.c_str(), args);

		va_end(args);

		return nRet;
	}

	int APE_API_VARI createLabel(APE_SharedInterface * iface, const char * name, const char * fmt, ...) 
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(name);
		REQUIRES_NOTNULL(fmt);

		va_list args;
		va_start(args, fmt);
		int tag = IEx::downcast(*iface).getCurrentPluginState().getCtrlManager().addLabel(name, fmt, args);

		va_end(args);
		if(tag == -1)
			IEx::downcast(*iface).getEngine().getController().console().printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}

	int APE_API msgBox(APE_SharedInterface * iface, const char * text, const char * title, int nStyle, int nBlocking) 
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(text);
		REQUIRES_NOTNULL(title);

		auto& engine = IEx::downcast(*iface).getEngine();
		return cpl::Misc::MsgBox(text, title, nStyle, engine.getController().getSystemWindow(), nBlocking ? true : false);

	}

	int APE_API createKnob(APE_SharedInterface * iface, const char * name, float * extVal, int type) 
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(name);
		REQUIRES_NOTNULL(extVal);

		auto& engine = IEx::downcast(*iface).getEngine();
		int tag = IEx::downcast(*iface).getCurrentPluginState().getCtrlManager().addKnob(name, extVal, static_cast<CKnobEx::type>(type));
		if(tag == -1)
			engine.getController().console().printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}

	int APE_API createKnobEx(APE_SharedInterface * iface, const char * name, float * extVal, char * values, char * unit) 
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(name);
		REQUIRES_NOTNULL(extVal);
		REQUIRES_NOTNULL(values);
		REQUIRES_NOTNULL(unit);

		auto& engine = IEx::downcast(*iface).getEngine();

		int tag = IEx::downcast(*iface).getCurrentPluginState().getCtrlManager().addKnob(name, extVal, values, unit);
		if(tag == -1)
			engine.getController().console().printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}

	int APE_API createMeter(APE_SharedInterface * iface, const char * name, float * extVal)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(name);
		REQUIRES_NOTNULL(extVal);

		auto& engine = IEx::downcast(*iface).getEngine();

		int tag = IEx::downcast(*iface).getCurrentPluginState().getCtrlManager().addMeter(name, extVal);
		if(tag == -1)
			engine.getController().console().printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}
	
	int APE_API createToggle(APE_SharedInterface * iface, const char * name, float * extVal)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(name);
		REQUIRES_NOTNULL(extVal);

		auto& engine = IEx::downcast(*iface).getEngine();

		int tag = IEx::downcast(*iface).getCurrentPluginState().getCtrlManager().addToggle(name, extVal);
		if(tag == -1)
			engine.getController().console().printLine(juce::Colours::red, "No more space for controls!");

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

		if (!IEx::downcast(*iface).getCurrentPluginState().isProcessing())
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
	
	void APE_API setCtrlValue(APE_SharedInterface * iface, int ID, float value)
	{
		REQUIRES_NOTNULL(iface);

		CBaseControl * c = IEx::downcast(*iface).getCurrentPluginState().getCtrlManager().getControl(ID);
		if(c)
			c->bSetValue(value);
		else
			THROW("No such control: " + std::to_string(ID));
	}
	
	float APE_API getCtrlValue(APE_SharedInterface * iface, int ID)
	{
		REQUIRES_NOTNULL(iface);

		CBaseControl * c = IEx::downcast(*iface).getCurrentPluginState().getCtrlManager().getControl(ID);
		if(c)
			return c->bGetValue();
		else  
			THROW("No such control: " + std::to_string(ID));
	}

	int	APE_API	createPlot(APE_SharedInterface * iface, const char * name, 
		const float * const vals, const unsigned int numVals)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(name);
		REQUIRES_NOTNULL(vals);

		auto& engine = IEx::downcast(*iface).getEngine();

		int tag = IEx::downcast(*iface).getCurrentPluginState().getCtrlManager().addPlot(name, vals, numVals);
		if (tag == -1)
			engine.getController().console().printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}
	
	int	APE_API	createRangeKnob(APE_SharedInterface * iface, const char * name, const char * unit, float * extVal, ScaleFunc scaleCB, float min, float max)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(name);
		REQUIRES_NOTNULL(unit);
		REQUIRES_NOTNULL(extVal);
		REQUIRES_NOTNULL(scaleCB);

		auto& engine = IEx::downcast(*iface).getEngine();

		int tag = IEx::downcast(*iface).getCurrentPluginState().getCtrlManager().addKnob(name, unit, extVal, scaleCB, min, max);
		if (tag == -1)
			engine.getController().console().printLine(juce::Colours::red, "No more space for controls!");

		return tag;

	}

	int	APE_API	presentTrace(APE_SharedInterface* iface, const char** nameTuple, size_t numNames, const float* const values, size_t numValues)
	{
		REQUIRES_NOTNULL(iface);
		REQUIRES_NOTNULL(nameTuple);
		REQUIRES_NOTNULL(values);
		REQUIRES_NOTZERO(numNames);
		REQUIRES_NOTZERO(numValues);

		auto& engine = IEx::downcast(*iface).getEngine();

		if (!engine.getCurrentPluginState()->isProcessing())
			THROW("Can only be called from a processing callback");

		engine.handleTraceCallback(nameTuple, numNames, values, numValues);

		return 1;
	}

}
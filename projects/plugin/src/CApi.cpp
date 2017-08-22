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

#include "MacroConstants.h"
#include "CApi.h"
#include "SharedInterfaceEx.h"
#include "GraphicUI.h"
#include <cstdarg>
#include <string>
#include "CConsole.h"
#include "CState.h"
#include "Misc.h"
#include "Engine.h"

namespace APE 
{
	using IEx = SharedInterfaceEx;

	#define CAPI_SANITY_CHECK() \
		if(!iface) \
			throw CState::CSystemException(CState::CSystemException::status::nullptr_from_plugin, true);

	float APE_API getSampleRate(APE_SharedInterface * iface)
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();
		return static_cast<float>(engine.getSampleRate());
	}

	Status APE_API setStatus(APE_SharedInterface * iface, Status status) 
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();
		return engine.requestStatusChange(status);
	}

	int APE_API_VARI printLine(APE_SharedInterface * iface, unsigned nColor, const char * fmt, ... ) 
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();

		std::va_list args;
		va_start(args, fmt);
		std::string msg ("[Plugin] : ");
		msg += fmt;
		CColour	color(_rgb_get_red(nColor), _rgb_get_green(nColor), _rgb_get_blue(nColor), (unsigned char)0xFF);
#pragma message("wtf")
		int nRet = engine.getGraphicUI()->console->printLine(color, msg.c_str(), args);

		va_end(args);

		return nRet;
	}

	int APE_API_VARI createLabel(APE_SharedInterface * iface, const char * name, const char * fmt, ...) 
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();

		va_list args;
		va_start(args, fmt);
		int tag = engine.getGraphicUI()->ctrlManager.addLabel(name, fmt, args);

		va_end(args);
		if(tag == -1)
			engine.getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}

	int APE_API msgBox(APE_SharedInterface * iface, const char * text, const char * title, int nStyle, int nBlocking) 
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();
		return Misc::MsgBox(text, title, nStyle, engine.getGraphicUI()->getSystemWindow(), nBlocking ? true : false);

	}

	int APE_API createKnob(APE_SharedInterface * iface, const char * name, float * extVal, int type) 
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();
		int tag = engine.getGraphicUI()->ctrlManager.addKnob(name, extVal, static_cast<CKnobEx::type>(type));
		if(tag == -1)
			engine.getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}

	int APE_API createKnobEx(APE_SharedInterface * iface, const char * name, float * extVal, char * values, char * unit) 
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();

		int tag = engine.getGraphicUI()->ctrlManager.addKnob(name, extVal, values, unit);
		if(tag == -1)
			engine.getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}

	int APE_API createMeter(APE_SharedInterface * iface, const char * name, float * extVal)
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();

		int tag = engine.getGraphicUI()->ctrlManager.addMeter(name, extVal);
		if(tag == -1)
			engine.getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}
	
	int APE_API createToggle(APE_SharedInterface * iface, const char * name, float * extVal)
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();

		int tag = engine.getGraphicUI()->ctrlManager.addToggle(name, extVal);
		if(tag == -1)
			engine.getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}

	long long APE_API timerGet(APE_SharedInterface * iface) 
	{
		CAPI_SANITY_CHECK();
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
		CAPI_SANITY_CHECK();

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

	void * APE_API alloc(APE_SharedInterface * iface, size_t size) 
	{
		CAPI_SANITY_CHECK();
		return IEx::upcast(*iface).getCState().getPluginAllocator().alloc(size);
	}

	void APE_API free(APE_SharedInterface * iface, void * ptr) 
	{
		CAPI_SANITY_CHECK();

		IEx::upcast(*iface).getCState().getPluginAllocator().free(ptr);
	}

	void APE_API setInitialDelay(APE_SharedInterface * iface, int samples) 
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();

		engine.changeInitialDelay(samples);
	}

	int APE_API getNumInputs(APE_SharedInterface * iface) 
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();

		return engine.getNumInputChannels();
	}

	int APE_API getNumOutputs(APE_SharedInterface * iface) 
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();

		return engine.getNumOutputChannels();
	}
	
	double APE_API getBPM(APE_SharedInterface * iface)
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();

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
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();

		CBaseControl * c = engine.getGraphicUI()->ctrlManager.getControl(ID);
		if(c)
			c->bSetValue(value);
	}
	
	float APE_API getCtrlValue(APE_SharedInterface * iface, int ID)
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();

		CBaseControl * c = engine.getGraphicUI()->ctrlManager.getControl(ID);
		if(c)
			return c->bGetValue();
		else 
			return 0.f;
	}

	int	APE_API	createPlot(APE_SharedInterface * iface, const char * name, 
		const float * const vals, const unsigned int numVals)
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();

		int tag = engine.getGraphicUI()->ctrlManager.addPlot(name, vals, numVals);
		if (tag == -1)
			engine.getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}
	
	int	APE_API	createRangeKnob(APE_SharedInterface * iface, const char * name, const char * unit, float * extVal, ScaleFunc scaleCB, float min, float max)
	{
		CAPI_SANITY_CHECK();
		auto& engine = IEx::upcast(*iface).getEngine();

		int tag = engine.getGraphicUI()->ctrlManager.addKnob(name, unit, extVal, scaleCB, min, max);
		if (tag == -1)
			engine.getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;

	}
}
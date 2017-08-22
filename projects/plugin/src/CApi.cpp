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
#include <ape/SharedInterface.h>
#include "GraphicUI.h"
#include <cstdarg>
#include <string>
#include "CConsole.h"
#include "CState.h"
#include "Misc.h"
#include "Engine.h"

namespace APE 
{
	#define CAPI_SANITY_CHECK() \
		if(!iface || !iface->engine) \
			throw CState::CSystemException(CState::CSystemException::status::nullptr_from_plugin, true);
	/*********************************************************************************************

	 	Returns sample rate of the current instance.

	 *********************************************************************************************/
	float APE_API getSampleRate(CSharedInterface * iface)
	{
		CAPI_SANITY_CHECK();
		return static_cast<float>(iface->engine->getSampleRate());
	}

	/*********************************************************************************************

		The c-subsystem can here request a change to it's status, it may or may not be 
		accepted, return value is always the (possible changed) state of APE.

	 *********************************************************************************************/
	Status APE_API setStatus(CSharedInterface * iface, Status status) 
	{
		CAPI_SANITY_CHECK();
		return iface->engine->requestStatusChange(status);
	}

	/*********************************************************************************************

		Prints a line with the color nColor in the console in the APE window associated with 
		the C script.

	 *********************************************************************************************/
	int APE_API_VARI printLine(CSharedInterface * iface, unsigned nColor, const char * fmt, ... ) 
	{
		CAPI_SANITY_CHECK();

		std::va_list args;
		va_start(args, fmt);
		std::string msg ("[Plugin] : ");
		msg += fmt;
		CColour	color(_rgb_get_red(nColor), _rgb_get_green(nColor), _rgb_get_blue(nColor), (unsigned char)0xFF);

		int nRet = iface->engine->getGraphicUI()->console->printLine(color, msg.c_str(), args);

		va_end(args);

		return nRet;
	}

	/*********************************************************************************************

		Creates a label according to the format string, that depends on the reference arguments.

	 *********************************************************************************************/
	int APE_API_VARI createLabel(CSharedInterface * iface, const char * name, const char * fmt, ...) 
	{
		CAPI_SANITY_CHECK();

		va_list args;
		va_start(args, fmt);

		int tag = iface->engine->getGraphicUI()->ctrlManager.addLabel(name, fmt, args);

		va_end(args);
		if(tag == -1)
			iface->engine->getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}

	/*********************************************************************************************

		presents a messagebox for the user, that may or may not be blocking

	 *********************************************************************************************/
	int APE_API msgBox(CSharedInterface * iface, const char * text, const char * title, int nStyle, int nBlocking) 
	{
		CAPI_SANITY_CHECK();

		return Misc::MsgBox(text, title, nStyle, iface->engine->getGraphicUI()->getSystemWindow(), nBlocking ? true : false);

	}

	/*********************************************************************************************

		Adds a automatable parameter to the GUI.

	 *********************************************************************************************/
	int APE_API createKnob(CSharedInterface * iface, const char * name, float * extVal, int type) 
	{
		CAPI_SANITY_CHECK();

		int tag = iface->engine->getGraphicUI()->ctrlManager.addKnob(name, extVal, static_cast<CKnobEx::type>(type));
		if(tag == -1)
			iface->engine->getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}

	/*********************************************************************************************

		Adds an automatable parameter to the GUI, using a list of values.

	 *********************************************************************************************/
	int APE_API createKnobEx(CSharedInterface * iface, const char * name, float * extVal, char * values, char * unit) 
	{
		CAPI_SANITY_CHECK();

		int tag = iface->engine->getGraphicUI()->ctrlManager.addKnob(name, extVal, values, unit);
		if(tag == -1)
			iface->engine->getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}
	/*********************************************************************************************

		Adds an automatable parameter to the GUI, using a list of values.

	 *********************************************************************************************/	
	int APE_API createMeter(CSharedInterface * iface, const char * name, float * extVal)
	{
		CAPI_SANITY_CHECK();

		int tag = iface->engine->getGraphicUI()->ctrlManager.addMeter(name, extVal);
		if(tag == -1)
			iface->engine->getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}
	/*********************************************************************************************

		Adds a button to the GUI.

	 *********************************************************************************************/	
	int APE_API createToggle(CSharedInterface * iface, const char * name, float * extVal)
	{
		CAPI_SANITY_CHECK();

		int tag = iface->engine->getGraphicUI()->ctrlManager.addToggle(name, extVal);
		if(tag == -1)
			iface->engine->getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}
	/*********************************************************************************************

		Returns an opaque handle to a starting point using the system's high-resolution clock.

	 *********************************************************************************************/
	long long APE_API timerGet(CSharedInterface * iface) 
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

	/*********************************************************************************************

		Calculates the difference from a previous call to timerGet.

	 *********************************************************************************************/
	double APE_API timerDiff(CSharedInterface * iface, long long time) 
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

	/*********************************************************************************************

		The memory allocation routine used by the hosted code. It's wrapped here so we can change
		the routine at will. At some point, register all allocations in a list for free'ing at exit.

	 *********************************************************************************************/
	void * APE_API alloc(CSharedInterface * iface, size_t size) 
	{
		CAPI_SANITY_CHECK();
		return iface->csys->getPluginAllocator().alloc(size);
	}

	/*********************************************************************************************

		Frees a pointer to an earlier call to alloc. Do not mix new/delete/malloc/free with these 
		functions!!

	 *********************************************************************************************/
	void APE_API free(CSharedInterface * iface, void * ptr) {

		CAPI_SANITY_CHECK();

		iface->csys->getPluginAllocator().free(ptr);
	}

	/*********************************************************************************************

		Requests the host to change the initial delay imposed by the module on next resume() call.

	 *********************************************************************************************/
	void APE_API setInitialDelay(CSharedInterface * iface, VstInt32 samples) {

		CAPI_SANITY_CHECK();

		iface->engine->changeInitialDelay(samples);
	}

	/*********************************************************************************************

		Returns the number of inputs associated with this plugin.

	 *********************************************************************************************/
	int APE_API getNumInputs(CSharedInterface * iface) {

		CAPI_SANITY_CHECK();

		return iface->engine->getNumInputChannels();
	}

	/*********************************************************************************************

		Returns the number of outputs associated with this plugin.

	 *********************************************************************************************/
	int APE_API getNumOutputs(CSharedInterface * iface) {

		CAPI_SANITY_CHECK();

		return iface->engine->getNumOutputChannels();
	}
	/*********************************************************************************************

		Returns the host's BPM

	 *********************************************************************************************/
	double APE_API getBPM(CSharedInterface * iface)
	{
		CAPI_SANITY_CHECK();

		double ret = 0.0;
		#ifdef APE_VST
			VstTimeInfo * info;
	
			info = iface->engine->getTimeInfo(kVstTempoValid);
	
			if(info && info->flags & kVstTempoValid)
				return info->tempo;
		#elif defined(APE_JUCE)

			auto ph = iface->engine->getPlayHead();
			if(ph)
			{
				juce::AudioPlayHead::CurrentPositionInfo cpi;
				ph->getCurrentPosition(cpi);
				ret = cpi.bpm;			
			}

		#endif
		return ret;
	}
	/*********************************************************************************************

		Set the value of a control with the given ID

	 *********************************************************************************************/
	void APE_API setCtrlValue(CSharedInterface * iface, int ID, float value)
	{
		CAPI_SANITY_CHECK();
		CBaseControl * c = iface->engine->getGraphicUI()->ctrlManager.getControl(ID);
		if(c)
			c->bSetValue(value);
	}
	/*********************************************************************************************

		Gets the value of a control with the given ID

	 *********************************************************************************************/
	float APE_API getCtrlValue(CSharedInterface * iface, int ID)
	{
		CAPI_SANITY_CHECK();
		CBaseControl * c = iface->engine->getGraphicUI()->ctrlManager.getControl(ID);
		if(c)
			return c->bGetValue();
		else 
			return 0.f;
	}
	/*********************************************************************************************

		Adds a plot to the GUI.

	*********************************************************************************************/
	int	APE_API	createPlot(CSharedInterface * iface, const char * name, 
		const float * const vals, const unsigned int numVals)
	{
		CAPI_SANITY_CHECK();

		int tag = iface->engine->getGraphicUI()->ctrlManager.addPlot(name, vals, numVals);
		if (tag == -1)
			iface->engine->getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;
	}
	/*********************************************************************************************

		Adds a ranged knob

	*********************************************************************************************/
	int	APE_API	createRangeKnob(CSharedInterface * iface, const char * name, const char * unit, float * extVal, ScaleFunc scaleCB, float min, float max)
	{
		CAPI_SANITY_CHECK();

		int tag = iface->engine->getGraphicUI()->ctrlManager.addKnob(name, unit, extVal, scaleCB, min, max);
		if (tag == -1)
			iface->engine->getGraphicUI()->console->printLine(juce::Colours::red, "No more space for controls!");

		return tag;

	}
}; // namespace APE
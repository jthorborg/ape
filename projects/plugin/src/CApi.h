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

	file:CApi.h

		The API the C program can utilize, definitions for exported functions/symbols.

*************************************************************************************/

#ifndef APE_CAPI_H
	#define APE_CAPI_H

	#include <cpl/MacroConstants.h>
	#include <exception>
	#include <ape/SharedInterface.h>

	namespace ape
	{
		/// <summary>
		/// Throws an abort exception, picked up by the engine and disables the plugin.
		/// </summary>
		void		APE_API			abortPlugin(APE_SharedInterface * iface, const char * reason);
		/// <summary>
		/// Returns sample rate of the current instance.
		/// </summary>
		float		APE_API			getSampleRate(APE_SharedInterface * iface);
		/// <summary>
		/// Prints a line with the color nColor in the console in the ape window associated with 
		/// the C script.
		/// </summary>
		int			APE_API_VARI	printLine(APE_SharedInterface * iface, unsigned nColor, const char * fmt, ... );
		/// <summary>
		/// presents a messagebox for the user, that may or may not be blocking
		/// </summary>
		int			APE_API			msgBox(APE_SharedInterface * iface, const char * text, const char * title, int nStyle, int nBlocking);
		/// <summary>
		/// The c-subsystem can here request a change to it's status, it may or may not be 
		/// accepted, return value is always the(possible changed) state of ape.
		/// </summary>
		Status		APE_API			setStatus(APE_SharedInterface * iface, Status status);
		/// <summary>
		/// Adds a automatable parameter to the GUI.
		/// </summary>
		int			APE_API			createKnob(APE_SharedInterface * iface, const char * name, float * extVal, int type);
		/// <summary>
		/// Returns an opaque handle to a starting point using the system's high-resolution clock.
		/// </summary>
		long long	APE_API			timerGet(APE_SharedInterface * iface);
		/// <summary>
		/// Calculates the difference from a previous call to timerGet.
		/// </summary>
		double		APE_API			timerDiff(APE_SharedInterface * iface, long long time);
		/// <summary>
		/// The memory allocation routine used by the hosted code. It's wrapped here so we can change
		/// the routine at will.At some point, register all allocations in a list for free'ing at exit.
		/// </summary>
		void *		APE_API			alloc(APE_SharedInterface * iface, APE_AllocationLabel label, size_t size);
		/// <summary>
		/// Frees a pointer to an earlier call to alloc. Do not mix new/delete/malloc/free with these 
		/// functions!!
		/// </summary>
		void		APE_API			free(APE_SharedInterface * iface, void * ptr);
		/// <summary>
		/// Adds an automatable parameter to the GUI, using a list of values.
		/// </summary>
		int			APE_API			createKnobEx(APE_SharedInterface * iface, const char * name, float * extVal, char * values, char * unit);
		/// <summary>
		/// Requests the host to change the initial delay imposed by the module on next resume() call.
		/// </summary>
		void		APE_API			setInitialDelay(APE_SharedInterface * iface, int samples);
		/// <summary>
		/// Creates a label according to the format string, that depends on the reference arguments.
		/// </summary>
		int			APE_API_VARI	createLabel(APE_SharedInterface * iface, const char * name, const char * fmt, ...);
		/// <summary>
		/// Returns the number of inputs associated with this plugin.
		/// </summary>
		int			APE_API			getNumInputs(APE_SharedInterface * iface);
		/// <summary>
		/// Returns the number of outputs associated with this plugin.
		/// </summary>
		int			APE_API			getNumOutputs(APE_SharedInterface * iface);
		/// <summary>
		/// Adds an automatable parameter to the GUI, using a list of values.
		/// </summary>
		int			APE_API			createMeter(APE_SharedInterface * iface, const char * name, float * extVal);
		/// <summary>
		/// Adds a button to the GUI.
		/// </summary>
		int			APE_API			createToggle(APE_SharedInterface * iface, const char * name, float * extVal);
		/// <summary>
		/// Returns the host's BPM
		/// </summary>
		double		APE_API			getBPM(APE_SharedInterface * iface);
		/// <summary>
		/// Gets the value of a control with the given ID
		/// </summary>
		float		APE_API			getCtrlValue(APE_SharedInterface * iface, int iD);
		/// <summary>
		/// Set the value of a control with the given ID
		/// </summary>
		void		APE_API			setCtrlValue(APE_SharedInterface * iface, int iD, float value);
		/// <summary>
		/// /Adds a plot to the GUI.
		/// </summary>
		int			APE_API			createPlot(APE_SharedInterface * iface, const char * name, const float * const vals, const unsigned int numVals);
		/// <summary>
		/// Adds a ranged knob
		/// </summary>
		int			APE_API			createRangeKnob(APE_SharedInterface * iface, const char * name, const char * unit, float * extVal, ScaleFunc scaleCB, float min, float max);

		int			APE_API			presentTrace(APE_SharedInterface* iface, const char** nameTuple, size_t numNames, const float* const values, size_t numValues);
		int			APE_API			createNormalParameter(APE_SharedInterface * iface, const char * name, const char * unit, PFloat* extVal, Transformer transformer, Normalizer normalizer, PFloat min, PFloat max);
		int			APE_API			createBooleanParameter(APE_SharedInterface * iface, const char * name, PFloat* extVal);


	};
#endif
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

	namespace ape::api
	{
		void clearThreadFaults();

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
		/// Prints a line with the color nColor in the console in the ape window associated with 
		/// the C script.
		/// </summary>
		int			APE_API_VARI	printThemedLine(APE_SharedInterface * iface, APE_TextColour color, const char * fmt, ...);
		/// <summary>
		/// presents a messagebox for the user, that may or may not be blocking
		/// </summary>
		int			APE_API			msgBox(APE_SharedInterface * iface, const char * text, const char * title, int nStyle, int nBlocking);
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
		int			APE_API			createMeter(APE_SharedInterface * iface, const char * name, const double* extVal, const double* peakVal);
		/// <summary>
		/// Returns the host's BPM
		/// </summary>
		double		APE_API			getBPM(APE_SharedInterface * iface);
		/// <summary>
		/// /Adds a plot to the GUI.
		/// </summary>
		int			APE_API			createPlot(APE_SharedInterface * iface, const char * name, const double * const vals, const unsigned int numVals);
		int			APE_API			presentTrace(APE_SharedInterface* iface, const char** nameTuple, size_t numNames, const float* const values, size_t numValues);
		int			APE_API			createNormalParameter(APE_SharedInterface * iface, const char * name, const char * unit, APE_Parameter* extVal, Transformer transformer, Normalizer normalizer, PFloat min, PFloat max);
		int			APE_API			createBooleanParameter(APE_SharedInterface * iface, const char * name, APE_Parameter* extVal);
		int			APE_API			createListParameter(APE_SharedInterface * iface, const char * name, APE_Parameter* extVal, int numValues, const char* const* values);
		int			APE_API			destroyResource(APE_SharedInterface * iface, int resource, int reserved);
		/// <summary>
		/// 
		/// </summary>
		/// <param name="targetSampleRate">
		/// Can either be:
		/// 1) a specific sample rate,
		/// 2) 0 for original sample rate (don't resample)
		/// 3) -1 for adopting current sample rate 
		/// </param>
		int			APE_API			loadAudioFile(APE_SharedInterface * iface, const char* path, double targetSampleRate, APE_AudioFile* result);
		struct APE_FFT*	APE_API		createFFT(struct APE_SharedInterface * iface, APE_DataType type, size_t size);
		void		APE_API			performFFT(struct APE_SharedInterface * iface, APE_FFT* fft, APE_FFT_Options options, const void* in, void* out);
		void		APE_API			releaseFFT(struct APE_SharedInterface * iface, APE_FFT* fft);
		
		void		APE_API			setTriggeringChannel(APE_SharedInterface * iface, int triggerChannel);
	};
#endif

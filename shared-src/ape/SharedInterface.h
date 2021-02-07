/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2017 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:SharedInterface.h
	
		Defines the shared interface between the engine and the plugins,
		providing a common API.

*************************************************************************************/

#ifndef APE_SHAREDINTERFACE_H
	#define APE_SHAREDINTERFACE_H

	#include "APE.h"

	typedef enum
	{
		APE_Alloc_Buffer,
		APE_Alloc_Tiny,
		APE_Alloc_Temp
	} APE_AllocationLabel;

	typedef enum
	{
		APE_TextColour_Default,
		APE_TextColour_Warning,
		APE_TextColour_Error
	} APE_TextColour;

	typedef enum 
	{
		APE_DataType_Single,
		APE_DataType_Double
	} APE_DataType;

	typedef enum
	{
		APE_FFT_Inverse		= 0 << 0,
		APE_FFT_Forward		= 1 << 0,
		APE_FFT_Real		= 1 << 1,
		APE_FFT_NonScaled	= 1 << 2
	} APE_FFT_Options;

	typedef enum
	{
		/// <summary>
		/// For checked builds, can include extra guards
		/// </summary>
		APE_Optimization_Debug,
		/// <summary>
		/// Whatever settings are the fastest to compile
		/// </summary>
		APE_Optimization_Fast,
		/// <summary>
		/// Whatever settings produce the fastest code
		/// </summary>
		APE_Optimization_Best
	} APE_Optimization_Level;

	struct APE_SharedInterface;

	struct APE_AudioFile
	{
		const char* name;
		unsigned long long samples;
		unsigned int channels;
		double fractionalLength;
		double sampleRate;
		const float* const* data;
	};

	struct APE_Parameter
	{
		PFloat old, next, step;
		int id, changeFlags;
	};

    /// <summary>
    /// <see cref="juce::AudioPlayHead::CurrentPositionInfo"/>
    /// </summary>
    struct APE_PlayHeadPosition
    {
        /** The tempo in BPM */
        double bpm;

        /** Time signature numerator, e.g. the 3 of a 3/4 time sig */
        int timeSigNumerator;
        /** Time signature denominator, e.g. the 4 of a 3/4 time sig */
        int timeSigDenominator;

        /** The current play position, in samples from the start of the edit. */
        long long timeInSamples;
        /** The current play position, in seconds from the start of the edit. */
        double timeInSeconds;

        /** For timecode, the position of the start of the edit, in seconds from 00:00:00:00. */
        double editOriginTime;

        /** The current play position, in pulses-per-quarter-note. */
        double ppqPosition;

        /** The position of the start of the last bar, in pulses-per-quarter-note.

            This is the time from the start of the edit to the start of the current
            bar, in ppq units.

            Note - this value may be unavailable on some hosts, e.g. Pro-Tools. If
            it's not available, the value will be 0.
        */
        double ppqPositionOfLastBarStart;

        /** The video frame rate, if applicable. */
        int frameRate;

        /** True if the transport is currently playing. */
        bool isPlaying;

        /** True if the transport is currently recording.

            (When isRecording is true, then isPlaying will also be true).
        */
        bool isRecording;

        /** The current cycle start position in pulses-per-quarter-note.
            Note that not all hosts or plugin formats may provide this value.
            @see isLooping
        */
        double ppqLoopStart;

        /** The current cycle end position in pulses-per-quarter-note.
            Note that not all hosts or plugin formats may provide this value.
            @see isLooping
        */
        double ppqLoopEnd;

        /** True if the transport is currently looping. */
        bool isLooping;
    };

	struct APE_SharedInterface
	{
		void		(APE_API * abortPlugin)				(struct APE_SharedInterface * iface, const char * reason);
		float		(APE_API * getSampleRate)			(struct APE_SharedInterface * iface);
		int			(APE_API_VARI * printLine)			(struct APE_SharedInterface * iface, unsigned nColor, const char * fmt, ...);
		int			(APE_API_VARI * printThemedLine)	(struct APE_SharedInterface * iface, APE_TextColour color, const char * fmt, ...);
		int			(APE_API * msgBox)					(struct APE_SharedInterface * iface, const char * text, const char * title, int nStyle, int nBlocking);
		long long	(APE_API * timerGet)				(struct APE_SharedInterface * iface);
		double		(APE_API * timerDiff)				(struct APE_SharedInterface * iface, long long time);
		void *		(APE_API * alloc)					(struct APE_SharedInterface * iface, APE_AllocationLabel label, size_t size, size_t align);
		void		(APE_API * free)					(struct APE_SharedInterface * iface, void * ptr);
		void		(APE_API * setInitialDelay)			(struct APE_SharedInterface * iface, int samples);
		int			(APE_API_VARI * createLabel)		(struct APE_SharedInterface * iface, const char * name, const char * fmt, ...);
		int			(APE_API * getNumInputs)			(struct APE_SharedInterface * iface);
		int			(APE_API * getNumOutputs)			(struct APE_SharedInterface * iface);
		int			(APE_API * createMeter)				(struct APE_SharedInterface * iface, const char * name, const double* extVal, const double* peakVal);
		double		(APE_API * getBPM)					(struct APE_SharedInterface * iface);
		int			(APE_API * createPlot)				(struct APE_SharedInterface * iface, const char * name, const double * const values, const unsigned int numVals);
		int			(APE_API * presentTrace)			(struct APE_SharedInterface * iface, const char** nameTuple, size_t numNames, const float* const values, size_t numValues);
		int			(APE_API * createNormalParameter)	(struct APE_SharedInterface * iface, const char * name, const char * unit, APE_Parameter* extVal, APE_Transformer transformer, APE_Normalizer normalizer, PFloat min, PFloat max);
		int			(APE_API * createBooleanParameter)	(struct APE_SharedInterface * iface, const char * name, APE_Parameter* extVal);
		int			(APE_API * createListParameter)		(struct APE_SharedInterface * iface, const char * name, APE_Parameter* extVal, int numValues, const char* const* values);
		int			(APE_API * destroyResource)			(struct APE_SharedInterface * iface, int resourceID, int reserved);
		int			(APE_API * loadAudioFile)			(struct APE_SharedInterface * iface, const char* path, double sampleRate, APE_AudioFile* result);
		int			(APE_API * createFFT)				(struct APE_SharedInterface * iface, APE_DataType type, size_t size);
		void		(APE_API * performFFT)				(struct APE_SharedInterface * iface, int fftID, APE_FFT_Options options, const void* in, void* out);
		void		(APE_API * releaseFFT)				(struct APE_SharedInterface * iface, int fftID);
		void		(APE_API * setTriggeringChannel)	(struct APE_SharedInterface * iface, int channel);
		int			(APE_API * createAudioOutputFile)	(struct APE_SharedInterface * iface, const char* relativePath, double sampleRate, int channels, int bits, float quality);
		void		(APE_API * writeAudioFile)			(struct APE_SharedInterface * iface, int file, unsigned int numSamples, const float* const* data);
		void		(APE_API * closeAudioFile)			(struct APE_SharedInterface * iface, int file);
        int         (APE_API * getPlayHeadPosition)     (struct APE_SharedInterface * iface, struct APE_PlayHeadPosition* result);
	};
	
#if defined(__cplusplus) && !defined(__cfront)

		//using SharedInterface = APE_SharedInterface;

#endif
#endif

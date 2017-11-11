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

	file:MacroConstants.h
	
		Definitions for lots of different macroes, used throughout the program.
		Detects OS and architechture.
		Solves incompabilities.

*************************************************************************************/

#ifndef APE_MACROCONSTANTS_H
	#define APE_MACROCONSTANTS_H

	#include <stdint.h>

	#define EXPORTED

	#define _CONSOLE_CLEAR_HISTORY
	#define __xstring(x) #x
	#define __tostring(x) __xstring(x)

	#if defined(_WIN64) || defined(__x86_64__)
		typedef unsigned long long XWORD;
		#define __M_64BIT_ 1
		#define _ARCH_STRING "64-bit"
	#else
		#define __M_32BIT_
		//typedef unsigned long XWORD;
		#define _ARCH_STRING "32-bit"
	#endif

	#if defined(_WIN32) || defined (_WIN64)

		#define __WINDOWS__
		#define _PROG_EXTENSION ".dll"
		#define DIR_SEP '\\'
	#elif defined (__MACH__) && (__APPLE__)
		#define DIR_SEP '/'
		#define __MAC__
		#ifdef APE_JUCE
			#define _PROG_EXTENSION ".component"
		#else
			#define _PROG_EXTENSION ".vst"
		#endif
	#endif
	#ifdef _MSC_VER

	#else

    #endif
    #define cwarn(text) message("[" __FILE__ "] (" __tostring(__LINE__) ") -> " __FUNCTION__ ": " text)

	#if defined(__WINDOWS__) || defined(__MAC__)
		#define __INTEL_ASSEMBLY_
		// gcc or msvc assembly syntax?
		#ifdef _MSC_VER
			#ifdef __M_64BIT_
				#define DBG_BREAK() DebugBreak();
			#else
				#define DBG_BREAK() __asm int 3
			#endif
		#else
			#define DBG_BREAK() __asm__("int $0x3")
		#endif
	#else
		#error Only Intel-compliant targets supported atm.
	#endif
	#if defined(_WIN32) || defined (_WIN64)
		#define isDebugged() IsDebuggerPresent()
		#define debug_out(x) OutputDebugString(x)
	#else
        // forward declare it
		namespace Misc
		{
			bool IsDebuggerPresent();
		};
		#define isDebugged() Misc::IsBeingDebugged()
		#define debug_out(x) (void*) 0
	#endif

	#ifdef _DEBUG
		#define BreakIfDebugged() if(isDebugged()) DBG_BREAK()
	#else
		#define BreakIfDebugged() (void *)0
	#endif

	/*
		On x64 windows, tcc emits wrong calling convention for floating point functions.
		We circumvent this using a hack.
	*/
	#if defined(__WINDOWS__) && defined(__M_64BIT_)
		#define APE_CHECK_FOR_TCC_HACK
	#endif

	#ifndef ArraySize
		#define ArraySize(x) (sizeof(x) / sizeof(x[0]))
	#endif

	#define DIRC_COMP(x) ((x) == '\\' || (x) == '/')

	#ifdef APE_JUCE
		#define _HOST_TARGET_TECH "JUCE"
		#define _VERSION_INT JucePlugin_VersionCode
		#define _VERSION_INT_STRING JucePlugin_VersionString
	#elif defined(APE_IPLUG)
		#define _HOST_TARGET_TECH "IPlug"
	#elif defined(APE_VST)
		#define _HOST_TARGET_TECH "VST"
	#endif
	#define _VERSION_STRING	_VERSION_SPECIFIC " " _VERSION_INT_STRING
	#define _PROGRAM_AUTHOR "Janus Thorborg"
	#define _TIME_OF_WRITING "2014"
	#define _HOMEPAGE_SENTENCE
	#define _ADDITIONAL_NOTES

	#define RADTOHZ(rads) (rads / PI * 2)

	#define lower_byte(w)		((unsigned char)(((unsigned long)(w)) & 0xff))
	#define _rgb_get_red(rgb)	(lower_byte(rgb))
	#define _rgb_get_green(rgb)	(lower_byte(((unsigned short)(rgb)) >> 8))
	#define _rgb_get_blue(rgb)	(lower_byte((rgb)>>16))

	#if defined(_MSC_VER) 
		#define NOMINMAX
		#if _MSC_VER >= 1700
			#define __CPP11__
		#endif
		#define __MSVC__
		#define APE_API _cdecl
		#define APE_STD_API _cdecl
		#define APE_API_VARI _cdecl
		#ifndef APE_ALL_WARNINGS
			#pragma warning(disable:4482) // qualified names
			#pragma warning(disable:4005) // redefinition of macros
			#pragma warning(disable:4201) // nameless structs/unions
			#pragma warning(disable:4100) // unused parameters
			#pragma warning(disable:4702) // unreachable code
			#pragma warning(disable:4996) // std::copy with pointers
		#endif
		#define __llvm_DummyNoExcept
		#define __alignas(x) __declspec(align(x))
		#define __thread_local __declspec(thread)
		#define siginfo_t void
	#elif defined(__llvm__)
		#define APE_API __cdecl
		#define APE_STD_API __cdecl
		#define APE_API_VARI __cdecl
		// change this to check against _LIBCPP_VERSION to detect cpp11 support
		#if 1
			#define __CPP11__
		#endif
		#define __LLVM__
		// a bug in current apple llvm emits an error message on derived
		// classes constructors/destructors if they don't have this specifier
		#define __llvm_DummyNoExcept noexcept
		#define __thread_local __thread
		#define __alignas(x) alignas(x)
	#elif defined(__GNUG__)
		#if __GNUG__ >= 4

			#define __CPP11__
		#endif
		#define APE_API __cdecl
		#define APE_STD_API __cdecl
		#define APE_API_VARI __cdecl
		#define __GCC__
		#define __llvm_DummyNoExcept
	#else
		#error "Compiler not supported."
	#endif

	#if defined(__LLVM__) || defined(__GCC__)
		// sets a standard for packing structs.
		// this is enforced on msvc by using #pragma pack()
		#define PACKED __attribute__((packed))
		// gives compability on gcc and llvm, and adds security on msvc
		#define sprintf_s sprintf
		#define strcpy_s strcpy
	#else
		#define PACKED
	#endif

	// program types
	namespace APE
	{
		#ifdef _DOUBLE_PRECISION
			typedef double Float;
		#else
			typedef float Float;
		#endif
		typedef int32_t Int;
		typedef int64_t Long;
	}

	#define APE_DEF_ALIGN 4

	#ifndef MAX_PATH
		#define MAX_PATH 260
	#endif
	#define NI "Error: This feature is not implemented for target (" __FILE__ ", " __LINE__ " - " __FUNCTION__ "."

	#ifndef PLUG_SC_CHANS
		#define PLUG_SC_CHANS 0
	#endif
	#ifndef PUBLIC_NAME
		#define PUBLIC_NAME PLUG_NAME
	#endif
	#ifndef IPLUG_CTOR
		#define IPLUG_CTOR(nParams, nPresets, instanceInfo) \
		IPlug(instanceInfo, nParams, PLUG_CHANNEL_IO, nPresets, \
		PUBLIC_NAME, "", PLUG_MFR, PLUG_VER, PLUG_UNIQUE_ID, PLUG_MFR_ID, \
		PLUG_LATENCY, PLUG_DOES_MIDI, PLUG_DOES_STATE_CHUNKS, PLUG_IS_INST, PLUG_SC_CHANS)
	#endif



#endif
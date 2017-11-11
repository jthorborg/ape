/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.4.0.
	 
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

	file:Ape.h
	
		Common basic definitions for use in other ape headers.

*************************************************************************************/

#ifndef APE_APE_H
	#define APE_APE_H

	#ifndef APE_MACROCONSTANTS_H
		#ifdef _WIN32
			#define __WINDOWS__
			#define EXPORTED __declspec(dllexport)
		#elif (__MACH__) && (__APPLE__)
			#define EXPORTED 
			#define __MAC__
		#endif
		#ifdef _MSC_VER
			#define APE_STD_API _cdecl
			#define APE_API APE_STD_API
			#define APE_API_VARI _cdecl
		#else
			#define APE_STD_API __cdecl
			#define APE_API APE_STD_API
			#define APE_API_VARI APE_STD_API
		#endif
	#endif

	typedef void (APE_API * APE_ErrorFunc)(void*, const char *);
	typedef float(APE_API * APE_ScaleFunc)(float value, float _min, float _max);

	// Status definitions for operation and states.
	typedef enum 
	{
		STATUS_OK = 0,		// operation completed succesfully
		STATUS_ERROR = 1,	// operation failed, state errornous
		STATUS_WAIT = 2,	// operation not completed yet
		STATUS_SILENT = 3,	// the plugin should not process data
		STATUS_READY = 4,	// ready for any operation
		STATUS_DISABLED = 5,// plugin is disabled
		STATUS_HANDLED = 6,  // plugin handled request, host shouldn't do anything.
		STATUS_NOT_IMPLEMENTED = 7 // operation not supported
	} APE_Status;
	
	#if defined(__cplusplus) && !defined(__cfront)
		namespace ape
		{
			using Status = APE_Status;
			using ScaleFunc = APE_ScaleFunc;
			using ErrorFunc = APE_ErrorFunc;
		};
	#endif
#endif
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

	file:Platformspecific.h
	
		Defines operating system and includes common system files.
		Defines some types according to system.

*************************************************************************************/

#ifndef _PLATFORMDEPENDENT_H

	#define _PLATFORMDEPENDENT_H
	#include "MacroConstants.h"

	#ifdef __WINDOWS__
		#include <Windows.h>
	#else
		#include <dlfcn.h>
		#include <pthread.h>
		#include <sys/types.h>
		#include <sys/sysctl.h>
		#include <unistd.h>
		#include <mach-o/dyld.h>
		#include <sys/time.h>
		#include <fcntl.h>
		#include <mach/mach_time.h>
		#include "MacSupport.h"
		#define __CSTATE_USE_SIGACTION
	#endif

	#ifndef __MSVC__
		#include <cfenv>
		// find similar header (set fpoint mask) for non-mscv on windows
		#include <xmmintrin.h>
	#endif

	#ifdef _USE_SCINTILLA
		#include "SciLexer.h"
		namespace APE 
		{
			typedef Scintilla::SciLexer ExternalEditor;
		};
	#else
		#include "CJuceEditor.h"
		namespace APE
		{
			typedef CJuceEditor ExternalEditor;
		};
	#endif
#endif
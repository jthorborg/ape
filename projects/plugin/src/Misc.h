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

	file:Misc.h
	
		Whatever doesn't fit other places, globals.
		Helper classes.
		Global functions.

*************************************************************************************/

#ifndef _MISC_H
	#define _MISC_H

	#include <string>
	#include "MacroConstants.h"
	#include <sstream>
	#include "Common.h"
	#include "PlatformSpecific.h"

	namespace APE
	{
		namespace Misc 
		{

			std::string GetTime ();
			long long ClockCounter();
			int ObtainUniqueInstanceID();
			void ReleaseUniqueInstanceID(int ID);
			bool IsBeingDebugged();
			
			const std::string & DirectoryPath();

			enum ExceptionStatus {
				Undefined,
				CSubsystem

			};
			long Round(double number);
			long Delay(int ms);
			unsigned int QuickTime();
			int GetSizeRequiredFormat(const char * fmt, va_list pargs);

			std::string StringFromVersion(unsigned int version);

			class CStringFormatter
			{
				std::stringstream stream;
			public:
				CStringFormatter(const std::string & start)
				{
					stream << start;
				}
				CStringFormatter() {}
				template <typename Type>
					std::stringstream & operator << (const Type & input)
					{
						stream << input;
						return stream;
					}
				std::string str()
				{
					return stream.str();
				}
			};

			class CStrException : public std::exception
			{
				std::string info;
			public:
				CStrException(const std::string & str)
					: info(str)
				{}
				virtual const char * what() const throw()
				{
					return info.c_str();
				}
			};
						
			enum MsgButton : int
			{
				#ifdef __WINDOWS__
					bYes = IDYES,
					bNo = IDNO,
					bRetry = IDRETRY,
					bTryAgain = IDTRYAGAIN,
					bContinue = IDCONTINUE,
					bCancel = IDCANCEL, 
					bError = 0
				#elif defined(APE_IPLUG)
					bYes = IDYES,
					bNo = IDNO,
					bRetry = IDRETRY,
					bTryAgain = bRetry,
					bContinue = bYes,
					bCancel = IDCANCEL
				#elif defined(__MAC__)
					bYes = 6,
					bNo = 7,
					bRetry = 4,
					bTryAgain = 10,
					bContinue = 11,
					bCancel = 2,
					bOk = bYes
				#endif
			};
			enum MsgStyle : int
			{
				#ifdef __WINDOWS__
					sOk = MB_OK,
					sYesNoCancel = MB_YESNOCANCEL,
					sConTryCancel = MB_CANCELTRYCONTINUE
				#elif defined(APE_IPLUG)
					sOk = MB_OK,
					sYesNoCancel = MB_YESNOCANCEL,
					sConTryCancel = sYesNoCancel
				#elif defined(__MAC__)
					sOk = 0,
					sYesNoCancel = 3,
					sConTryCancel = 6
				#endif
			};
			enum MsgIcon : int
			{
				#ifdef __WINDOWS__
					iStop = MB_ICONSTOP,
					iQuestion = MB_ICONQUESTION,
					iInfo = MB_ICONINFORMATION,
					iWarning = MB_ICONWARNING
				#elif defined(APE_IPLUG)
					iStop = MB_ICONSTOP,
					iQuestion = MB_ICONINFORMATION,
					iInfo = MB_ICONINFORMATION,
					iWarning = MB_ICONSTOP
				#elif defined(__MAC__)
					iStop = 0x10,
					iWarning = iStop,
					iInfo = 0x40,
					iQuestion = 0x20
				#endif
			};

			int MsgBox(	const std::string & text, 
						const std::string & title = _PROGRAM_NAME_ABRV, 
						int nStyle = MsgStyle::sOk, 
						void * parent = NULL, 
						const bool bBlocking = true);
			

			/*
				This is not a true 'acuqire' spinlock. See CMutex for this functionality
			*/
			template<typename T> 
				bool SpinLock(unsigned int ms, T & bVal) {
					unsigned start;
					int ret;
				loop:
					start = QuickTime();
					while(!!bVal) {
						if((QuickTime() - start) > ms)
							goto time_out;
						Delay(0);
					}
					// normal exitpoint
					return true;
					// deadlock occurs

				time_out:

					ret = MsgBox("Deadlock detected in spinlock: Protected resource is not released after max interval. "
							"Wait again (try again), release resource (continue) - can create async issues - or exit (cancel)?", 
							_PROGRAM_NAME_ABRV " Error!", 
							sConTryCancel | iStop);
					switch(ret) 
					{
					case MsgButton::bTryAgain:
						goto loop;
					case MsgButton::bContinue:
						bVal = !bVal; // flipping val, and it's a reference so should release resource.
						return false;
					case MsgButton::bCancel:
	#pragma cwarn("Find a more gentle way to exit...")
						exit(-1);
					}
					// not needed (except for warns)
					return false;
				}
		}; // Misc
	}; // APE
#endif
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

	file:CState.h
	
		Implements interface for the c-subsystem-controller and communication between
		them; handles compiling and run-time resolution of dispatches from C to C++ 
		member functions, as well as wrapping unsafe code turning hardware exceptions
		into the software exception CSystemException. Allows setting of FPU environment.

*************************************************************************************/

#ifndef APE_CSTATE_H
	#define APE_CSTATE_H

	#include <cpl/PlatformSpecific.h>
	#include "MacroConstants.h"
	#include "CMemoryGuard.h"
	#include <vector>
	#include <exception>
	#include <signal.h>
	#include "CAllocator.h"
	#include <ape/Project.h>
	#include <ape/Events.h>
	#include <thread>
	#include <cpl/CMutex.h>
	#include "ProjectEx.h"

	namespace ape
	{

		typedef void * RawFunctionPtr;
		typedef int VstInt32;

		class Engine;
		struct Module;
		class CState;
		class CCodeGenerator;
		struct SharedInterfaceEx;
		
		/*
			This class is responsible for interconnecting the C and C++ systems, dispatch calls correctly and hold the state of the C-system
		*/			
		class CState
		{

		public:

			struct CSystemException;

			SharedInterfaceEx & getSharedInterface();
			CAllocator & getPluginAllocator();
			std::vector<CMemoryGuard> & getPMemory();

			CState(ape::Engine * effect);
			~CState();
			/*
				utility

			*/
			bool compileCurrentProject();
			void setNewProject(ProjectEx *);
			static void useFPUExceptions(bool b);
			static std::string formatExceptionMessage(const CSystemException &);
			void projectCrashed();
			/*
				safe wrappers around compiler
			*/
			Status activateProject();
			Status disableProject(bool didMisbehave);
			Status processReplacing(Float ** in, Float ** out, Int sampleFrames);
			Status onEvent(Event * e);
			/*
				RunProtectedCode
					calls lambda inside 'safe wrappers', catches OS errors and filters them.
					Throws CSystemException on errors, crashes on severe errors.
			*/
			template <class func>
				void RunProtectedCode(func function)
				{
					// save this pointer
					threadData.activeStateObject = this;
					/*
						This is not really crossplatform, but I'm working on it!
					*/
					#ifdef __WINDOWS__
						bool exception_caught = false;

						CSystemException::eStorage exceptionData;

						__try {
							
						//	BreakIfDebugged();
							function();
						}
						__except (CState::structuredExceptionHandler(GetExceptionCode(), exceptionData, GetExceptionInformation()))
						{
							// this is a way of leaving the SEH block before we throw a C++ software exception
							exception_caught = true;
						}
						if(exception_caught)
							throw CSystemException(exceptionData);
					#else
						// trigger int 3
						BreakIfDebugged();
						// set the jump in case a signal gets raised
						if(sigsetjmp(threadData.threadJumpBuffer, 1))
						{
							/*
								return from exception handler.
								current exception is in CState::currentException
							*/
							threadData.activeStateObject = nullptr;
							throw threadData.currentException;
						}
						// run the potentially bad code
						function();

					#endif
					/*
					 
						This is the ideal code.
						Unfortunately, to succesfully throw from a signal handler,
						there must not be any non-c++ stackframes in between.
						If, the exception will be uncaught even though we have this handler
						lower down on the stack. Therefore, we have to implement the 
						exception system using longjmps
					
					 
						try {
							// <-- breakpoint disables SIGBUS
							BreakIfDebugged();
							function();
						}
						catch(const CSystemException & e) {
							activeStateObject = nullptr;
							// rethrow it
							throw e;
						}
						catch(...)
						{
							Misc::MsgBox("Unknown exception thrown, breakpoint is triggered afterwards");
							BreakIfDebugged();
						
						}
					 */
					// delete the active object
					threadData.activeStateObject = nullptr;
				}

			/*
				Base exception for all critical exceptions thrown in this program.
				Derives from std::exception, but has no meaningful .what()
				- See CState::formatExceptionMessage
			*/



			struct CSystemException :  public std::exception
			{
			public:
				// this is kinda ugly, but needed to create a simple interface, abstract to seh and signals
				struct eStorage
				{
					const void * faultAddr; // the address the exception occured
					const void * attemptedAddr; // if exception is a memory violation, this is the attempted address
					XWORD exceptCode; // the exception code
					int extraInfoCode; // addition, exception-specific code
					int actualCode; // what signal it was

					union {
						// hack hack union
						bool safeToContinue; // exception not critical or can be handled
						bool aVInProtectedMemory; // whether an access violation happened in our protected memory
					};

					eStorage(XWORD exp, bool resolved = true, const void * faultAddress = nullptr, 
								const void * attemptedAddress = nullptr, int extraCode = 0, int actualCode = 0)
						:	exceptCode(exp), safeToContinue(resolved), faultAddr(faultAddress),
							attemptedAddr(attemptedAddress), extraInfoCode(extraCode), actualCode(actualCode)
					{

					}

					eStorage()
						: exceptCode(0), safeToContinue(false), faultAddr(nullptr),
						attemptedAddr(nullptr), extraInfoCode(0), actualCode(0)
					{

					}
				} data;

				enum status : XWORD {
					nullptr_from_plugin = 1,
					#ifdef _WINDOWS_
						// this is not good: these are not crossplatform constants.
						access_violation = EXCEPTION_ACCESS_VIOLATION,
						intdiv_zero = EXCEPTION_INT_DIVIDE_BY_ZERO,
						fdiv_zero = EXCEPTION_FLT_DIVIDE_BY_ZERO,
						finvalid = EXCEPTION_FLT_INVALID_OPERATION,
						fdenormal = EXCEPTION_FLT_DENORMAL_OPERAND,
						finexact = EXCEPTION_FLT_INEXACT_RESULT,
						foverflow = EXCEPTION_FLT_OVERFLOW,
						funderflow = EXCEPTION_FLT_UNDERFLOW
					#elif defined(__MAC__)
						access_violation = SIGSEGV,
						intdiv_zero,
						fdiv_zero,
						finvalid,
						fdenormal,
						finexact,
						foverflow,
						funderflow,
						intsubscript,
						intoverflow
					#endif
				};
				CSystemException()
				{
					
				}

				CSystemException(const eStorage & eData)
					: data(eData)
				{

				}

				CSystemException(XWORD exp, bool resolved = true, const void * faultAddress = nullptr, const void * attemptedAddress = nullptr, int extraCode = 0, int actualCode = 0)
					: data(exp, resolved, faultAddress, attemptedAddress, extraCode, actualCode)
				{
				
				}
				const char * what() const noexcept override
				{
					return "CState::CSystemException (non-software exception)";
				}
			};

		private:
			
			 static struct StaticData
			 {
				#ifndef __WINDOWS__
					std::map<int, struct sigaction> oldHandlers;
					struct sigaction newHandler;
					volatile int signalReferenceCount;
					CMutex::Lockable signalLock;
				#endif
			 
			 } staticData;
			 
			 
			static __thread_local struct ThreadData
			{
				CState * activeStateObject;
				#ifndef __WINDOWS__
					sigjmp_buf threadJumpBuffer;
					CSystemException currentException;
				#endif
				unsigned fpuMask;
			} threadData;
			 
			std::unique_ptr<SharedInterfaceEx> sharedObject;
			std::unique_ptr<CCodeGenerator> generator;
			Engine * engine;
			CAllocator pluginAllocator;
			std::vector<CMemoryGuard> protectedMemory;

			std::unique_ptr<ProjectEx, ProjectDeleter> curProject;


			XWORD structuredExceptionHandler(XWORD _code, CSystemException::eStorage & e, void * _systemInformation);
			void createSharedObject();
			static void signalHandler(int some_number);
			static void signalActionHandler(int signal, siginfo_t * siginfo, void * extraData);
			static bool registerHandlers();
			static bool unregisterHandlers();
		};
	};
#endif
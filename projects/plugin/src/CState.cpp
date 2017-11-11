/*************************************************************************************

	Audio Programming Environment VST. 
		
		VST is a trademark of Steinberg Media Technologies GmbH.

    Copyright (C) 2013 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:CState.cpp
	
		Implementation of CState.h

*************************************************************************************/

#include <cpl/PlatformSpecific.h>
#include "SharedInterfaceEx.h"
#include "CState.h"
#include "Engine.h"
#include "CCodeGenerator.h"
#include "GraphicUI.h"
#include "CConsole.h"
#include <sstream>

namespace ape
{
	
	CState::StaticData CState::staticData;
	thread_local CState::ThreadData CState::threadData;


	CState::CState(Engine * engine) 
		: engine(engine), curProject(nullptr)
	{
		generator = std::make_unique<CCodeGenerator>(engine);
		generator->setErrorFunc(engine->errPrint, engine);
		createSharedObject();
		#ifndef __WINDOWS__
			registerHandlers();
		#endif
	}

	CState::~CState() 
	{
		if (curProject)  
		{
			generator->releaseProject(*curProject);
		}

		unregisterHandlers();
	}

	void CState::createSharedObject() 
	{
		sharedObject = std::make_unique<SharedInterfaceEx>(*engine, *this);
	}

	SharedInterfaceEx & CState::getSharedInterface()
	{
		return *sharedObject.get();
	}


	/*********************************************************************************************

		Getter for the plugin allocator.

	 *********************************************************************************************/
	ape::CAllocator & CState::getPluginAllocator() 
	{
		return pluginAllocator;
	}
	/*********************************************************************************************

		Getter for the protected memory.

	 *********************************************************************************************/
	std::vector<ape::CMemoryGuard> & CState::getPMemory() 
	{
		return protectedMemory;
	}

	/*********************************************************************************************

		Tells compiler to compile this->project.

	 *********************************************************************************************/
	bool CState::compileCurrentProject()
	{
		if (curProject)
		{
			this->curProject->iface = sharedObject.get();
			if (!generator->compileProject(*curProject))
				return false;
			if (!generator->initProject(*curProject))
				return false;
		}
		return true;
	}
	/*********************************************************************************************

		Calls compilers activate function.

	 *********************************************************************************************/
	Status CState::activateProject()
	{
		Status ret = Status::STATUS_ERROR;
		this->RunProtectedCode( [&]
			{
				ret = generator->activateProject(*curProject);
			}
		);
		return ret;
	}
	/*********************************************************************************************

		Calls compilers disable function.

	 *********************************************************************************************/
	Status CState::disableProject(bool didMisbehave)
	{
		Status ret = Status::STATUS_ERROR;
		
		if (curProject)
		{
			this->RunProtectedCode([&]
			{
				ret = generator->disableProject(*curProject, didMisbehave);
			}
			);
		}

		return ret;
	}
	/*********************************************************************************************

		Calls plugin's processReplacer

	 *********************************************************************************************/
	Status CState::processReplacing(float ** in, float ** out, std::size_t sampleFrames)
	{
		Status ret = Status::STATUS_ERROR;
		if (curProject)
		{
			this->RunProtectedCode([&]
			{
				ret = generator->processReplacing(*curProject, in, out, sampleFrames);
			}
			);
		}
		return ret;

	}
	/*********************************************************************************************

		Calls plugin's onEvent handler

	 *********************************************************************************************/
	Status CState::onEvent(Event * e)
	{
		Status ret = Status::STATUS_ERROR;
		if (curProject)
		{
			this->RunProtectedCode([&]
			{
				ret = generator->onEvent(*curProject, e);
			}
			);
		}
		return ret;
	}
	/*********************************************************************************************

		Releases old project and updates it to new one. Does not check whether project is used!!

	 *********************************************************************************************/
	void CState::setNewProject(ape::ProjectEx * project)
	{
		if(curProject)  
		{
			generator->releaseProject(*curProject);
		}
		curProject.reset(project);
	}

	/*********************************************************************************************

		Formats a string and returns it. It explains what went wrong.

	 *********************************************************************************************/
	std::string CState::formatExceptionMessage(const CSystemException & e) 
	{
		std::stringstream base;
		base << "Exception at " << std::hex << e.data.faultAddr << ": ";
		switch(e.data.exceptCode)
		{
		case CSystemException::status::intdiv_zero:
				return base.str() + "An integral division-by-zero was performed";
		case CSystemException::status::funderflow:
				return base.str() + "A floating point operation resulted in underflow";
		case CSystemException::status::foverflow :
				return base.str() + "A floating point operation resulted in overflow";
		case CSystemException::status::finexact:
				return base.str() + "A floating point operation's result cannot be accurately expressed";
		case CSystemException::status::finvalid:
				return base.str() + "One of the operands for a floating point operation was invalid (typically negative numbers for sqrt, exp, log)";
		case CSystemException::status::fdiv_zero:
				return base.str() + "A floating point division-by-zero was performed";
		case CSystemException::status::fdenormal:
				return base.str() + "One of the operands for a floating point operation was denormal (too small to be represented)";
		case CSystemException::status::nullptr_from_plugin:
				return base.str() + "An API function was called with 'this' as an null pointer.";
		case CSystemException::status::access_violation:
			{
				std::stringstream fmt;
				#ifndef __WINDOWS__
					switch(e.data.actualCode)
					{
						case SIGSEGV:
						{
							fmt << "Segmentation fault ";
							switch(e.data.extraInfoCode)
							{
								case SEGV_ACCERR:
									fmt << "(invalid permission for object) ";
									break;
								case SEGV_MAPERR:
									fmt << "(address not mapped for object) ";
									break;
							}
							break;
						}
						case SIGBUS:
						{
							fmt << "Bus error ";
							switch(e.data.extraInfoCode)
							{
								case BUS_ADRALN:
									fmt << "(invalid address alignment) ";
									break;
								case BUS_ADRERR:
									fmt << "(non-existant address) ";
									break;
								case BUS_OBJERR:
									fmt << "(object hardware error) ";
							}
							break;
							
						}
						default:
							fmt << "Access violation ";
							break;
					}
				#else
					fmt << "Access violation ";

					switch (e.data.actualCode)
					{
						case 0:
							fmt << "reading ";
							break;
						case 1:
							fmt << "writing ";
							break;
						case 8:
							fmt << "executing ";
							break;
						default:
							fmt << "(unknown error?) at ";
							break;
					};


				#endif
				fmt << " address " << std::hex << e.data.attemptedAddr << ".";
				return base.str() + fmt.str();
			}
		default:
			return base.str() + " Unknown exception (BAD!).";
		};
	}
	/*
		unless you want to have a heart attack, dont look at this code.
	*/
	#ifdef __MSVC__
		#pragma fenv_access (on)
	#else
		#pragma STDC FENV_ACCESS on
	#endif
	/*********************************************************************************************

		Toggle floating point exceptions on and off.

	 *********************************************************************************************/
	void CState::useFPUExceptions(bool bVal) 
	{
		// always call this!!#
		#ifdef CPL_MSVC
			_clearfp();
			unsigned int nfpcw = 0;
			if(bVal)
			{
				unsigned int temp;
				// save old environment
				_controlfp_s(&threadData.fpuMask, 0, 0);

				// Make the new fp env same as the old one,
				// except for the changes we're going to make
				nfpcw = _EM_INVALID | _EM_DENORMAL | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW; 
				//Update the control word with our changes
				_controlfp_s(&temp, 1, nfpcw);
			}
			else 
			{
				_controlfp_s(&nfpcw, 1, threadData.fpuMask);
			}
		#else
			if(bVal)
			{
				// always call this - changes in masks may trigger exceptions
				std::feclearexcept(FE_ALL_EXCEPT);
				threadData.fpuMask = _MM_GET_EXCEPTION_MASK();
				
				unsigned nfpcw = _MM_MASK_INVALID | _MM_MASK_DENORM | _MM_MASK_DIV_ZERO | _MM_MASK_OVERFLOW
								| _MM_MASK_UNDERFLOW |_MM_MASK_INEXACT;
				
				_MM_SET_EXCEPTION_MASK(threadData.fpuMask & ~nfpcw);

			}
			else
			{
				// always call this - changes in masks may trigger exceptions
				std::feclearexcept(FE_ALL_EXCEPT);
				// check return value here?
				_MM_SET_EXCEPTION_MASK(threadData.fpuMask);
			}
		#endif
	}

	/*********************************************************************************************

		Structured exception handler for windows

	 *********************************************************************************************/
	XWORD CState::structuredExceptionHandler(XWORD _code, CSystemException::eStorage & e, void * systemInformation)
	{
		CPL_BREAKIFDEBUGGED();

		#ifdef __WINDOWS__
			auto exceptCode = _code;
			bool safeToContinue(false);
			void * exceptionAddress = nullptr;
			int additionalCode = 0;
			EXCEPTION_POINTERS * exp = reinterpret_cast<EXCEPTION_POINTERS *>(systemInformation);
			if (exp)
				exceptionAddress = exp->ExceptionRecord->ExceptionAddress;
			switch(_code)
			{
			case EXCEPTION_ACCESS_VIOLATION:
				{
					EXCEPTION_POINTERS * exp = reinterpret_cast<EXCEPTION_POINTERS *>(systemInformation);
					std::ptrdiff_t addr = 0; // nullptr invalid here?

					if (exp)
					{
						// the address that was attempted
						addr = exp->ExceptionRecord->ExceptionInformation[1];
						// 0 = read violation, 1 = write violation, 8 = dep violation
						additionalCode = static_cast<int>(exp->ExceptionRecord->ExceptionInformation[0]);
					}
					if(addr) {
						for( auto it = protectedMemory.begin(); it != protectedMemory.end(); ++it) {

							// is the attempted access in our guarded code? if, we can safely signal
							// that we can continue.
							if(it->in_range((void*)addr)) {
								safeToContinue = true;
								break;
							}
						}
					}

					e = CSystemException::eStorage(exceptCode, safeToContinue, exceptionAddress, (const void *) addr, additionalCode);

					return EXCEPTION_EXECUTE_HANDLER;
				}
				/*
					trap all math errors:
				*/
			case EXCEPTION_INT_DIVIDE_BY_ZERO:
			case EXCEPTION_FLT_UNDERFLOW:
			case EXCEPTION_FLT_OVERFLOW: 
			case EXCEPTION_FLT_INEXACT_RESULT:
			case EXCEPTION_FLT_INVALID_OPERATION:
			case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			case EXCEPTION_FLT_DENORMAL_OPERAND:
				_clearfp();
				safeToContinue = true;

				e = CSystemException::eStorage(exceptCode, safeToContinue, exceptionAddress);

				return EXCEPTION_EXECUTE_HANDLER;

			default:
				return EXCEPTION_CONTINUE_SEARCH;
			};
			return EXCEPTION_CONTINUE_SEARCH;

		#endif
		return 0;
	}

	/*********************************************************************************************

		Implementation specefic exception handlers for unix systems.

	 *********************************************************************************************/
	void CState::signalHandler(int some_number)
	{
		throw (XWORD) some_number;
	}
	/*********************************************************************************************

		Eventhandler that is noticed when current project crashed.

	 *********************************************************************************************/
	void CState::projectCrashed()
	{
		curProject->state = CodeState::Disabled;
	}

	void CState::signalActionHandler(int sig, siginfo_t * siginfo, void * extra)
	{
		#ifndef __WINDOWS__
			// consider locking signalLock here -- not sure if its well-defined, though
		
			/*
				Firstly, check if the exception occured at our stack, after runProtectedCode 
				which sets activeStateObject to a valid object
			*/
			if(threadData.activeStateObject)
			{
				/*
					handle the exception here.
				*/
			
				const void * fault_address = siginfo ? siginfo->si_addr : nullptr;
				auto ecode = siginfo->si_code;
				bool safeToContinue = false;

				// -- this should be handled by siglongjmp
				//sigemptyset (&newAction.sa_mask);
				//sigaddset(&newAction.sa_mask, sig);
				//sigprocmask(SIG_UNBLOCK, &newAction.sa_mask, NULL);
			
				switch(sig)
				{
					case SIGBUS:
					case SIGSEGV:
					{
						// for sigsegv and sigbus, siginfo->si_addr is the attempted address
						// to find the faulting address, we have to look at the context instead.
						const void * in_address = nullptr;
						for( auto it = threadData.activeStateObject->protectedMemory.begin();
							it != threadData.activeStateObject->protectedMemory.end();
							++it)
						{
						
							// is the attempted access in our guarded code? if, we can safely signal
							// that we can continue.
							if(it->in_range(fault_address)) {
								safeToContinue = true;
								break;
							}
						}

					
						threadData.currentException = CSystemException(CSystemException::access_violation,
												 safeToContinue,
												 in_address,
												 fault_address,
												 ecode,
												 sig);
						// jump back to CState::runProtectedCode. Note, we know that function was called
						// earlier in the stackframe, because threadData.activeStateObject is non-null
						// : that field is __only__ set in runProtectedCode. Therefore, the threadJumpBuffer
						// IS valid.
						siglongjmp(threadData.threadJumpBuffer, 1);
						break;
					}
					case SIGFPE:
					{
						// exceptions that happened are still set in the status flags - always clear these,
						// or the exception might throw again
						std::feclearexcept(FE_ALL_EXCEPT);
						CSystemException::status code_status;
						switch(ecode)
						{
							case FPE_FLTDIV:
								code_status = CSystemException::status::fdiv_zero;
								break;
							case FPE_FLTOVF:
								code_status = CSystemException::status::foverflow;
								break;
							case FPE_FLTUND:
								code_status = CSystemException::status::funderflow;
								break;
							case FPE_FLTRES:
								code_status = CSystemException::status::finexact;
								break;
							case FPE_FLTINV:
								code_status = CSystemException::status::finvalid;
								break;
							case FPE_FLTSUB:
								code_status = CSystemException::status::intsubscript;
								break;
							case FPE_INTDIV:
								code_status = CSystemException::status::intdiv_zero;
								break;
							case FPE_INTOVF:
								code_status = CSystemException::status::intoverflow;
								break;
						}
						safeToContinue = true;
						threadData.currentException = CSystemException(code_status, safeToContinue, fault_address);
					
						// jump back to CState::runProtectedCode. Note, we know that function was called
						// earlier in the stackframe, because threadData.activeStateObject is non-null
						// : that field is __only__ set in runProtectedCode. Therefore, the threadJumpBuffer
						// IS valid. 
						siglongjmp(threadData.threadJumpBuffer, 1);
						break;
					}
					default:
						goto default_handler;
				} // switch signal
			} // if threadData.activeStateObject
		
			/*
				Exception happened in some arbitrary place we have no knowledge off.
				First we try to call the old signal handlers
			*/
		
		
		default_handler:
			/*
				consider checking here that sa_handler/sa_sigaction is actually valid and not something like
				SIG_DFLT, in which case re have to reset the handlers and manually raise the signal again
			*/
		
			if(staticData.oldHandlers[sig].sa_flags & SA_SIGINFO)
			{
				if(staticData.oldHandlers[sig].sa_sigaction)
					return staticData.oldHandlers[sig].sa_sigaction(sig, siginfo, extra);
			}
			else
			{
				if(staticData.oldHandlers[sig].sa_handler)
					return staticData.oldHandlers[sig].sa_handler(sig);
			}
			/*
				WE SHOULD NEVER REACH THIS POINT. NEVER. Except for nuclear war and/or nearby black hole
			*/
		
			// no handler found, throw exception (that will call terminate)
		
			throw std::runtime_error(_PROGRAM_NAME " - CState::signalActionHandler called for unregistrered signal; no appropriate signal handler to call.");
		#endif
	}

	/*********************************************************************************************

		Implementation specefic exception handlers for unix systems.

	 *********************************************************************************************/
	bool CState::registerHandlers()
	{
		#ifndef __WINDOWS__
			CMutex lock(staticData.signalLock);
			if(!staticData.signalReferenceCount)
			{
				#ifdef __CSTATE_USE_SIGNALS
					signal(SIGFPE, &CState::signalHandler);
					signal(SIGSEGV, &CState::signalHandler);
				#elif defined (__CSTATE_USE_SIGACTION)
					staticData.newHandler.sa_sigaction = &CState::signalActionHandler;
					staticData.newHandler.sa_flags = SA_SIGINFO;
					staticData.newHandler.sa_mask = 0;
					sigemptyset(&staticData.newHandler.sa_mask);
					sigaction(SIGSEGV, &staticData.newHandler, &staticData.oldHandlers[SIGSEGV]);
					sigaction(SIGFPE, &staticData.newHandler, &staticData.oldHandlers[SIGFPE]);
					sigaction(SIGBUS, &staticData.newHandler, &staticData.oldHandlers[SIGBUS]);
				#endif
			}
			staticData.signalReferenceCount++;
			return true;
		#endif
		return false;
	}
	
	bool CState::unregisterHandlers()
	{
		#ifndef __WINDOWS__
			CMutex lock(staticData.signalLock);
			staticData.signalReferenceCount--;
			if(staticData.signalReferenceCount == 0)
			{
				for(auto & signalData : staticData.oldHandlers)
				{
					// restore all registrered old signal handlers
					sigaction(signalData.first, &signalData.second, nullptr);
				}
				staticData.oldHandlers.clear();
				return true;
			}
			return false;
		#endif
		return false;
	}
}
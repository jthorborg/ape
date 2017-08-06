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

	file:CThread.h
		
		Crossplatform threading class, that provides standard synchronization methods
		as well as safe RAII design.
 
		target function signature is CThread::func_sig

*************************************************************************************/


#ifndef _CTHREAD_H

	#define _CTHREAD_H

	#include "PlatformSpecific.h"
	#define __threadh_win_call
	#ifdef __CPP11__
		#include <thread>
		typedef std::thread * Thread_t;
		typedef std::thread::native_handle_type ThreadHandle;
	#elif defined(__WINDOWS__)
		typedef HANDLE ThreadHandle;
		typedef ThreadHandle Thread_t;
		#undef __threadh_win_call
		#define __threadh_win_call CALLBACK
	#else
		typedef pthread_t ThreadHandle;
		typedef ThreadHandle Thread_t;
	#endif

	namespace APE
	{
		class CThread
		{
			typedef void * (__cdecl * func_sig)(void*);
			
			Thread_t thread;
			func_sig func;
			
			struct args
			{
				args(func_sig a1, void * a2)
					: addr(a1), arg(a2) {}
				func_sig addr;
				void * arg;
			};
			
		public:
			CThread(func_sig functionAddress)
				:	thread(NULL), func(functionAddress)
			{
				
				
			}
			~CThread()
			{
				detach();
			}
			ThreadHandle getHandle()
			{
				if (thread) {
					#ifdef __CPP11__
						return static_cast<ThreadHandle>(thread->native_handle());
					#else
						return static_cast<ThreadHandle>(thread);
					#endif
				}
				return static_cast<ThreadHandle>(NULL);
			}

			int join()
			{
				void * retval;
				if(!thread)
					return -1;
				int result = 0;
				#ifdef __CPP11__
					if (thread->joinable()) {
						thread->join();
					}
					else
						result = -2;
					retval = nullptr;
				#elif defined(__WINDOWS__)
					auto fret = WaitForSingleObject(thread, INFINITE);
					if (!fret) {
						result = GetLastError();
						return result;
					}
					DWORD ret;
					auto sret = GetExitCodeThread(thread, &ret);
					if (!sret) {
						result = GetLastError();
					}
					retval = static_cast<void*>(ret);
				#else
					result = pthread_join(thread, &retval);
				#endif
				return result;
			}
			void detach()
			{
				if(thread)
				{
					#ifdef __CPP11__
					if (thread->joinable())
						thread->detach();
						delete thread;
					#elif defined(__WINDOWS__)
						CloseHandle(thread);
					#else
						pthread_detach(thread);
					#endif
					thread = NULL;
				}
				
			}
			int run(void * argument = NULL)
			{
				if(!func)
					return -1;
				args * func_args = new args(func, argument);
				int result = 0;
				#ifdef __CPP11__
					thread = new std::thread(cpp11_target, func_args);
				#elif defined(__WINDOWS__)
					thread = CreateThread(NULL, NULL, NULL,
										win_target,
										reinterpret_cast<void*>(func_args),
										NULL);
					if(!thread)
						result = GetLastError();
				#else
					result = pthread_create(&thread, NULL, posix_target, reinterpret_cast<void*>(func_args));
				
				#endif
				return result;
			}
		private:
			static void * cpp11_target(args * func)
			{
				auto ret = func->addr(func->arg);
				delete func;
				return ret;
			}
			static unsigned long __threadh_win_call win_target(void * imp)
			{
				args * func = reinterpret_cast<args *>(imp);
				auto ret = func->addr(func->arg);
				delete func;
				return reinterpret_cast<unsigned long>(ret);
			}
			static void * posix_target(void * imp)
			{
				#ifndef __WINDOWS__
					args * func = reinterpret_cast<args *>(imp);
					auto ret = func->addr(func->arg);
					delete func;
					pthread_exit(ret);
				#endif
			}
		};
	}
#endif
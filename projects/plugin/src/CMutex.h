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

	file:CMutex.h
		
		Provides an interface for easily locking objects through RAII so long as
		they derive from CMutex::Lockable.
		Uses an special spinlock that yields the thread instead of busy-waiting.
		time-outs after a specified interval, and asks the user what to do, as well
		as providing debugger breakpoints in case of locks.

*************************************************************************************/

#ifndef _CMUTEX_H
	#define _CMUTEX_H

	#include "MacroConstants.h"
	#include "Misc.h"
	#ifndef __CPP11__
		#error "No mutex support!"
	#else
		#include <atomic>
	#endif
	namespace APE
	{

		class CMutex
		{
		public:
			class Lockable
			{
				friend class CMutex;
				std::atomic_flag flag;
			public:
				Lockable()
				{
					flag.clear();
				}
			};
		private:
			Lockable * resource;
		public:
			CMutex(Lockable * l)
			{
				resource = l;
				acquire(l);

			}
			CMutex(Lockable & l)
			{
				resource = &l;
				acquire(l);
			}
			CMutex()
				: resource(nullptr)
			{

			}
			~CMutex()
			{
				if (resource)
					release(resource);

			}
			void acquire(Lockable * l)
			{
				if (!resource)
					resource = l;
				if (!spinLock(2000, resource))
					//explode here
					return;
			}
			void acquire(Lockable & l)
			{
				acquire(&l);
			}
			void release()
			{
				
				if (resource)
					release(resource);
			}
		private:
			void release(Lockable * l)
			{
				if (l)
					l->flag.clear();
				resource = nullptr;

			}
			static bool spinLock(unsigned ms, Lockable *  bVal)
			{
				using namespace Misc;
				unsigned int start;
				int ret;
			loop:
				
				start = QuickTime();
				while (bVal->flag.test_and_set()) {
					if ((QuickTime() - start) > ms)
						goto time_out;
					Delay(0);
				}
				// normal exitpoint
				return true;
				// deadlock occurs

			time_out:
				// hello - you have reached this point if mutex was frozen.
				BreakIfDebugged();
				ret = Misc::MsgBox("Deadlock detected in spinlock: Protected resource is not released after max interval. "
					"Wait again (try again), release resource (continue) - can create async issues - or exit (cancel)?",
					_PROGRAM_NAME_ABRV " Error!",
					sConTryCancel | iStop,
					NULL,
					true);
				switch (ret)
				{
				case MsgButton::bTryAgain:
					goto loop;
				case MsgButton::bContinue:
					bVal->flag.clear(); // flip val
					// try again 
					goto loop;
				case MsgButton::bCancel:
					exit(-1);
				}
				// not needed (except for warns)
				return false;
			}
		};
	};
#endif
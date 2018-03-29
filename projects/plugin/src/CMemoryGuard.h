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

	file:CMemoryGuard.h
	
		An allocation service, that protects/guards the memory around the memory allocated
		from this object. Any access to memory returned by this function should be executed
		in a CState::RunProtectedCode<lambda> block, since access beyond the allocated memory raises an 
		segmentation fault exception. This should be handled, since the system
		is recoverable and well-defined from this situation.
 
	options:
		#define __CMEMORYGUARD_USE_CSTDLIB
			causes the header to uses non-implementation-specific services and rely
			on standard malloc/free. this will not provide protection/faults, but provided
			for compability

*************************************************************************************/

#ifndef _CMEMORYGUARD_H
	#define _CMEMORYGUARD_H

	#include <cpl/MacroConstants.h>
	#ifdef __WINDOWS__
		#include <Windows.h>
		typedef DWORD prot_t;
	#elif defined(__MAC__)
		#include <unistd.h>
		#include <sys/mman.h>
		typedef int prot_t;
	#else
		#error "Implement for your system."
	#endif

	#include <cmath>

	namespace ape 
	{
		class CMemoryGuard
		{
		private:
			std::size_t pageSize; // size of a page on this current system
			std::size_t bankSize; // the actual memory request size, rounded up to nearest page
			std::size_t totalAllocation; // total allocation done
			char * regions [3]; // [0] = protected page before, [1] = usable memory with requested protected, [2] = protected page after
			prot_t dwProtection; // protection of regions[1]
			int lastErr; // potention last error of operation, not used yet
			void setPageSize() {
				#ifdef __WINDOWS__
					SYSTEM_INFO info;
					GetSystemInfo(&info);
					pageSize = info.dwPageSize;
				#elif defined (__MAC__)
					pageSize = getpagesize();
				#endif
			}
		public:

			enum protection : prot_t {
				#ifdef __WINDOWS__
					readonly = PAGE_READONLY,
					readwrite = PAGE_READWRITE,
					execute = PAGE_EXECUTE,
					readwriteexecute = PAGE_EXECUTE_READWRITE,
					none = PAGE_NOACCESS
				#elif defined (__MAC__)
					readonly = PROT_READ,
					readwrite = PROT_WRITE | readonly,
					execute = PROT_EXEC,
					readwriteexecute = PROT_EXEC | readwrite,
					none = PROT_NONE
				#else
					#error "Implement for your system"
				#endif
			};

			CMemoryGuard()
			: pageSize(0), bankSize(0), dwProtection(readwrite), totalAllocation(0)
			{
				regions[0] = regions[1] = regions[2] = nullptr;
				setPageSize();
			}
			~CMemoryGuard() {
				reset();
			}
			/*
				unallocated any memory allocated. invalidates get().
				automatically called on destruction
			*/
			bool reset() {
				if(regions[0])
					#if defined(__CMEMORYGUARD_USE_CSTDLIB)
						std::free(regions[0]);
					#elif defined(__WINDOWS__)
						if(!VirtualFree(regions[0], 0, MEM_RELEASE))
							return false;
					#elif defined(__MAC__)
						if(munmap(regions[0], totalAllocation))
							return false;
					#endif
				regions[0] = regions[1] = regions[2] = nullptr;
				totalAllocation = 0;
				return true;
			}
			/*
				changes the internal protection used, and currently allocated usable memory (if any)
			*/
			bool setProtect(protection _dwProtection) {
				#ifndef __CMEMORYGUARD_USE_CSTDLIB
					dwProtection = _dwProtection;
					if(regions[0]) {
						#ifdef __WINDOWS__
							prot_t dwOldProtect;
							if(!VirtualProtect(regions[1], bankSize, dwProtection, &dwOldProtect)) {
						#elif defined(__MAC__)
							if(mprotect(regions[1], bankSize, dwProtection)) {
						#endif
							reset();
							return false;
						}			
					}
				#endif
				return true;
			}

			/// <summary>
			/// if size > getMaxSize() it attemps to resize the allocation.
			/// this causes all memory to be forgotten.
			///	automatically guards pages before and after with no - access protection.
			///	this will cause segfaults if this memory is accessed.
			///	returns true if sucessful, false if error occured.
			///	the usable memory is protected with default (readwrite) or whatever setProtect has
			///	been called with.
			/// </summary>
			bool resize(size_t size) 
			{
				if(size < bankSize)
					return true;

				if(!reset())
					return false;

				size_t needed_memory = ((size_t)std::ceil(float(size) / pageSize)) * pageSize;
				bankSize = needed_memory; // set bankSize to the size of the usable memory.
				needed_memory += 2 * pageSize; // add a page before and after
				
				regions[0] = reinterpret_cast<char*>
				(
					#if defined(__CMEMORYGUARD_USE_CSTDLIB)
						std::malloc(needed_memory)
					#elif defined(__WINDOWS__)
						VirtualAlloc(nullptr,
						needed_memory, 
						MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
					#elif defined(__MAC__)
						mmap(nullptr, needed_memory, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0)
						//(std::malloc(needed_memory));
					#endif
				);
				lastErr = errno;
				if(!regions[0])
					return false;
				regions[1] = regions[0] + pageSize;
				regions[2] = regions[1] + bankSize;
				totalAllocation = needed_memory;
				#ifdef __WINDOWS__
					prot_t dwOldProtect;
					if(!VirtualProtect(regions[0], pageSize, none, &dwOldProtect)) {
						reset();
						return false;
					}
					if(size && !VirtualProtect(regions[1], bankSize, dwProtection, &dwOldProtect)) {
						reset();
						return false;
					}
					if(!VirtualProtect(regions[2], pageSize, none, &dwOldProtect)) {
						reset();
						return false;
					}
				#elif defined(__MAC__)
					if(mprotect(regions[0], pageSize, none) ||
					   mprotect(regions[1], bankSize, dwProtection) ||
					   mprotect(regions[2], pageSize, none)
					)
					{
						reset();
						return false;
					}
				#endif
				return true;
			}

			template<typename T>
			bool resize(std::size_t elements)
			{
				return resize(elements * sizeof(T));
			}

			/*
				checks whether a pointer points to memory inside this object's allocated memory.
					return values are -1 if it points to protected page before,
					2 if it points to protected page after,
					1 if it points to the actual valid memory.
					0 if it doesn't point to anything this object knows of.
			*/
			int in_range(const void * ptr) noexcept 
			{
				if(ptr > regions[0] && ptr < regions[1])
					return -1;
				if(ptr > regions[1] && ptr < regions[2])
					return 1;
				if(ptr > regions[2] && ptr < regions[2] + pageSize)
					return 2;
				return 0;

			}
			/*
				returns the valid usable memory. guaranteed to be valid if resize() returned true.
				returns nullptr if resize() error'd or hasn't been called.
			*/
			void * get() noexcept
			{
				return regions[1] ? regions[1] : nullptr;
			}

			template<typename T>
			T* get() noexcept
			{
				return reinterpret_cast<T*>(get());
			}

			/*
				returns the maximum size of the valid memory
				this implies that get() + 0 to get() + getMaxSize() - 1 is valid memory
			*/
			size_t getMaxSize() 
			{
				return bankSize;
			}
		};
	};
#endif
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

	file:CAllocator.h
	
		Interface for an utility memory allocator class which supports RAII (allocations 
		free'd on destruction).
		All memory is zeroinitialized and contains corrupted memory checks.

*************************************************************************************/

#ifndef _CALLOCATOR_H

	#define _CALLOCATOR_H
	#define CALLOCATOR_CATCH_DOUBLEFREE

	#include <list>
	#include <cstdlib>
	#include <cpl/MacroConstants.h>
	#include <ape/SharedInterface.h>
	#include <cpl/CMutex.h>
	#include <set>
	
	namespace ape 
	{

		class CAllocator
		{
		public:
			using Label = APE_AllocationLabel;

		public:
			CAllocator(unsigned align);
			void * alloc(Label l, size_t memsize);
			
			template <typename T> 
				T * alloc(Label l)
				{
					return reinterpret_cast<T*> (this->alloc(l, sizeof (T)));
				}


			void free(void * block);
			void clear();

			~CAllocator();

		private:

			static const unsigned int end_marker = 0xBADC0DE; 

			unsigned alignment;
			struct mem_block;
			std::list<mem_block*> allocations;

			typedef std::list<mem_block*>::iterator mem_it;

			struct mem_block
			{
				struct mem_end
				{
					unsigned int ncheck;
				};

				union
				{
					struct
					{
						// sizeof start of header + memoryblock + mem_end
						std::size_t totalSize;
						mem_end * end;
						Label memLabel;
					};

					std::byte align[32];
				};


				// held memory is after this block, so we add the size of this block to it's pointer.
				void * getMemory() { return reinterpret_cast<char*>(this) + sizeof(*this); }
			};

			mem_block * headerFromBlock(void * block);
			bool removeBlock(mem_block * header);
			cpl::CMutex::Lockable mutex;
		};

	};
#endif
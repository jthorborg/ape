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

	file:CAllocator.cpp
	
		Implementation of allocator class.

*************************************************************************************/

#include "CAllocator.h"
#include <cpl/Protected.h>
#include <cpl/Misc.h>

namespace ape 
{

	CAllocator::CAllocator(unsigned align) : alignment(align) {};

	void * CAllocator::alloc(CAllocator::Label l, std::size_t memsize)
	{
		// calculate needed size.
		size_t size = sizeof(mem_block) + memsize + sizeof(mem_block::mem_end);
		// calculate if it's unaligned
		unsigned rem = size % alignment;
		// check remainder, and add it if needed.
		if(rem) {
			memsize += rem;
			size = sizeof(mem_block) + memsize + sizeof(mem_block::mem_end);
		}
		// allocate the memory
		void * m = cpl::Misc::alignedMalloc<std::byte>(size);
		// crash here perhaps.
		if(!m)
			return nullptr;
		// zero out memory
		memset(m, 0, size);
		mem_block * header = reinterpret_cast<mem_block *>(m);
		header->memLabel = l;
		header->totalSize = size;
		header->end = reinterpret_cast<mem_block::mem_end *>
			(		// end = start + size - size of end header
				(reinterpret_cast<char*>(m) + size) - sizeof(mem_block::mem_end)
			);
		// set a unique value in the end to so we can check if memory has been overwritten.
		header->end->ncheck = end_marker;


		cpl::CMutex lock(mutex);
		allocations.push_back(header);
		return header->getMemory();
	}

	CAllocator::mem_block * CAllocator::headerFromBlock(void * block)
	{
		if (!block)
			return nullptr;
		
		mem_block * header = reinterpret_cast<mem_block *> 
			(	// the header is located before the actual block of memory, so subtract that value.
				reinterpret_cast<char*>(block) - sizeof(mem_block)
			);

		if(header->end && header->end->ncheck != end_marker)
			// either we didnt allocate this or the block is corrupted: return null
			return nullptr;

		return header;
	}

	bool CAllocator::removeBlock(mem_block * header)
	{
		mem_it ret_it;
		bool found(false);

		for(ret_it = allocations.begin(); ret_it != allocations.end(); ret_it++) 
		{

			if((*ret_it) == header) 
			{
				found = true;
				break;
			}
		}

		if(found)
			allocations.erase(ret_it);

		return found;
	}

	void CAllocator::free(void * block) 
	{
		if(!block)
			return;

		cpl::CMutex lock (mutex);

		mem_block * header = headerFromBlock(block);

		if (!header)
			cpl::CProtected::instance().throwException<std::runtime_error>("double free of pointer: " + std::to_string((std::uintptr_t)block));

		removeBlock(header);
		cpl::Misc::alignedFree(header);
	}


	void CAllocator::clear()
	{
		cpl::CMutex lock (mutex);
		for(auto it = allocations.begin(); it != allocations.end(); it++)
		{
			cpl::Misc::alignedFree(*it);
		}
		allocations.clear();
	}

	CAllocator::~CAllocator()
	{
		clear();
	}
};
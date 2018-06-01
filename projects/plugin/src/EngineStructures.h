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

	file:EngineStructures.h
		
		Auxillary data structures for the engine

*************************************************************************************/

#ifndef APE_ENGINESTRUCTURES_H
	#define APE_ENGINESTRUCTURES_H

	#include <vector>
	#include <string>
	#include <algorithm>
	
	namespace ape 
	{

		class AuxMatrix
		{
		public:

			void resizeChannels(std::size_t length)
			{
				auxBuffers.resize(length);
			}

			void softBufferResize(std::size_t length)
			{
				auto newSize = length * auxBuffers.size();
				auxData.resize(std::max(newSize, auxData.size()));

				for (std::size_t i = 0; i < auxBuffers.size(); ++i)
					auxBuffers[i] = auxData.data() + length * i;
					
				bufferLength = length;
			}

			void copy(const float** buffers, std::size_t index, std::size_t numBuffers)
			{
				for (std::size_t i = 0; i < numBuffers; ++i)
					std::memcpy(auxBuffers[i + index], buffers[i], bufferLength * sizeof(float));
			}

			float* operator [] (std::size_t index) const { return auxBuffers[index]; }
			float** data() noexcept { return auxBuffers.data(); }
			std::size_t size() const noexcept { return auxBuffers.size(); }

		private:
			std::size_t bufferLength = 0;
			std::vector<float> auxData;
			std::vector<float*> auxBuffers;
		};

		class ChannelNamePool
		{
		public:

			ChannelNamePool(std::size_t initialCount, std::size_t bufferLength)
			{
				for (std::size_t i = 0; i < initialCount; ++i)
					names.emplace_back(bufferLength, '0');
			}

			std::string&& dequeue() 
			{ 
				auto&& s = std::move(names.back()); 
				names.pop_back(); 
				return std::move(s); 
			}

			void enqueue(std::string&& s)
			{
				names.emplace_back(std::move(s));
			}

		private:
			std::vector<std::string> names;
		};
	}
#endif
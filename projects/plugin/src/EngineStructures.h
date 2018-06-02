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
				{
					std::memcpy(auxBuffers[i + index], buffers[i], bufferLength * sizeof(float));
				}
			}

			void copyResample(const float* buffer, std::size_t index, std::size_t numSamples)
			{
				if (numSamples == bufferLength)
					return copy(&buffer, index, 1);
				
				if (numSamples > bufferLength)
				{

				}
				else if (numSamples < bufferLength)
				{

				}
				
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
					names.emplace_back(bufferLength, '\0');
			}

			std::string dequeue()
			{
				auto s = std::move(names.back());
				names.pop_back();
				return std::move(s);
			}

			void enqueue(std::string&& s)
			{
				names.emplace_back(std::move(s));
			}

			std::string& operator [] (std::size_t index) { return names[names.size() - (1 + index)]; }

		private:
			std::vector<std::string> names;
		};

		class TracerState
		{
			static constexpr std::size_t traceCap = 10;

		public:

			TracerState()
				: pool(traceCap, 100)
			{

			}

			void beginPhase(AuxMatrix* matrixToUse, std::size_t offset)
			{
				matrix = matrixToUse;
				matrixOffset = offset;
				traceCounter = 0;
			}

			void endPhase()
			{
				firstPhase = false;
				matrix = nullptr;
				matrixOffset = 0;
			}

			void handleTrace(const char** nameTuple, std::size_t numNames, const float* vals, const std::size_t numValues)
			{
				if (traceCounter >= traceCap || !matrix)
					return;

				if (firstPhase)
				{
					std::size_t c = 0;
					auto& str = pool[numTraces];

					for (std::size_t i = 0; i < numNames; ++i)
					{
						std::size_t k = 0;
						for (; (k + c) < str.size() && nameTuple[i][k]; ++k)
							str[k + c] = nameTuple[i][k];

						c += k;

						if (i + 1 < numNames)
						{
							if (c + 4 >= str.size())
								break;

							str[c++] = ' ';
							str[c++] = '-';
							str[c++] = '>';
							str[c++] = ' ';
						}


					}

					numTraces++;
				}

				if (traceCounter + matrixOffset >= matrix->size())
					return;

				matrix->copyResample(vals, traceCounter + matrixOffset, numValues);

				traceCounter++;
			}

			bool changesPending()
			{
				return firstPhase;
			}

			std::size_t getTraceCount() const noexcept
			{
				return numTraces;
			}

			std::string dequeueName()
			{
				return std::move(pool.dequeue());
			}

		private:

			AuxMatrix* matrix = nullptr;
			ChannelNamePool pool;
			std::size_t numTraces = 0, traceCounter = 0, matrixOffset = 0;
			bool firstPhase = true;
		};
	}
#endif
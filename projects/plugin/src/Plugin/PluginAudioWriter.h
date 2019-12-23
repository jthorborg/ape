/*************************************************************************************

	Audio Programming Environment VST. 
		
    Copyright (C) 2018 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:PluginAudioWriter.h
		
		Represents a async SPSC streamed audio to disk protocol

*************************************************************************************/

#ifndef APE_PLUGINAUDIOWRITER_H
	#define APE_PLUGINAUDIOWRITER_H

	#include <ape/SharedInterface.h>
	#include <string>
	#include <vector>
	#include <memory>
	#include <cpl/Common.h>
	
	namespace ape 
	{
		class PluginStreamProducer : public juce::AudioFormatWriter::ThreadedWriter
		{
			friend class OutputFileManager;
		public:

			static constexpr std::size_t kBufferSize = 1 << 16;

			using juce::AudioFormatWriter::ThreadedWriter::ThreadedWriter;

			bool writeAsync(std::uint64_t samples, const float * const * data);
		};

		class OutputFileManager : private juce::TimeSliceThread
		{
		public:

			static std::unique_ptr<PluginStreamProducer> createProducer(juce::File path, juce::AudioFormat& format, double sampleRate, int channels, int bits, int quality);

			static juce::AudioFormat* selectFormatFor(juce::File path);

		private:

			OutputFileManager();

			static OutputFileManager& getInstance();
			juce::AudioFormatManager formatManager;
		};
	}
#endif
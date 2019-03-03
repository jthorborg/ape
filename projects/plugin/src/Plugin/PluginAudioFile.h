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

	file:PluginAudioFile.h
		
		Represents a synchronously loaded audio file

*************************************************************************************/

#ifndef APE_PLUGINAUDIOFILE_H
	#define APE_PLUGINAUDIOFILE_H

	#include <ape/SharedInterface.h>
	#include <string>
	#include <vector>
	#include <memory>
	#include <cpl/Common.h>
	
	namespace ape 
	{

		class PluginAudioFile
		{
		public:

			PluginAudioFile(juce::File path);
			PluginAudioFile(const PluginAudioFile& other, double targetSampleRate);

			APE_AudioFile getAudioFile() const noexcept;

		private:

			static void resampleFrom(const PluginAudioFile& source, PluginAudioFile& dest, double newSampleRate);
			void readFormat(juce::AudioFormatReader& reader);
			void convertToFloatingPoint();

			std::uint64_t samples;
			std::uint32_t channels;
			double sampleRate;
			std::string name;
			std::vector<float*> columns;
			std::vector<float> storage;
		};

	}
#endif
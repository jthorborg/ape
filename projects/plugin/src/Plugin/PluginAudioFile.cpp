/*************************************************************************************

	Audio Programming Environment VST. 
		
    Copyright (C) 2019 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:PluginAudioFile.cpp
		
		Implemention of a plugin audio file

*************************************************************************************/

#include "PluginAudioFile.h"
#include <cpl/Common.h>
#include <cpl/Exceptions.h>

namespace ape
{
	struct CodecTable
	{
		juce::AudioFormatManager manager;

		CodecTable() 
		{
			manager.registerBasicFormats();
		}
	};

	static CodecTable codecTable;

	PluginAudioFile::PluginAudioFile(juce::File file)
		: name(file.getFileName().toStdString())
	{
		if (!file.existsAsFile())
			CPL_RUNTIME_EXCEPTION("File not found: " + file.getFullPathName().toStdString());

		std::unique_ptr<juce::AudioFormatReader> reader(codecTable.manager.createReaderFor(file));

		if (reader)
			readFormat(*reader);
		else
			CPL_RUNTIME_EXCEPTION("No available codecs for: " + file.getFullPathName().toStdString());
	}

	APE_AudioFile PluginAudioFile::getAudioFile() const noexcept
	{
		APE_AudioFile ret;

		ret.channels = channels;
		ret.data = columns.data();
		ret.name = name.c_str();
		ret.sampleRate = sampleRate;
		ret.samples = samples;

		return ret;
	}


	void PluginAudioFile::readFormat(juce::AudioFormatReader& reader)
	{
		channels = reader.numChannels;
		samples = reader.lengthInSamples;
		sampleRate = reader.sampleRate;

		storage.resize(channels * samples);
		columns.resize(channels);

		for (std::size_t i = 0; i < channels; ++i)
			columns[i] = storage.data() + i * samples;

		reader.read(reinterpret_cast<int* const*>(columns.data()), channels, 0, samples, true);

		if (!reader.usesFloatingPointData)
			convertToFloatingPoint();
	}

	void PluginAudioFile::convertToFloatingPoint()
	{
		std::int32_t intRepresentation;

		for (std::size_t i = 0; i < storage.size(); ++i)
		{
			std::memcpy(&intRepresentation, storage.data() + i, sizeof(float));
			storage[i] = (1.0 / (1.0 + 0x7FFFFFFF)) * intRepresentation;
		}
	}

}
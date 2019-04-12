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
#include <cmath>

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

	PluginAudioFile::PluginAudioFile(const PluginAudioFile& other, double targetSampleRate)
	{
		sampleRate = other.sampleRate;
		name = other.name;
		channels = other.channels;
		samples = other.samples;

		resampleFrom(other, *this, targetSampleRate);
	}

	APE_AudioFile PluginAudioFile::getAudioFile() const noexcept
	{
		APE_AudioFile ret;

		ret.channels = channels;
		ret.data = columns.data();
		ret.name = name.c_str();
		ret.sampleRate = sampleRate;
		ret.fractionalLength = fractionalLength;
		ret.samples = samples;

		return ret;
	}


	void PluginAudioFile::readFormat(juce::AudioFormatReader& reader)
	{
		channels = reader.numChannels;
		samples = reader.lengthInSamples;
		sampleRate = reader.sampleRate;
		fractionalLength = samples;

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

	// laurent de soras
	template<typename T>
	inline const T hermite4(const T offset, const T ym1, const T y0, const T y1, const T y2)
	{
		const T c = (y1 - ym1) * static_cast<T>(0.5);
		const T v = y0 - y1;
		const T w = c + v;
		const T a = w + v + (y2 - y0) * static_cast<T>(0.5);
		const T b_neg = w + a;

		return ((((a * offset) - b_neg) * offset + c) * offset + y0);
	}

	void PluginAudioFile::resampleFrom(const PluginAudioFile& source, PluginAudioFile& dest, double newSampleRate)
	{
		CPL_RUNTIME_ASSERTION(newSampleRate > 0);
		CPL_RUNTIME_ASSERTION(source.sampleRate != newSampleRate);

		const auto inverseRatio = source.sampleRate / newSampleRate;

		dest.sampleRate = newSampleRate;
		dest.fractionalLength = source.samples * newSampleRate / source.sampleRate;
		dest.samples = 0;

		if (source.samples == 0)
			return;

		const auto newSampleCount = static_cast<std::uint64_t>(std::ceil(source.samples * (newSampleRate / source.sampleRate)));

		std::vector<float> newSamples(newSampleCount * source.channels);
		std::vector<float*> newChannels(source.channels);

		const auto oldSamples = source.samples;
		const auto channels = source.channels;

		for (std::size_t c = 0; c < channels; ++c)
			newChannels[c] = newSamples.data() + c * newSampleCount;


		for (std::uint64_t n = 0; n < newSampleCount; ++n)
		{
			const auto x = n * inverseRatio;
			std::uint64_t x0 = static_cast<std::uint64_t>(x);

			float ym1, y0, y1, y2;

			for (std::size_t c = 0; c < channels; ++c)
			{
				ym1 = x0 > 0 && (x0 - 1) < oldSamples ? source.columns[c][x0 - 1] : 0.0f;
				y0 = x0 < oldSamples ? source.columns[c][x0] : 0.0f;
				y1 = (x0 + 1) < oldSamples ? source.columns[c][x0 + 1] : 0.0f;
				y2 = (x0 + 2) < oldSamples ? source.columns[c][x0 + 2] : 0.0f;

				newChannels[c][n] = hermite4(static_cast<float>(x - x0), ym1, y0, y1, y2);
			}
		}

		dest.columns = std::move(newChannels);
		dest.storage = std::move(newSamples);

		dest.samples = newSampleCount;
	}

}
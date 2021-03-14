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

	file:PluginAudioWriter.cpp
		
		Implemention of a plugin audio writer system

*************************************************************************************/

#include "PluginAudioWriter.h"
#include <cpl/Mathext.h>
#include "../Engine.h"
#include "../UIController.h"
#include "../CConsole.h"

namespace ape
{
	std::unique_ptr<PluginStreamProducer> OutputFileManager::createProducer(juce::File path, juce::AudioFormat& format, double sampleRate, int channels, int bits, int quality)
	{
		auto& instance = getInstance();

		if (auto stream = path.createOutputStream())
		{
			if (auto writer = format.createWriterFor(stream, sampleRate, channels, bits, juce::StringPairArray(), quality))
			{
				return std::make_unique<PluginStreamProducer>(writer, static_cast<juce::TimeSliceThread&>(instance), static_cast<int>(PluginStreamProducer::kBufferSize));
			}
			else
			{
				delete stream;
			}
		}

		CPL_RUNTIME_EXCEPTION("Couldn't create audio output file at " + path.getFullPathName().toStdString());
	}

	bool PluginStreamProducer::writeAsync(std::uint64_t samples, const float * const * data)
	{
		return write(data, samples);
	}

	juce::AudioFormat * OutputFileManager::selectFormatFor(juce::File path)
	{
		return getInstance().formatManager.findFormatForFileExtension(path.getFileExtension());
	}

	OutputFileManager::OutputFileManager()
		: juce::TimeSliceThread("Async audio file thread")
	{
		startThread();
		formatManager.registerBasicFormats();
	}

	OutputFileManager & OutputFileManager::getInstance()
	{
		static OutputFileManager manager;

		return manager;
	}
}
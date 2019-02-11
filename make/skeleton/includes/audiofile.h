#ifndef CPPAPE_AUDIOFILE_H
#define CPPAPE_AUDIOFILE_H

#include "baselib.h"
#include "misc.h"

namespace ape
{
	class AudioFile
	{
	public:

		AudioFile(const char* relativePath) : AudioFile(loadFile(relativePath))	{ }


		const float* getChannel(std::size_t i) const
		{
			return samples.data[i];
		}

		explicit operator bool() const noexcept { return loadedOk && samples.data && samples.count; }

		const SampleMatrix<const float> samples;
		const bool loadedOk;
		const double sampleRate;
		const char* const name;

	private:

		static APE_AudioFile loadFile(const char* relativePath)
		{
			APE_AudioFile file;
			getInterface().loadAudioFile(&getInterface(), relativePath, &file);

			return file;
		}

		AudioFile(const APE_AudioFile& audioFile)
			: samples{audioFile.data, audioFile.samples, audioFile.channels}
			, loadedOk(audioFile.name)
			, sampleRate(audioFile.sampleRate)
			, name(audioFile.name)
		{

		}
	};

}

#endif
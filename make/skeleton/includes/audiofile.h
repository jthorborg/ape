#ifndef CPPAPE_AUDIOFILE_H
#define CPPAPE_AUDIOFILE_H

#include "baselib.h"
#include "misc.h"

namespace ape
{
	class AudioFile : public const_umatrix<const float>
	{
	public:

		using const_umatrix<const float>::data;
		using const_umatrix<const float>::samples;

		AudioFile(const char* relativePath) : AudioFile(loadFile(relativePath))	{ }

		explicit operator bool() const noexcept { return loadedOk && data && samples(); }

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
			: const_umatrix<const float>{audioFile.data, audioFile.samples, audioFile.channels}
			, loadedOk(audioFile.name)
			, sampleRate(audioFile.sampleRate)
			, name(audioFile.name)
		{

		}
	};

}

#endif
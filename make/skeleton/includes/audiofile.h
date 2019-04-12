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

		explicit operator bool() const noexcept { return loadedOk() && data && samples(); }

		bool loadedOk() const noexcept { return fileLoadedOk; }
		double sampleRate() const noexcept { return fileSampleRate; }
		const char* name() const noexcept { return fileName; }

	protected:

		static APE_AudioFile loadFile(const char* relativePath, double sampleRate = APE_SampleRate_Retain)
		{
			APE_AudioFile file;
			getInterface().loadAudioFile(&getInterface(), relativePath, sampleRate, &file);

			return file;
		}

		AudioFile(const APE_AudioFile& audioFile)
			: const_umatrix<const float>{audioFile.data, audioFile.samples, audioFile.channels}
			, fileLoadedOk(audioFile.name)
			, fileSampleRate(audioFile.sampleRate)
			, fileName(audioFile.name)
			, preciseLength(audioFile.fractionalLength)
		{

		}

	protected:

		double preciseLength;

	private:

		bool fileLoadedOk;
		double fileSampleRate;
		const char* fileName;
	};

	class ResampledAudioFile : public AudioFile
	{
		using AudioFile::loadFile;
	public:

		constexpr static double AdoptProjectRate = APE_SampleRate_Adopt;
		constexpr static double OriginalSampleRate = APE_SampleRate_Retain;

		ResampledAudioFile(const char* relativePath, double targetSampleRate = AdoptProjectRate) : AudioFile(loadFile(relativePath, targetSampleRate)) { }

		double fractionalLength() const noexcept { return this->preciseLength; }
	};

}

#endif
#ifndef CPPAPE_AUDIOFILE_H
#define CPPAPE_AUDIOFILE_H

#include "baselib.h"
#include "misc.h"

namespace ape
{
	/// <summary>
	/// Represents a loaded audio file from disk, as a <see cref="umatrix"/>
	/// </summary>
	class AudioFile : public umatrix<const float>
	{
	public:

		using umatrix<const float>::data;
		using umatrix<const float>::samples;

		/// <summary>
		/// Load the file into memory, entirely.
		/// </summary>
		/// <param name="relativePath">
		/// The path is taken to be relative to the directory the current script is located in
		/// </param>
		AudioFile(const char* relativePath) : AudioFile(loadFile(relativePath))	{ }

		/// <summary>
		/// Test whether this contains a valid audio file with valid audio data
		/// <seealso cref="loadedOk"/>
		/// </summary>
		explicit operator bool() const noexcept { return loadedOk() && data && samples(); }

		/// <summary>
		/// Test whether the file was technically loaded okay.
		/// </summary>
		bool loadedOk() const noexcept { return fileLoadedOk; }
		/// <summary>
		/// Returns the original sample rate of this file
		/// </summary>
		double sampleRate() const noexcept { return fileSampleRate; }
		/// <summary>
		/// Returns the local file name of this file
		/// </summary>
		const char* name() const noexcept { return fileName; }

	protected:

		static APE_AudioFile loadFile(const char* relativePath, double sampleRate = APE_SampleRate_Retain)
		{
			APE_AudioFile file;
			getInterface().loadAudioFile(&getInterface(), relativePath, sampleRate, &file);

			return file;
		}

		AudioFile(const APE_AudioFile& audioFile)
			: umatrix<const float>{audioFile.data, audioFile.channels, audioFile.samples}
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

	/// <summary>
	/// An <see cref="AudioFile"/> that can be automatically resampled to the project sample rate.
	/// </summary>
	class ResampledAudioFile : public AudioFile
	{
		using AudioFile::loadFile;
	public:

		/// <summary>
		/// Resample this file to the project sample rate
		/// </summary>
		constexpr static double AdoptProjectRate = APE_SampleRate_Adopt;
		/// <summary>
		/// Retain this file in it's original sample rate.
		/// Equivalent to a normal <see cref="AudioFile"/>
		/// </summary>
		constexpr static double OriginalSampleRate = APE_SampleRate_Retain;

		/// <summary>
		/// Load a resampled audio file.
		/// </summary>
		/// <param name="relativePath">
		/// The relative path to the file to load
		/// </param>
		/// <param name="targetSampleRate">
		/// The sample rate to resample to. Special values are also available:
		/// <see cref="AdoptProjectRate"/> and <see cref="OriginalSampleRate"/>
		/// </param>
		ResampledAudioFile(const char* relativePath, double targetSampleRate = AdoptProjectRate) : AudioFile(loadFile(relativePath, targetSampleRate)) { }

		/// <summary>
		/// Length in fractional samples (if resampled)
		/// </summary>
		double fractionalLength() const noexcept { return this->preciseLength; }
	};

}

#endif

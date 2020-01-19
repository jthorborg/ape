#ifndef CPPAPE_OUTPUTFILE_H
#define CPPAPE_OUTPUTFILE_H

#include "baselib.h"
#include "misc.h"
#include "processor.h"

namespace ape
{
	/// <summary>
	/// Provides capability to record streamed audio asynchronously to a file on disk.
	/// </summary>
	class OutputAudioFile
	{
	public:

		/// <summary>
		/// For non-compressed formats supporting it, provides a choice of bitdepths
		/// </summary>
		enum class BitDepth
		{
			Int8 = 8, 
			Int16 = 16,
			Int24 = 24,
			Float32 = 32
		};

		/// <summary>
		/// Default-initialize an output file.
		/// </summary>
		OutputAudioFile() : fd(0), channels(0) {}

		/// <summary>
		/// Create an output file ready to be streamed.
		/// </summary>
		/// <param name="whereWithExtension">
		/// Relative to your script's location.
		/// If the file already exists, it will be renamed with a numbered suffix (up to 1000)
		/// 
		/// Supported extensions depend on platform, but following are always supported:
		/// - .wav
		/// - .flac
		/// - .aiff
		/// - .ogg
		/// 
		/// Depending on platform, these may be available:
		/// - .mp3
		/// - .wma
		/// - .caf
		/// </param>
		/// <param name="numChannels">
		/// How many channels in the recorded audio file
		/// </param>
		/// <param name="sampleRate">
		/// The sample rate of the recorded file.
		/// </param>
		/// <param name="bitsPerSample">
		/// Choice of bitdepth in the recorded file.
		/// </param>
		/// <param name="quality">
		/// For compressed formats, indicates a quality between 0 (worst) and 1 (best).
		/// </param>
		OutputAudioFile(const char* whereWithExtension, int numChannels, double sampleRate, BitDepth bitsPerSample = BitDepth::Int24, float quality = 1)
		{
			channels = numChannels;

			fd = getInterface().createAudioOutputFile(
				&getInterface(), 
				whereWithExtension,
				sampleRate,
				numChannels,
				(int)bitsPerSample,
				quality
			);

			if (!fd)
				abort("Couldn't create audio file");
		}

		/// <summary>
		/// Create an output file ready to be streamed.
		/// </summary>
		/// <param name="whereWithExtension">
		/// Relative to your script's location.
		/// If the file already exists, it will be renamed with a numbered suffix (up to 1000)
		/// 
		/// Supported extensions depend on platform, but following are always supported:
		/// - .wav
		/// - .flac
		/// - .aiff
		/// - .ogg
		/// 
		/// Depending on platform, these may be available:
		/// - .mp3
		/// - .wma
		/// - .caf
		/// </param>
		/// <param name="cfg">
		/// Use <see cref="IOConfig::sampleRate"/> and <see cref="IOConfig::outputs"/> for channels
		/// </param>
		/// <param name="bitsPerSample">
		/// Choice of bitdepth in the recorded file.
		/// </param>
		/// <param name="quality">
		/// For compressed formats, indicates a quality between 0 (worst) and 1 (best).
		/// </param>
		OutputAudioFile(const char* whereWithExtension, const IOConfig& cfg, BitDepth bitsPerSample = BitDepth::Int24, float quality = 1)
			: OutputAudioFile(whereWithExtension, cfg.outputs, cfg.sampleRate, bitsPerSample, quality)
		{

		}

		/// <summary>
		/// Move and take ownership of the other file
		/// </summary>
		OutputAudioFile(OutputAudioFile&& other)
			: fd(other.fd), channels(other.channels)
		{
			other.fd = 0;
			other.channels = 0;
		}

		/// <summary>
		/// Move-assign and take ownership of the other file
		/// </summary>
		OutputAudioFile& operator = (OutputAudioFile&& other)
		{
			if (fd)
			{
				getInterface().closeAudioFile(&getInterface(), fd);
			}

			fd = other.fd;
			channels = other.channels;

			other.fd = 0;
			other.channels = 0;

			return *this;
		}

		/// <summary>
		/// Stream the matrix to the file.
		/// </summary>
		void write(const umatrix<const float>& matrix)
		{
			if (!fd)
				abort("Audio file not initialized");

			if (channels != matrix.channels())
				abort("Mismatched channel count");

			write(matrix.samples(), matrix.pointers());
		}

		/// <summary>
		/// Stream a container supporting .size() and .data()
		/// </summary>
		/// <remarks>
		/// The container is assumed to only have one channel.
		/// </remarks>
		template<typename Container>
		auto write(const Container& c) -> decltype(c.size(), c.data(), void())
		{
			if (channels != 1)
				abort("Mismatched channel count");

			write(c.size(), c.data());
		}

		/// <summary>
		/// Stream a flat buffer.
		/// </summary>
		/// <remarks>
		/// Data is assumed to only have one channel.
		/// </remarks>
		void write(std::size_t numSamples, const float* data)
		{
			getInterface().writeAudioFile(&getInterface(), fd, (int)numSamples, &data);
		}

		/// <summary>
		/// Stream a flat rectangular buffer
		/// </summary>
		/// <remarks>
		/// Assumed to have as many channels as this file was initialized with.
		/// </remarks>
		void write(std::size_t numSamples, const float* const* data)
		{
			getInterface().writeAudioFile(&getInterface(), fd, (int)numSamples, data);
		}

		/// <summary>
		/// Close this file, if it hasn't been moved or properly initialized
		/// </summary>
		~OutputAudioFile()
		{
			if (fd)
				getInterface().closeAudioFile(&getInterface(), fd);
		}

	private:
		int fd;
		int channels;
	};

}

#endif

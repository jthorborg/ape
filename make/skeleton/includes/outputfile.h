#ifndef CPPAPE_OUTPUTFILE_H
#define CPPAPE_OUTPUTFILE_H

#include "baselib.h"
#include "misc.h"
#include "processor.h"

namespace ape
{
	class OutputAudioFile
	{
	public:

		OutputAudioFile() : fd(0), channels(0) {}

		OutputAudioFile(const char* whereWithExtension, int numChannels, double sampleRate, int bitsPerSample = 24, float quality = 1)
		{
			channels = numChannels;

			fd = getInterface().createAudioOutputFile(
				&getInterface(), 
				whereWithExtension,
				sampleRate,
				numChannels,
				bitsPerSample,
				quality
			);

			if (!fd)
				abort("Couldn't create audio file");
		}

		OutputAudioFile(const char* whereWithExtension, const IOConfig& cfg, int bitsPerSample = 24, float quality = 1)
			: OutputAudioFile(whereWithExtension, cfg.outputs, cfg.sampleRate, bitsPerSample, quality)
		{

		}

		OutputAudioFile(OutputAudioFile&& other)
			: fd(other.fd), channels(other.channels)
		{
			other.fd = 0;
			other.channels = 0;
		}

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

		void write(const const_umatrix<float>& matrix)
		{
			if (!fd)
				abort("Audio file not initialized");

			if (channels != matrix.channels())
				abort("Mismatched channel count");

			write(matrix.samples(), matrix.data);
		}

		template<typename Container>
		auto write(const Container& c) -> decltype(c.size(), c.data(), void())
		{
			if (channels != 1)
				abort("Mismatched channel count");

			write(c.size(), c.data());
		}

		void write(std::size_t numSamples, const float* data)
		{
			getInterface().writeAudioFile(&getInterface(), fd, (int)numSamples, &data);
		}

		void write(std::size_t numSamples, const float* const* data)
		{
			getInterface().writeAudioFile(&getInterface(), fd, (int)numSamples, data);
		}

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

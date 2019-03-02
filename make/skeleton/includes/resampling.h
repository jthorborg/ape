#ifndef CPPAPE_RESAMPLING_H
#define CPPAPE_RESAMPLING_H

#include "misc.h"
#include "audiofile.h"
#include <vector>
#include <memory>

namespace ape
{
	template<typename T>
	class RealSourceResampler
	{

		class Impl
		{
		public:

			virtual void produce(DynamicSampleMatrix<T>& output, std::size_t frames, double factor) = 0;
			virtual ~Impl() {}
		};

		class AFImpl : public Impl
		{
		public:

			AFImpl(const AudioFile& file)
				: source(file)
			{

			}

			void produce(DynamicSampleMatrix<T>& output, std::size_t frames, double factor) override
			{

				for (std::size_t f = 0; f < frames; ++f)
				{
					const auto x0 = static_cast<std::size_t>(position);
					auto x1 = x0 + 1;

					if (x1 >= source.samples())
						x1 -= source.samples();


					const auto weight = position - x0;

					for (std::size_t c = 0; c < source.channels(); ++c)
					{
						const auto y0 = source[c][x0];
						const auto y1 = source[c][x1];

						output.channels[c][f] = y0 * (1 - weight) + weight * y1;

					}

					position += factor;

					while (position >= source.samples())
						position -= source.samples();
				}
			}

			double position;
			const const_umatrix<const float> source;
		};

	public:

		RealSourceResampler(const AudioFile& file)
			: channels(file.channels())
		{
			impl = std::make_unique<AFImpl>(file);
		}


		const umatrix<T> produce(std::size_t frames, double factor = 1)
		{
			buffer.resize(channels, frames);
			impl->produce(buffer, frames, factor);
			return buffer;
		}

		template<typename Func>
		auto produce(std::size_t frames, Func && f)
		{
			const auto matrix = produce(frames);

			for (std::size_t frame = 0; frame < frames; ++frame)
			{
				for (std::size_t c = 0; c < channelBuffer.size(); ++c)
					channelBuffer[c] = matrix[c][frame];

				f(frame, channels, channelBuffer);
			}
		}
		
	private:
		std::size_t channels;
		std::vector<T> channelBuffer;
		DynamicSampleMatrix<T> buffer;
		std::unique_ptr<Impl> impl;
	};
}

#endif
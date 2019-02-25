#ifndef CPPAPE_MISC_H
#define CPPAPE_MISC_H

#include <cstdint>

namespace ape
{
	template<typename T>
	struct SampleMatrix
	{
		T* const* data;

		std::uint64_t count;
		std::uint32_t channels;
	};

	template<typename T>
	struct DynamicSampleMatrix
	{
		void resize(std::size_t channelCount, std::size_t samples)
		{
			channels.resize(channelCount);
			buffer.resize(channelCount * samples);

			for (std::size_t c = 0; c < channelCount; ++c)
				channels[c] = buffer.data() + c * samples * channelCount;
		}

		operator SampleMatrix<const T>() noexcept
		{
			return SampleMatrix<const T> { channels.data(), buffer.size() / channels.size(), channels.size() };
		}

		std::vector<T*> channels;
		std::vector<T> buffer;
	};
}

#endif
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
}

#endif
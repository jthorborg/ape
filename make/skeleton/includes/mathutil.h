#ifndef CPPAPE_MATHUTIL_H
#define CPPAPE_MATHUTIL_H

#include <cmath>
#include <algorithm>

namespace ape
{
    /// <summary>
    /// Step N towards <paramref name="size"/> given <paramref name="available"/> "samples",
    /// if at <paramref name="position"/> sample.
    /// This is a useful utility to count towards a task to be done every <paramref name="size"/> samples,
    /// given variably sized sample inputs.
    /// </summary>
	inline std::size_t clamp_available(std::size_t position, std::size_t size, std::size_t available)
	{
		return std::min(available, size - position);
	}

	inline size_t nextpow2(size_t current)
	{
		size_t p = 1;
		while (p < current)
			p <<= 1;
		return p;
	}
}

#endif
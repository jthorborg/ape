#ifndef CPPAPE_MATHUTIL_H
#define CPPAPE_MATHUTIL_H

#include <cmath>
#include <algorithm>

namespace ape
{
	inline std::size_t remainder(std::size_t where, std::size_t size, std::size_t input)
	{
		return std::min(input, size - where);
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
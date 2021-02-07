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

	/// <summary>
	/// Returns the next power of two, or equivalent to <paramref name="current"/>
	/// </summary>
	inline size_t nextpow2(size_t current)
	{
		size_t p = 1;
		while (p < current)
			p <<= 1;
		return p;
	}

	/// <summary>
	/// Returns the next power of two, above <paramref name="current"/>
	/// </summary>
	inline size_t nextpow2above(size_t current)
	{
		size_t p = 1;
		while (p <= current)
			p <<= 1;
		return p;
	}

	/// <summary>
	/// Tests whether <paramref name="t"/> is a power of two.
	/// </summary>
	template<typename UIntType>
	inline typename std::enable_if<std::is_unsigned<UIntType>::value, bool>::type ispow2(UIntType t)
	{
		return (t & (t - 1)) == 0;
	}
}

#endif
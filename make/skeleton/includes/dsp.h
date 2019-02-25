#ifndef CPPAPE_DSP_H
#define CPPAPE_DSP_H

#include <cmath>

namespace ape
{
	template<typename T>
	T lanczos(T x, int size)
	{
		constexpr T pi = static_cast<T>(M_PI);
		return x ? (size * std::sin(pi * x) * std::sin(pi * x / size)) / (pi * pi * x * x) : 1;
	}

	template<typename T>
	T sinc(T x)
	{
		constexpr T pi = static_cast<T>(M_PI);
		return x ? (std::sin(pi * x)) / (pi * x) : 1;
	}
}

#endif
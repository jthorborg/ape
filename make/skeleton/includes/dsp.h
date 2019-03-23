#ifndef CPPAPE_DSP_H
#define CPPAPE_DSP_H

#include <cmath>
#include <cstddef>
#include "misc.h"
#include <vector>
#include <complex>
#include <algorithm>

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

	template<typename T, typename Container>
	auto to_complex(const Container& c) -> decltype(std::begin(c), std::end(c), std::vector<std::complex<T>>())
	{
		return to_complex(c, std::distance(std::begin(c), std::end(c)));
	}

	template<typename T, typename Container>
	auto to_complex(const Container& c, std::size_t size) -> decltype(std::begin(c), std::end(c), std::vector<std::complex<T>>())
	{
		std::vector<std::complex<T>> ret;
		ret.reserve(size);

		std::size_t count = 0;

		for (auto it = std::begin(c); count < size && it != std::end(c); it++, count++)
		{
			ret.emplace_back(static_cast<T>(*it));
		}

		for (; count < size; ++count)
			ret.emplace_back(T());

		return std::move(ret);
	}
}

#endif
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

#if 0

	constexpr inline long double operator ""dB(long double arg)
	{
		return std::pow(10, arg / 20);
	}

	constexpr inline long double operator ""_dB(long double arg)
	{
		return std::pow(10, arg / 20);
	}

#endif

	/// <summary>
	/// Provides methods for converting back and forth from decibels
	/// </summary>
	class dB
	{
	public:
		/// <summary>
		/// Converts <paramref name="arg"/> as decibels to a scalar value
		/// </summary>
		template<typename T>
		static inline T from(T arg)
		{
			return std::pow(10, arg / 20);
		}

		/// <summary>
		/// Converts <paramref name="arg"/> to decibels
		/// </summary>
		template<typename T>
		static inline T to(T arg)
		{
			return 20 * std::log10(arg);
		}
	};

	/// <summary>
	/// Evaluates a lanczos kernel of size <paramref name="size"/> at <paramref name="x"/>
	/// </summary>
	/// <remarks>If <paramref name="x"/> is 0, the function returns 1.</remarks>
	template<typename T>
	T lanczos(T x, int size)
	{
		constexpr T pi = static_cast<T>(M_PI);
		return x ? (size * std::sin(pi * x) * std::sin(pi * x / size)) / (pi * pi * x * x) : 1;
	}

	/// <summary>
	/// Evaluates the sinc() function at x at <paramref name="x"/>
	/// </summary>
	/// <remarks>If <paramref name="x"/> is 0, the function returns 1.</remarks>
	template<typename T>
	T sinc(T x)
	{
		constexpr T pi = static_cast<T>(M_PI);
		return x ? (std::sin(pi * x)) / (pi * x) : 1;
	}

	/// <summary>
	/// Converts the entire container <paramref name="c"/> to a vector of complex numbers
	/// </summary>
	template<typename T, typename Container>
	auto to_complex(const Container& c) -> decltype(std::begin(c), std::end(c), std::vector<std::complex<T>>())
	{
		return to_complex(c, std::distance(std::begin(c), std::end(c)));
	}

	/// <summary>
	/// Converts <paramref name="size"/> elements from the <paramref name="c"/> to a vector of complex numbers
	/// </summary>
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
#ifndef CPPAPE_SIMD_H
#define CPPAPE_SIMD_H

#include <complex>
#include <climits>
#include "misc.h"
#include <vector>

namespace ape
{
#ifndef __CPPAPE_NATIVE_VECTOR_BIT_WIDTH__
#define __CPPAPE_NATIVE_VECTOR_BIT_WIDTH__ 128
#endif

#define __CPPAPE_NATIVE_VECTOR_BYTES__ (__CPPAPE_NATIVE_VECTOR_BIT_WIDTH__ / CHAR_BIT)

	/// <summary>
	/// A machine sized SIMD register with as many lanes as possible for the given <typeparamref name="T"/>
	/// </summary>
	template<typename T>
	using vector_register = T __attribute__((ext_vector_type(__CPPAPE_NATIVE_VECTOR_BYTES__ / sizeof(T))));

	/// <summary>
	/// Traits for <see cref="vector_register"/>
	/// </summary>
	template<typename V>
	struct vector_traits
	{
		/// <summary>
		/// The type of the element in the <see cref="vector_register"/>
		/// </summary>
		typedef decltype(V()[0]) value_type;
		/// <summary>
		/// How many lanes (or "width") of the register there is
		/// </summary>
		static constexpr std::size_t lanes = sizeof(V) / sizeof(value_type);
	};

	template<typename T>
	inline std::complex<vector_register<T>> operator * (std::complex<vector_register<T>> a, std::complex<vector_register<T>> b)
	{
		return { a.real() * b.real() - a.imag() * b.imag(), a.real() * b.imag() + a.imag() * b.real() };
	}

	/// <summary>
	/// Reinterprets the <paramref name="input"/> as a array of SIMD vectors
	/// </summary>
	template<typename T>
	inline uarray<vector_register<T>> as_vectors(uarray<T> input)
	{
		constexpr auto mask = __CPPAPE_NATIVE_VECTOR_BYTES__ - 1;

		assert((static_cast<std::uintptr_t>(input.data()) & mask) == 0 && "Input array not sufficiently aligned");
		assert(input.size() * sizeof(T) % sizeof(vector_register<T>) == 0 && "Whole number of machine vector registers does not fit in array");

		return { reinterpret_cast<vector_register<T>>(input.data(), (sizeof(T) * input.size()) / sizeof(vector_register<T>)) };
	}

	/// <summary>
	/// Reinterprets the <paramref name="input"/> as a array of complex SIMD vectors
	/// </summary>
	template<typename T>
	inline uarray<std::complex<vector_register<T>>> as_vectors(uarray<std::complex<T>> input)
	{
		constexpr auto mask = __CPPAPE_NATIVE_VECTOR_BYTES__ - 1;

		assert((reinterpret_cast<std::uintptr_t>(input.data()) & mask) == 0 && "Input array not sufficiently aligned");
		assert(input.size() * sizeof(T) % sizeof(std::complex<vector_register<T>>) == 0 && "Whole number of machine vector registers does not fit in array");

		return { reinterpret_cast<std::complex<vector_register<T>>*>(input.data()), (sizeof(std::complex<T>) * input.size()) / sizeof(std::complex<vector_register<T>>) };
	}

	/// <summary>
	/// Reinterprets the <paramref name="input"/> as a array of SIMD vectors
	/// </summary>
	template<typename T>
	inline uarray<vector_register<T>> as_vectors(std::vector<T>& input)
	{
		return as_vectors(as_uarray(input));
	}

	/// <summary>
	/// Reinterprets the <paramref name="input"/> as a array of complex SIMD vectors
	/// </summary>
	template<typename T>
	inline uarray<std::complex<vector_register<T>>> as_vectors(std::vector<std::complex<T>>& input)
	{
		return as_vectors(as_uarray(input));
	}
}

#endif
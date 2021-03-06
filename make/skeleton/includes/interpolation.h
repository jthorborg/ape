#ifndef CPPAPE_INTERPOLATION_H
#define CPPAPE_INTERPOLATION_H

#include <cmath>
#include <utility>
#include "dsp.h"

namespace ape
{
	/// <summary>
	/// Do lanczos interpolation at a specific point in a signal.
	/// </summary>
	/// <param name="wsize">
	/// The kernel size of the lanczos function.
	/// </param>
	/// <param name="x">
	/// A fractional point in the <paramref name="s"/> to compute.
	/// </param>
	/// <typeparam name="Signal">
	/// A functor object supporting operator().
	/// <seealso cref="circular_signal{T}"/>
	/// <seealso cref="windowed_signal{T}"/>
	/// </typeparam>
	template<typename T, typename Signal>
	inline T lanczosFilter(const Signal& s, double x, long long wsize)
	{
		T resonance = 0;
		long long start = std::floor<long long>(x);
		for (auto i = start - wsize + 1; i < (start + wsize + 1); ++i)
		{
			auto impulse = s(i);
			auto response = lanczos<T>(x - i, wsize);
			resonance += impulse * response;
		}
		return resonance;
	}

	/// <summary>
	/// Do sinc interpolation at a specific point in a signal.
	/// </summary>
	/// <param name="wsize">
	/// The kernel size of the sinc function.
	/// </param>
	/// <param name="x">
	/// A fractional point in the <paramref name="s"/> to compute.
	/// </param>
	/// <typeparam name="Signal">
	/// A functor object supporting operator().
	/// <seealso cref="circular_signal{T}"/>
	/// <seealso cref="windowed_signal{T}"/>
	/// </typeparam>
	template<typename T, typename Signal>
	inline T sincFilter(const Signal& s, double x, long long wsize)
	{
		T resonance = 0;
		long long start = std::floor<long long>(x);
		for (auto i = start - wsize + 1; i < (start + wsize + 1); ++i)
		{
			auto impulse = s(i);
			auto response = sinc<T>(x - i);
			resonance += impulse * response;
		}
		return resonance;
	}

	/// <summary>
	/// Do hermite 4 interpolation given the four y-coordinates
	/// </summary>
	/// <param name="offset">
	/// Fractional offset to evaluate
	/// </param>
	/// <remarks>
	/// This is Laurent de Soras' algorithm
	/// </remarks>
	template<typename T>
	inline const T hermite4(const T offset, const T ym1, const T y0, const T y1, const T y2)
	{
		const T c = (y1 - ym1) * static_cast<T>(0.5);
		const T v = y0 - y1;
		const T w = c + v;
		const T a = w + v + (y2 - y0) * static_cast<T>(0.5);
		const T b_neg = w + a;

		return ((((a * offset) - b_neg) * offset + c) * offset + y0);
	}

	/// <summary>
	/// Do hermite 4 interpolation at a specific point in a signal.
	/// </summary>
	/// <typeparam name="Signal">
	/// A functor object supporting operator().
	/// <seealso cref="circular_signal{T}"/>
	/// <seealso cref="windowed_signal{T}"/>
	/// </typeparam>
	template<typename T, typename Signal>
	inline const T hermite4(const Signal& s, const T x)
	{
		long long x0 = std::floor<long long>(x);
		return hermite4(x - x0, s(x0 - 1), s(x0), s(x0 + 1), s(x0 + 2));
	}

	/// <summary>
	/// Do simple linear interpolation between two points.
	/// </summary>
	/// <param name="offset">
	/// Fractional offset to evaluate between <paramref name="y0"/> and <paramref name="y1"/>
	/// </param>
	template<typename T>
	inline const T linear(const T offset, const T y0, const T y1)
	{
		return y0 * (1 - offset) + y1 * offset;
	}

	/// <summary>
	/// Do simple linear interpolation at a specific point in a signal.
	/// </summary>
	/// <typeparam name="Signal">
	/// A functor object supporting operator().
	/// <seealso cref="circular_signal{T}"/>
	/// <seealso cref="windowed_signal{T}"/>
	/// </typeparam>
	template<typename T, typename Signal>
	inline const T linear(const Signal& s, const T x)
	{
		long long x0 = std::floor<long long>(x);
		return linear(x - x0, s(x0), s(x0 + 1));
	}

	namespace detail
	{
		// Code is a generalization of JUCE's lagrange interpolator class, which is this case is GPL v3 
		template <typename T, int k>
		struct lagrange_basis
		{
			static void mul(T& a, T b) { a *= b * (static_cast<T>(1) / k); }
		};

		template<typename T>
		struct lagrange_basis <T, 0>
		{
			static void mul(T&, T) {}
		};

		template <typename T, int k, int Order, std::size_t... I>
		static float lagrange_polynomial(T input, const T offset, std::index_sequence<I...>) noexcept
		{
			auto const ks = -Order / 2;
			(lagrange_basis <T, static_cast<int>(I) - k>::mul(input, static_cast<int>(I) + ks - offset), ...);
			return input;
		}

		template<typename T, std::size_t Order, typename Signal, std::size_t... I>
		inline const T lagrange_unpack(const Signal& s, long long start, const T offset, std::index_sequence<I...> indices) noexcept
		{
			auto const ks = static_cast<long>(-static_cast<T>(Order) / static_cast<T>(2));
			return (lagrange_polynomial<T, I, Order>(s(ks + start + I), offset, indices) + ...);
		}
	}

	/// <summary>
	/// Do lagrange interpolation at a specific point in a signal, of <typeparamref name="Order"/> order.
	/// <seealso cref="lagrange5"/>
	/// </summary>
	/// <typeparam name="Signal">
	/// A functor object supporting operator().
	/// <seealso cref="circular_signal{T}"/>
	/// <seealso cref="windowed_signal{T}"/>
	/// </typeparam>
	template<typename T, std::size_t Order, typename Signal>
	inline const T lagrange(const Signal& s, const T position) noexcept
	{
		using namespace detail;
		const long long start = static_cast<long long>(position);
		return lagrange_unpack<T, Order>(s, start, position - start, std::make_index_sequence<Order>());
	}

	/// <summary>
	/// Do lagrange interpolation between the y parameters, with 5 terms.
	/// </summary>
	/// <param name="offset">
	/// Fractional offset to evaluate.
	/// </param>
	template<typename T>
	inline const T lagrange5(const T offset, const T ym2, const T ym1, const T y0, const T y1, const T y2)
	{
		using namespace detail;

		const auto indices = std::make_index_sequence<5>();

		return	lagrange_polynomial<T, 0, 5>(ym2, offset, indices)
			+ lagrange_polynomial<T, 1, 5>(ym1, offset, indices)
			+ lagrange_polynomial<T, 2, 5>(y0, offset, indices)
			+ lagrange_polynomial<T, 3, 5>(y1, offset, indices)
			+ lagrange_polynomial<T, 4, 5>(y2, offset, indices);
	}
}

#endif
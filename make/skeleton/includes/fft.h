#ifndef CPPAPE_FFT_H
#define CPPAPE_FFT_H

#include "baselib.h"
#include "misc.h"
#include <complex>

namespace ape
{
	/// <summary>
	/// A typed FFT.
	/// <seealso cref="FFT{float}"/>
	/// <seealso cref="FFT{double}"/>
	/// </summary>
	template<typename T>
	class FFT;

	/// <summary>
	/// Shared operations on typed FFTs.
	/// <seealso cref="FFT{float}"/>
	/// <seealso cref="FFT{double}"/>
	/// </summary>
	template<typename T>
	class FFTBase
	{
	public:

		/// <summary>
		/// Do in-place forward complex fourier transform
		/// </summary>
		void forward(uarray<std::complex<T>> inout)
		{
			assert(inout.size() == size);

			perform(APE_FFT_Forward, inout.data(), inout.data());
		}

		/// <summary>
		/// Do out of place forward complex fourier transform
		/// </summary>
		void forward(uarray<const std::complex<T>> in, uarray<std::complex<T>> out)
		{
			assert(in.size() == size);
			assert(out.size() == size);

			perform(APE_FFT_Forward, in.data(), out.data());
		}

		/// <summary>
		/// Do out of place forward real only fourier transform
		/// </summary>
		/// <remarks>
		/// The complex mirror spectrum is not guaranteed to exist in the output
		/// </remarks>
		void forwardReal(uarray<const T> in, uarray<std::complex<T>> out)
		{
			assert(in.size() == size);
			assert(out.size() == size);

			perform(APE_FFT_Forward, in.data(), out.data());
		}

		/// <summary>
		/// Do in-place backwards / inverse complex fourier transform.
		/// The output will be scaled back such that inverse(forward(signal)) equals the original signal
		/// </summary>
		void inverse(uarray<std::complex<T>> inout)
		{
			assert(inout.size() == size);

			perform(APE_FFT_Inverse, inout.data(), inout.data());
		}

		/// <summary>
		/// Do out of place backwards / inverse complex fourier transform.
		/// The output will be scaled back such that inverse(forward(signal)) equals the original signal
		/// </summary>
		void inverse(uarray<const std::complex<T>> in, uarray<std::complex<T>> out)
		{
			assert(in.size() == size);
			assert(out.size() == size);

			perform(APE_FFT_Inverse, in.data(), out.data());
		}

		/// <summary>
		/// Do in-place backwards / inverse complex fourier transform.
		/// The output will not be scaled.
		/// </summary>
		void inverseNonScaled(uarray<std::complex<T>> inout)
		{
			assert(inout.size() == size);

			perform(APE_FFT_Inverse | APE_FFT_NonScaled, inout.data(), inout.data());
		}

		/// <summary>
		/// Do out of place backwards / inverse complex fourier transform.
		/// The output will not be scaled.
		/// </summary>
		void inverseNonScaled(uarray<const std::complex<T>> in, uarray<std::complex<T>> out)
		{
			assert(in.size() == size);
			assert(out.size() == size);

			perform(APE_FFT_Inverse | APE_FFT_NonScaled, in.data(), out.data());
		}

		FFTBase(FFTBase&& other)
			: size(other.size), fft(other.fft)
		{
			other.fft = 0;
		}

		FFTBase& operator = (FFTBase&& other)
		{
			size = other.size;
			fft = other.fft;
			other.fft = 0;

			return *this;
		}

		FFTBase& operator = (const FFTBase& other) = delete;
		FFTBase(const FFTBase& other) = delete;

		~FFTBase()
		{
			if(fft != 0)
				getInterface().releaseFFT(&getInterface(), fft);
		}

	protected:

		FFTBase(std::size_t N, int fft)
			: size(N), fft(fft)
		{
		}

	private:

		void perform(APE_FFT_Options options, const void* in, void* out)
		{
			assert(fft != 0);
			assert(size != 0);

			getInterface().performFFT(&getInterface(), fft, options, in, out);
		}

		std::size_t size;
		int fft;
	};

	/// <summary>
	/// Class for performing power-of-two fast fourier transforms on 32-bit floats
	/// </summary>
	template<>
	class FFT<float> : public FFTBase<float>
	{
	public:

		/// <summary>
		/// Create a new instance of an fft.
		/// </summary>
		/// <param name="N">
		/// The size of the transform. Must be a power of two.
		/// </param>
		FFT(std::size_t N)
			: FFTBase(N, getInterface().createFFT(&getInterface(), APE_DataType_Single, N))
		{
		}

		/// <summary>
		/// Default-initialized, invalid fft.
		/// </summary>
		FFT() : FFTBase(0, 0) {}
	};

	/// <summary>
	/// Class for performing power-of-two fast fourier transforms on 64-bit floats
	/// </summary>
	template<>
	class FFT<double> : public FFTBase<double>
	{
	public:

		/// <summary>
		/// Create a new instance of an fft.
		/// </summary>
		/// <param name="N">
		/// The size of the transform. Must be a power of two.
		/// </param>
		FFT(std::size_t N)
			: FFTBase(N, getInterface().createFFT(&getInterface(), APE_DataType_Double, N))
		{
		}

		/// <summary>
		/// Default-initialized, invalid fft.
		/// </summary>
		FFT() : FFTBase(0, 0) {}

	};

}

#endif
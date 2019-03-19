#ifndef CPPAPE_FFT_H
#define CPPAPE_FFT_H

#include "baselib.h"
#include "misc.h"
#include <complex>

namespace ape
{
	template<typename T>
	class FFT;

	template<typename T>
	class FFTBase
	{
	public:

		void forward(uarray<std::complex<T>> inout)
		{
			assert(inout.size() == size);

			perform(APE_FFT_Forward, inout.data(), inout.data());
		}

		void forward(const_uarray<std::complex<T>> in, uarray<std::complex<T>> out)
		{
			assert(in.size() == size);
			assert(out.size() == size);

			perform(APE_FFT_Forward, in.data(), out.data());
		}

		void forwardReal(const_uarray<T> in, uarray<std::complex<T>> out)
		{
			assert(in.size() == size);
			assert(out.size() == size);

			perform(APE_FFT_Forward, in.data(), out.data());
		}

		void inverse(uarray<std::complex<T>> inout)
		{
			assert(inout.size() == size);

			perform(APE_FFT_Inverse, inout.data(), inout.data());
		}

		void inverse(const_uarray<std::complex<T>> in, uarray<std::complex<T>> out)
		{
			assert(in.size() == size);
			assert(out.size() == size);

			perform(APE_FFT_Inverse, in.data(), out.data());
		}

		void inverseNonScaled(uarray<std::complex<T>> inout)
		{
			assert(inout.size() == size);

			perform(APE_FFT_Inverse | APE_FFT_NonScaled, inout.data(), inout.data());
		}

		void inverseNonScaled(const_uarray<std::complex<T>> in, uarray<std::complex<T>> out)
		{
			assert(in.size() == size);
			assert(out.size() == size);

			perform(APE_FFT_Inverse | APE_FFT_NonScaled, in.data(), out.data());
		}

		~FFTBase()
		{
			getInterface().releaseFFT(&getInterface(), fft);
		}

	protected:

		FFTBase(std::size_t N, APE_FFT* fft)
			: size(N), fft(fft)
		{
		}

	private:

		void perform(APE_FFT_Options options, const void* in, void* out)
		{
			getInterface().performFFT(&getInterface(), fft, options, in, out);
		}

		std::size_t size;
		APE_FFT* fft;
	};

	template<>
	class FFT<float> : public FFTBase<float>
	{
	public:

		FFT(std::size_t N)
			: FFTBase(N, getInterface().createFFT(&getInterface(), APE_DataType_Single, N))
		{
		}
	};

	template<>
	class FFT<double> : public FFTBase<double>
	{
	public:

		FFT(std::size_t N)
			: FFTBase(N, getInterface().createFFT(&getInterface(), APE_DataType_Double, N))
		{
		}
	};

}

#endif
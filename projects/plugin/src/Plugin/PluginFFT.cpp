/*************************************************************************************

	Audio Programming Environment VST. 
		
    Copyright (C) 2019 Janus Lynggaard Thorborg [LightBridge Studios]

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

	See \licenses\ for additional details on licenses associated with this program.

**************************************************************************************

	file:PluginFFT.cpp
		
		Implementions of plugin FFTs

*************************************************************************************/

#include "PluginFFT.h"
#include <cpl/Exceptions.h>
#include <vector>
#include <complex>
#include <cpl/ffts/dustfft.h>
#include <cpl/lib/AlignedAllocator.h>

namespace ape
{

	static void Transform(std::complex<double>* signal, std::size_t N, APE_FFT_Options options)
	{
		double* buffer = reinterpret_cast<double*>(signal);
		if (options & APE_FFT_Forward)
		{
			signaldust::DustFFT_fwdDa(buffer, N);
		}
		else
		{
			signaldust::DustFFT_revDa(buffer, N);

			if ((options & APE_FFT_NonScaled) == 0)
			{
				auto recip = 1.0 / N; 
				for (std::size_t i = 0; i < N; ++i)
					signal[i] *= recip;
			}
		}
	}

	class PluginFFTSingle : public APE_FFT
	{
	public:

		PluginFFTSingle(std::size_t size)
			: buffer(size)
		{

		}

		virtual void transform(const void * in, void * out, APE_FFT_Options options) override
		{
			std::size_t N = buffer.size();

			if (options & APE_FFT_Real)
			{
				const float* source = static_cast<const float*>(in);
				for (std::size_t i = 0; i < N; ++i)
				{
					buffer[i] = source[i];
				}
			}
			else
			{
				const std::complex<float>* source = static_cast<const std::complex<float>*>(in);
				for (std::size_t i = 0; i < N; ++i)
				{
					buffer[i] = source[i];
				}
			}

			Transform(buffer.data(), N, options);


			const std::complex<float>* source = static_cast<const std::complex<float>*>(out);

			for (std::size_t i = 0; i < N; ++i)
			{
				buffer[i] = source[i];
			}
		}

	private:
		cpl::aligned_vector<std::complex<double>, 32> buffer;
	};

	class PluginFFTDouble : public APE_FFT
	{
	public:

		PluginFFTDouble(std::size_t size)
			: buffer(size)
		{

		}

		virtual void transform(const void * in, void * out, APE_FFT_Options options) override
		{
			std::size_t N = buffer.size();

			if (options & APE_FFT_Real)
			{
				const double* source = static_cast<const double*>(in);
				std::complex<double>* destination = static_cast<std::complex<double>*>(out);

				for (std::size_t i = 0; i < N; ++i)
				{
					buffer[i] = source[i];
				}

				Transform(buffer.data(), N, options);

				std::memcpy(out, buffer.data(), sizeof(double) * 2 * N);
				return;
			}
			else if (in == out)
			{
				std::complex<double>* source = static_cast<std::complex<double>*>(out);
				Transform(source, N, options);
				return;
			}
			else
			{
				std::memcpy(buffer.data(), in, sizeof(double) * 2 * N);
				Transform(buffer.data(), N, options);

				std::memcpy(out, buffer.data(), sizeof(double) * 2 * N);
				return;
			}

			std::memcpy(out, buffer.data(), sizeof(double) * 2 * N);
		}

	private:
		cpl::aligned_vector<std::complex<double>, 32> buffer;
	};
}

std::unique_ptr<APE_FFT> APE_FFT::factory(std::size_t size, APE_DataType type)
{
	switch (type)
	{
	case APE_DataType_Single: return std::unique_ptr<APE_FFT> { new ape::PluginFFTSingle(size) };
	case APE_DataType_Double: return std::unique_ptr<APE_FFT> { new ape::PluginFFTDouble(size) };
	}

	CPL_RUNTIME_EXCEPTION("Trying to create an unsupported FFT");
}
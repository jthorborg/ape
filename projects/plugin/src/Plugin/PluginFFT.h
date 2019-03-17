/*************************************************************************************

	Audio Programming Environment VST. 
		
    Copyright (C) 2018 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:PluginFFT.h
		
		A FFT object for plugins

*************************************************************************************/

#ifndef APE_PLUGINFFT_H
	#define APE_PLUGINFFT_H

	#include <ape/SharedInterface.h>
	#include <cstddef>
	#include <memory>
	
	struct APE_FFT
	{
		static std::unique_ptr<APE_FFT> factory(std::size_t size, APE_DataType type);
		virtual void transform(const void* in, void* out, APE_FFT_Options options) = 0;
		virtual ~APE_FFT() { }
	};
#endif
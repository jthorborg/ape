/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
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

	file:UICommands.h
	
		Interface for an utility memory allocator class which supports RAII (allocations 
		free'd on destruction).
		All memory is zeroinitialized and contains corrupted memory checks.

*************************************************************************************/

#ifndef APE_CHANNELDIAGNOSTICVISUALIZER_H
#define APE_CHANNELDIAGNOSTICVISUALIZER_H

#include <cpl/Common.h>
#include <chrono>
namespace ape
{
	class ChannelDiagnosticVisualizer : public juce::Component, private juce::Timer
	{
	public:

		ChannelDiagnosticVisualizer();

		void reset();
		void updateStates(bool nansDetected, bool hotDetected, bool denormalsDetected);

		void timerCallback() override;

	protected:

		void paint(juce::Graphics& g) override;

	private:

		std::chrono::time_point<std::chrono::steady_clock> lastTick;
		float nanStrength, hotStrength, denormalStrength;

		bool anyStatesDetected;
		

	};
}

#endif
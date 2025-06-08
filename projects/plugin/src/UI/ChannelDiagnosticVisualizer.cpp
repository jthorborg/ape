/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2014 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:UICommands.cpp
	
		Code for handling commands to the interface

*************************************************************************************/

#include "ChannelDiagnosticVisualizer.h"
#include <chrono>

namespace ape
{
	ChannelDiagnosticVisualizer::ChannelDiagnosticVisualizer()
	{
		reset();
	}

	void ChannelDiagnosticVisualizer::reset()
	{
		nanStrength = hotStrength = denormalStrength = 0;

		anyStatesDetected = false;
	}

	void ChannelDiagnosticVisualizer::updateStates(bool nansDetected, bool hotDetected, bool denormalsDetected)
	{
		auto combined = nansDetected | hotDetected | denormalsDetected;
		anyStatesDetected |= combined;

		if (nansDetected)
			nanStrength = 1.0f;

		if (hotDetected)
			hotStrength = 1.0f;		
		
		if (denormalsDetected)
			denormalStrength = 1.0f;

		if (combined)
			repaint();

		if (anyStatesDetected)
			startTimerHz(30);

		lastTick = std::chrono::steady_clock::now();
	}

	void ChannelDiagnosticVisualizer::timerCallback()
	{
		repaint();
	}

	void ChannelDiagnosticVisualizer::paint(juce::Graphics& g)
	{
		constexpr double pole = 0.99;

		if (nanStrength > 0.5f || hotStrength > 0.5f || denormalStrength > 0.5f)
		{
			const auto now = std::chrono::steady_clock::now();
			const auto delta = std::chrono::duration_cast<std::chrono::microseconds>(now - lastTick).count() / (1000.0 * 1000.0);

			float nan = nanStrength - 0.5f;
			float hot = nanStrength - 0.5f;
			float denormal = nanStrength - 0.5f;

			const auto coeff = std::pow(pole, delta);

			nan *= coeff;
			hot *= coeff;
			denormal *= coeff;

			auto rect = getLocalBounds().toFloat();
			auto topLeft = rect.withSize(rect.getWidth() / 2, rect.getWidth() / 2);

			g.fillEllipse(topLeft);


			nanStrength = nan + 0.5f;			
			hotStrength = hot + 0.5f;
			denormalStrength = denormal + 0.5f;

			lastTick = now;
		}
	}
}
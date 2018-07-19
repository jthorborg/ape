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

	file:PluginSurface.cpp
	
		Implements controllers for commands and parameters in a plugin state

*************************************************************************************/

#include "PluginSurface.h"
#include "../PluginState.h"
#include <cpl/gui/GUI.h>
#include "../Engine.h"

namespace ape
{

	PluginSurface::PluginSurface(Engine& engine, PluginState& state)
	{
		auto& paramDefs = state.parameters;
		auto& paramManager = engine.getParameterManager();

		for (std::size_t i = 0; i < paramDefs.size(); ++i)
		{
			auto control = std::make_unique<cpl::CValueKnobSlider>(
				&paramManager.getValueFor(
					static_cast<cpl::Parameters::Handle>(i)
				)
			);

			addAndMakeVisible(*control);
			components.emplace_back(std::move(control));
		}
	}

	void PluginSurface::clearComponents()
	{
		components.clear();
	}

	void PluginSurface::resized()
	{
		if (components.empty())
			return;

		int maxWidth = 0, maxHeight = 0;

		for (std::size_t i = 0; i < components.size(); ++i)
		{
			maxWidth = std::max(maxWidth, components[i]->getWidth());
			maxHeight = std::max(maxHeight, components[i]->getHeight());
		}

		// weird error
		if (maxWidth == 0 || maxHeight == 0)
			return;

		int columns = std::max(1, getWidth() / maxWidth);

		for (std::size_t i = 0; i < components.size(); ++i)
		{
			auto bounds = components[i]->getBounds();
			bounds.setPosition(maxWidth * (i % columns), maxHeight * (i / columns));
			components[i]->setBounds(bounds);
		}
	}
}

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
#include "PluginParameter.h"
#include "PluginWidget.h"

namespace ape
{

	PluginSurface::PluginSurface(Engine& engine, PluginState& state)
	{
		auto& paramDefs = state.parameters;
		auto& paramManager = engine.getParameterManager();

		for (std::size_t i = 0; i < paramDefs.size(); ++i)
		{
			auto control = paramDefs[i]->createController(
				paramManager.getValueFor(
					static_cast<cpl::Parameters::Handle>(i)
				)
			);

			addAndMakeVisible(*control);
			controls.emplace_back(std::move(control));
		}

		auto& pluginWidgets = state.widgets;

		for (std::size_t i = 0; i < pluginWidgets.size(); ++i)
		{
			switch (pluginWidgets[i]->getType())
			{
				case PluginWidget::Label:
				case PluginWidget::Plot:
				{
					auto widget = pluginWidgets[i]->createController();
					addAndMakeVisible(*widget);
					widgets.emplace_back(std::move(widget));
					break;
				}
				case PluginWidget::Meter:
				{
					auto meter = pluginWidgets[i]->createController();
					addAndMakeVisible(*meter);
					meters.emplace_back(std::move(meter));
					break;
				}

			}
				
		}


	}

	void PluginSurface::repaintActiveAreas()
	{
		for(auto& w : widgets)
			w->repaint();

		for (auto& m : meters)
			m->repaint();
	}

	void PluginSurface::clearComponents()
	{
		controls.clear();
		widgets.clear();
		meters.clear();
	}

	template<typename ComponentPointerList>
	static void LayoutComponents(const ComponentPointerList& components, const juce::Rectangle<int>& bounds)
	{
		if (components.empty())
			return;

		int maxWidth = 0, maxHeight = 0, size = static_cast<int>(components.size());

		for (auto i = 0; i < size; ++i)
		{
			maxWidth = std::max(maxWidth, components[i]->getWidth());
			maxHeight = std::max(maxHeight, components[i]->getHeight());
		}

		// weird error
		if (maxWidth == 0 || maxHeight == 0)
			return;

		int columns = std::max(1, bounds.getWidth() / maxWidth);

		for (auto i = 0; i < size; ++i)
		{
			auto localBounds = components[i]->getBounds();

			localBounds.setPosition(
				bounds.getX() + maxWidth * (i % columns), 
				bounds.getY() + maxHeight * (i / columns)
			);

			components[i]->setBounds(localBounds);
		}
	}

	void PluginSurface::resized()
	{
		LayoutComponents(controls, getControlSpace());
		LayoutComponents(meters, getMeteringSpace());
		LayoutComponents(widgets, getWidgetSpace());
	}

	void PluginSurface::paint(juce::Graphics & g)
	{
	}

	constexpr int space = 90;

	juce::Rectangle<int> PluginSurface::getControlSpace()
	{
		auto current = getLocalBounds();
		current.removeFromRight(space);
		current.removeFromBottom(space);
		return current.reduced(5);
	}

	juce::Rectangle<int> PluginSurface::getMeteringSpace()
	{
		return getLocalBounds().withLeft(getWidth() - space).reduced(5);
	}

	juce::Rectangle<int> PluginSurface::getWidgetSpace()
	{
		return getLocalBounds().withTop(getHeight() - space).reduced(5);
	}
}

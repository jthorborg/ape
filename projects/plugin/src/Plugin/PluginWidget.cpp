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

	file:PluginWidget.cpp
	
		Implementation of PluginWidget.h

*************************************************************************************/

#include "PluginWidget.h"
#include "PluginCommandQueue.h"
// TODO: Remove when exceptions.h is properly standalone
#include <cpl/PlatformSpecific.h>
#include <cpl/Core.h>
#include <cpl/Exceptions.h>
#include <cpl/gui/GUI.h>

namespace ape
{
	class MeterWidget : public juce::Component
	{
	public:


	private:
		const float* extVal;
	};

	class LabelWidget : public PluginWidget
	{
	public:

		class LabelWidgetUI : public juce::Component
		{
		public:

			LabelWidgetUI(LabelWidget& parent) 
				: parent(parent) 
			{
				setSize(cpl::ControlSize::Rectangle.width, cpl::ControlSize::Rectangle.height);
			}

			void paint(juce::Graphics& g) override
			{
				const auto halfHeight = getHeight() / 2;
				const auto bounds = getLocalBounds().withHeight(halfHeight);
				g.setColour(cpl::GetColour(cpl::ColourEntry::ControlText));

				g.drawText(parent.name, bounds.reduced(5), juce::Justification::centredLeft, false);
				g.drawText(parent.formatString.get(), bounds.withY(bounds.getY() + halfHeight).reduced(5), juce::Justification::centredLeft, false);
			}

		private:
			LabelWidget& parent;
		};

		LabelWidget(FormatLabelRecord&& record)
			: PluginWidget(Label)
			, name(std::move(record.name))
			, formatString(std::move(record.stringValue))
		{

		}

		std::unique_ptr<juce::Component> createController() override
		{
			return std::unique_ptr<juce::Component>(new LabelWidgetUI(*this));
		}


	private:
		std::string name;
		ReferenceFormattedString formatString;
	};


	std::unique_ptr<PluginWidget> PluginWidget::FromRecord(WidgetRecord&& record)
	{
		switch (record.getWidgetType())
		{
		case WidgetRecord::Label:
			return std::unique_ptr<PluginWidget>(new LabelWidget(std::move(static_cast<FormatLabelRecord&>(record))));
		}

		CPL_RUNTIME_EXCEPTION("Unknown mapping from parameter record to plugin parameter");
	}
};
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

	class MeterWidget : public PluginWidget
	{
	public:

		class MeterWidgetUI : public juce::Component
		{
		public:

			MeterWidgetUI(MeterWidget& parent)
				: parent(parent)
			{
				setSize(cpl::ControlSize::Rectangle.height, cpl::ControlSize::Rectangle.width);
			}

			void paint(juce::Graphics& g) override
			{
				g.setColour(cpl::GetColour(cpl::ColourEntry::ControlText));

				const auto textRect = getLocalBounds().withHeight(20);

				g.drawText(parent.name, textRect.reduced(5), juce::Justification::centredLeft, false);

				auto meterRect = getLocalBounds().withTop(textRect.getBottom()).toFloat();
				auto totalHeight = meterRect.getHeight();

				auto meterValue = *parent.normalValue;
				meterValue = std::clamp(meterValue, 0.0, 1.0);
				auto top = static_cast<float>(meterRect.getBottom() - totalHeight * meterValue);

				g.setColour(juce::Colours::green);
				g.fillRect(meterRect.withTop(top));

				if (parent.peakValue)
				{
					auto peakValue = *parent.peakValue;
					g.setColour(juce::Colours::lightyellow);
					top = static_cast<float>(meterRect.getBottom() - totalHeight * std::clamp(peakValue, 0.0, 1.0));
					g.drawLine(0, top, getWidth(), top, 2);
				}

			}

		private:
			MeterWidget& parent;
		};

		MeterWidget(MeterRecord&& record)
			: PluginWidget(Meter)
			, name(std::move(record.name))
			, normalValue(record.value)
			, peakValue(record.peak)
		{

		}

		std::unique_ptr<juce::Component> createController() override
		{
			return std::unique_ptr<juce::Component>(new MeterWidgetUI(*this));
		}


	private:
		std::string name;
		const volatile double
			* normalValue,
			* peakValue;
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

	class PlotWidget : public PluginWidget
	{
	public:

		class PlotRecordUI : public juce::Component
		{
		public:

			PlotRecordUI(PlotWidget& parent)
				: parent(parent)
			{
				setSize(cpl::ControlSize::Rectangle.width, cpl::ControlSize::Rectangle.height);
			}

			void paint(juce::Graphics& g) override
			{
				const auto textRect = getLocalBounds().withHeight(20);
				g.setColour(cpl::GetColour(cpl::ColourEntry::ControlText));

				g.drawText(parent.name, textRect.reduced(5), juce::Justification::centredLeft, false);

				g.setColour(juce::Colours::green);
				auto plotSize = getLocalBounds().withTop(textRect.getBottom()).toFloat();

				float halfHeight = plotSize.getHeight() / 2.f, offset = static_cast<float>(plotSize.getY());

				juce::Path p;
				p.startNewSubPath(0, -parent.values[0] * halfHeight + offset + halfHeight);
				bool shouldScale(false); // we scale any values over [-1, 1] to fit, otherwise we wont.
				double tempVal;

				for (std::size_t i = 1; i < parent.numValues; i++)
				{
					tempVal = -parent.values[i] * halfHeight + offset + halfHeight;

					if (!std::isnormal(tempVal))
					{
						// 'error' on NaN values
						g.setColour(juce::Colours::red);
						g.setFont(cpl::TextSize::normalText);
						g.drawText("NaN values", juce::Rectangle<int>(0, 0, getWidth(), getHeight()), juce::Justification::centred, false);
						return;
					}

					p.lineTo(static_cast<float>(i), static_cast<float>(tempVal));

					if (!shouldScale && (std::fabs(parent.values[i]) > 1))
						shouldScale = true;
				}

				juce::PathStrokeType pst(2);

				//p.closeSubPath();
				if (shouldScale)
				{
					p.scaleToFit(plotSize.getX(), plotSize.getY(), plotSize.getWidth(), plotSize.getHeight(), false);
					g.strokePath(p, pst);
					g.setColour(juce::Colours::red);
					g.drawText("+1", juce::Rectangle<int>(5, plotSize.getY(), 20, 20), juce::Justification::centred, false);
				}
				else
				{
					g.strokePath(p, pst);
				}


			}

		private:
			PlotWidget& parent;
		};

		PlotWidget(PlotRecord&& record)
			: PluginWidget(Label)
			, name(std::move(record.name))
			, values(record.values)
			, numValues(record.numValues)
		{

		}

		std::unique_ptr<juce::Component> createController() override
		{
			return std::unique_ptr<juce::Component>(new PlotRecordUI(*this));
		}


	private:
		std::string name;
		const double* const values;
		const std::size_t numValues;
	};


	std::unique_ptr<PluginWidget> PluginWidget::FromRecord(WidgetRecord&& record)
	{
		switch (record.getWidgetType())
		{
		case WidgetRecord::Meter:
			return std::unique_ptr<PluginWidget>(new MeterWidget(std::move(static_cast<MeterRecord&>(record))));
		case WidgetRecord::Plot:
			return std::unique_ptr<PluginWidget>(new PlotWidget(std::move(static_cast<PlotRecord&>(record))));
		case WidgetRecord::Label:
			return std::unique_ptr<PluginWidget>(new LabelWidget(std::move(static_cast<FormatLabelRecord&>(record))));

		}

		CPL_RUNTIME_EXCEPTION("Unknown mapping from parameter record to plugin parameter");
	}
};
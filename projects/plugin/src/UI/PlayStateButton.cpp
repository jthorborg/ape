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

	file:PlayStateButton.cpp
	
		Implementation of PlayStateButton.h

*************************************************************************************/

#include "PlayStateButton.h"

namespace ape
{
	using namespace cpl;


	PlayStateButton::PlayStateButton(ValueEntityBase& compilationState, ValueEntityBase& activationState)
		: juce::Button("PlayStateButtton")
		, Base(this, &compilationState, false)
		, activationState(activationState)
	{
		setSize(ControlSize::Rectangle.width, ControlSize::Rectangle.height / 2);
		enableTooltip(true);
		setClickingTogglesState(true);
		setToggleState(getValueReference().getNormalizedValue() > 0.5 ? true : false, juce::NotificationType::dontSendNotification);

		activationState.addListener(this);
	}

	PlayStateButton::~PlayStateButton()
	{
		activationState.removeListener(this);
	}

	void PlayStateButton::onValueObjectChange(ValueEntityListener * sender, ValueEntityBase * object)
	{
		const auto toggled = object->getNormalizedValue() > 0.5 ? true : false;

		if (object != &activationState)
		{
			if (toggled)
				startTimerHz(30);
			else
				stopTimer();

			timeAtCompilationStart = std::chrono::steady_clock::now();

			setToggleState(toggled, juce::NotificationType::dontSendNotification);
		}
		else
		{
			if (toggled)
				stopTimer();

			repaint();
		}

	}

	std::string PlayStateButton::bGetTitle() const
	{
		return "Play state";
	}

	void PlayStateButton::timerCallback()
	{
		repaint();
	}

	void PlayStateButton::clicked()
	{
		valueObject->setNormalizedValue(getToggleState() ? 1.0f : 0.0f);
	}


	void PlayStateButton::paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
	{
		const bool isPressed = isButtonDown || getToggleState();

		auto bounds = getLocalBounds().toType<float>();
		auto smallestSide = std::min(bounds.getWidth(), bounds.getHeight());

		bounds = bounds.withSizeKeepingCentre(smallestSide, smallestSide);

		juce::Path p;

		bool isCompiling = getToggleState();
		bool isActivated = false;

		if (isCompiling)
		{
			constexpr auto numBalls = 8;
			constexpr auto ballSize = 0.5f;
			constexpr auto spread = 2 * M_PI / numBalls;
			constexpr auto timeScale = 1;

			const auto center = bounds.getCentre();
			const auto radius = (1 - ballSize) * smallestSide * 0.5;

			const auto fdelta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - timeAtCompilationStart).count() / (1000.0 * 1000.0);

			auto phaseOffset = timeScale * fdelta * 2 * M_PI;

			phaseOffset += 0.5 * std::sin(phaseOffset - M_PI);

			phaseOffset -= M_PI * 0.5;

			for (int i = 0; i < numBalls; ++i)
			{
				const auto position = static_cast<double>(i) / numBalls;
				const auto sizeFactor = 0.5 + 0.5 * (1 - position);

				const auto x = std::cos(position * 2 * M_PI + phaseOffset);
				const auto y = std::sin(position * 2 * M_PI + phaseOffset);

				const auto rect = juce::Rectangle<float>(center.getX() + x * radius, center.getY() + y * radius, radius * ballSize * sizeFactor, radius * ballSize * sizeFactor);

				p.addEllipse(rect);
			}


		}
		else
		{
			if (isPressed)
				bounds.expand(-5, -5);

			if (activationState.getNormalizedValue() > 0.5)
			{
				constexpr float width = 0.28f;

				auto unit = width * smallestSide;

				const auto pieSize = bounds.expanded(-unit * 0.413);

				p.addPieSegment(pieSize.getX(), pieSize.getY(), pieSize.getWidth(), pieSize.getHeight(), 0, 1.5 * M_PI, 1 - (width * 0.5));

				auto topRight = bounds.getTopLeft().translated(bounds.getWidth() * 0.5f, 0);

				p.addTriangle(topRight, topRight.translated(0, unit), topRight.translated(-unit, unit * 0.5f));
			}
			else
			{

				p.addTriangle(bounds.getTopLeft(), bounds.getTopRight().withY(bounds.getHeight() * 0.5f + bounds.getY()), bounds.getBottomLeft());
			}
		}

		juce::PathStrokeType pst(1);

		g.setColour(juce::Colours::lightgoldenrodyellow.darker((isMouseOverButton ^ isCompiling) ? 0.4 : 0));
		g.fillPath(p);

	}

}

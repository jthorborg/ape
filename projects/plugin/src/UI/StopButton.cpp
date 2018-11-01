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

	file:StopButton.cpp
	
		Implementation of StopButton.h

*************************************************************************************/

#include "StopButton.h"

namespace ape
{
	using namespace cpl;


	StopButton::StopButton(ValueEntityBase& activationState)
		: juce::Button("StopButton")
		, Base(this, &activationState, false)
	{
		setSize(ControlSize::Rectangle.width, ControlSize::Rectangle.height / 2);
		enableTooltip(true);
		setClickingTogglesState(true);
		setToggleState(getValueReference().getNormalizedValue() > 0.5 ? true : false, juce::NotificationType::dontSendNotification);
	}

	StopButton::~StopButton()
	{
	}

	void StopButton::onValueObjectChange(ValueEntityListener * sender, ValueEntityBase * object)
	{
		const auto toggled = object->getNormalizedValue() > 0.5 ? true : false;
		setToggleState(toggled, juce::NotificationType::dontSendNotification);
	}

	std::string StopButton::bGetTitle() const
	{
		return "Stop state";
	}

	void StopButton::clicked()
	{
		valueObject->setNormalizedValue(0.0f);
	}

	void StopButton::paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
	{
		const bool isPressed = isButtonDown || getToggleState();

		auto bounds = getLocalBounds().toType<float>();
		auto smallestSide = std::min(bounds.getWidth(), bounds.getHeight());

		bounds = bounds.withSizeKeepingCentre(smallestSide, smallestSide);

		if (isButtonDown)
			bounds.expand(-5, -5);

		juce::PathStrokeType pst(1);

		g.setColour(juce::Colours::lightgoldenrodyellow.darker((isMouseOverButton) ? 0.4 : 0));
		g.fillRect(bounds);
	}

}

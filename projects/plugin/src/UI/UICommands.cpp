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

#include "UICommands.h"
#include "../UIController.h"

namespace ape
{
	UICommandState::UICommandState(UIController& controller)
		: parent(controller)
		, precisionChoices(precisionTransformer)
		, compile(*this, *this)
		, activationState(*this, *this)
		, clean(*this, *this)
		, precision(precisionTransformer, precisionChoices)
	{
		clean.addListener(this);
		compile.addListener(this);
		activationState.addListener(this);
		precision.addListener(this);

		precisionChoices.setValues({ "32-bit", "64-bit", "80-bit" });
	}

	void UICommandState::changeValueExternally(UIValue& value, double newValue)
	{
		value.setValue(newValue, this);
	}

	void UICommandState::serialize(Archiver & ar, cpl::Version version)
	{
		ar << compile << activationState << precision;
	}

	void UICommandState::deserialize(Builder & builder, cpl::Version version)
	{
		// TODO: Really how we want to activate a plugin?
		builder >> compile >> activationState >> precision;
	}

	void UICommandState::valueEntityChanged(ValueEntityListener * sender, cpl::ValueEntityBase * value)
	{
		if (sender == this)
			return;

		const auto toggled = value->getNormalizedValue() > 0.5;
		auto command = UICommand::Invalid;

		if(value == &precision || (value == &compile && toggled))
		{
			command = UICommand::Recompile;
		}
		else if (value == &activationState)
		{
			command = toggled ? UICommand::Activate : UICommand::Deactivate;
		}
		else if (value == &clean)
		{
			command = UICommand::Clean;
		}
		else
		{
			return;
		}

		if (!parent.performCommand(command))
			changeValueExternally(dynamic_cast<UIValue&>(*value), !toggled);
	}
}
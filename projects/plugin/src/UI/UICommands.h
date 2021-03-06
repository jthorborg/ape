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

#ifndef UICOMMANDS_H
#define UICOMMANDS_H

#include "UIValue.h"

namespace ape
{
	enum class UICommand
	{
		Invalid,
		Recompile,
		Activate,
		AsyncActivate,
		Deactivate,
		Clean
	};

	enum class FPrecision
	{
		FP32,
		FP64,
		FP80
	};

	class UIController;

	class UICommandState 
		: public cpl::CSerializer::Serializable
		, private UIValue::Listener
		, private cpl::BasicFormatter<cpl::ValueT>
		, private cpl::UnityRange<cpl::ValueT>
	{
	public:

		using Archiver = cpl::CSerializer::Archiver;
		using Builder = cpl::CSerializer::Builder;

		UICommandState(UIController& controller);

		UIValue compile, activationState, clean, precision;

		void changeValueExternally(UIValue& value, double newValue);

	protected:

		void serialize(Archiver & ar, cpl::Version version) override;
		void deserialize(Builder & builder, cpl::Version version) override;
		void valueEntityChanged(ValueEntityListener * sender, cpl::ValueEntityBase* value) override;

	private:
		cpl::ChoiceFormatter<cpl::ValueT> precisionChoices;
		cpl::ChoiceTransformer<cpl::ValueT> precisionTransformer;
		UIController& parent;
	};
}

#endif
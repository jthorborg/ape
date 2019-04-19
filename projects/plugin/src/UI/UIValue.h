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

#ifndef UIVALUE_H
#define UIVALUE_H

#include <set>
#include <cpl/infrastructure/values/Values.h>

namespace ape
{
	using UITransformer = cpl::VirtualTransformer<cpl::ValueT>;
	using UIFormatter = cpl::VirtualFormatter<cpl::ValueT>;

	class UIValue : public cpl::ValueEntityBase
	{
	public:

		typedef cpl::ValueEntityBase::Listener Listener;

		UIValue(UITransformer& transformer, UIFormatter& formatter)
			: internalValue(0)
			, transformer(transformer)
			, formatter(formatter)
		{

		}

		void addListener(Listener* listener) override { listeners.insert(listener); }
		void removeListener(Listener* listener) override { listeners.erase(listener); }

		void setValue(cpl::ValueT value, Listener* sender)
		{
			internalValue = value;
			notifyListeners(sender);
		}

	private:

		const cpl::VirtualTransformer<cpl::ValueT>& getTransformer() const override { return transformer; }
		cpl::VirtualFormatter<cpl::ValueT>& getFormatter() override { return formatter; }
		cpl::ValueT getNormalizedValue() const override { return internalValue; }
		void setNormalizedValue(cpl::ValueT value) override { setValue(value, nullptr); }

		void notifyListeners(Listener* sender = nullptr)
		{
			for (auto l : listeners)
				l->valueEntityChanged(sender, this);
		}

		double internalValue;
		UITransformer& transformer;
		UIFormatter& formatter;
		std::set<ValueEntityListener *> listeners;

	};


}

#endif
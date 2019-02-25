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

	file:ParameterManager.cpp
	
		Implementation of ParameterManager.h

*************************************************************************************/

#include "ParameterManager.h"
#include "../Engine.h"

namespace ape 
{
	ParameterManager::ParameterManager(Engine & engine, std::size_t numParameters)
		: engine(engine)
		, numParameters(numParameters)
		, parameterSet("", "", *this)
		, unnamed("unnamed")
		, traits(numParameters)
	{
		parameters.reserve(numParameters);
		
		for (std::size_t i = 0; i < numParameters; ++i)
		{
			auto& param = parameters.emplace_back(static_cast<int>(i), static_cast<LowLevelParameter::Callbacks&>(*this));
			parameterSet.registerParameter(&param);
		}

		parameterSet.seal();

		// (now parameter views are available)
		valueWrappers.reserve(numParameters);

		for (std::size_t i = 0; i < numParameters; ++i)
		{
			valueWrappers.emplace_back(parameterSet.findParameter(static_cast<IndexHandle>(i)));
		}
	}

	std::size_t ParameterManager::numParams() const noexcept
	{
		return parameters.size();
	}

	void ParameterManager::setParameter(IndexHandle index, HostFloat normalizedValue)
	{
		parameterSet.findParameter(index)->updateFromHostNormalized(normalizedValue);
	}

	HostFloat ParameterManager::getParameter(IndexHandle index)
	{
		return parameterSet.findParameter(index)->getValueNormalized<HostFloat>();
	}

	std::string ParameterManager::getParameterName(IndexHandle index)
	{
		return parameterSet.findParameter(index)->getExportedName();
	}

	std::string ParameterManager::getParameterText(IndexHandle index)
	{
		return parameterSet.findParameter(index)->getDisplayText();
	}

	cpl::ValueEntityBase& ParameterManager::getValueFor(IndexHandle index)
	{
		return valueWrappers[index];
	}

	void ParameterManager::emplaceTrait(IndexHandle index, ExternalParameterTraits& trait)
	{
		traits.at(index).exchange(&trait, std::memory_order_release);
	}

	void ParameterManager::clearTrait(IndexHandle index)
	{
		traits.at(index).store(nullptr, std::memory_order_release);
	}

	void ParameterManager::clearTraitIfMatching(IndexHandle index, ExternalParameterTraits& trait)
	{
		auto* pTrait = &trait;

		traits.at(index).compare_exchange_strong(pTrait, nullptr, std::memory_order_release);
	}

	void ParameterManager::pulse()
	{
		parameterSet.pulseUI();
	}

	void ParameterManager::automatedTransmitChangeMessage(int parameter, ParameterSet::FrameworkType value)
	{
		engine.sendParamChangeMessageToListeners(parameter, value);
	}

	void ParameterManager::automatedBeginChangeGesture(int parameter)
	{
		engine.beginParameterChangeGesture(parameter);
	}

	void ParameterManager::automatedEndChangeGesture(int parameter)
	{
		engine.endParameterChangeGesture(parameter);
	}

	bool ParameterManager::format(int ID, const LowLevelParameter::ValueType & val, std::string & buf)
	{
		if (ID < traits.size())
		{
			if (auto* trait = traits[ID].load(std::memory_order_acquire))
				return trait->format(val, buf);
		}

		return defaultFormatter.format(val, buf);
	}

	bool ParameterManager::interpret(int ID, const cpl::string_ref buf, LowLevelParameter::ValueType & val)
	{
		if (ID < traits.size())
		{
			if (auto* trait = traits[ID].load(std::memory_order_acquire))
				return trait->interpret(buf, val);
		}

		return defaultFormatter.interpret(buf, val);
	}

	LowLevelParameter::ValueType ParameterManager::transform(int ID, LowLevelParameter::ValueType val) const noexcept
	{
		if (ID < traits.size())
		{
			if (auto* trait = traits[ID].load(std::memory_order_acquire))
				return trait->transform(val);
		}

		return defaultRange.transform(val);
	}

	LowLevelParameter::ValueType ParameterManager::normalize(int ID, LowLevelParameter::ValueType val) const noexcept
	{
		if (ID < traits.size())
		{
			if (auto* trait = traits[ID].load(std::memory_order_acquire))
				return trait->normalize(val);
		}

		return defaultRange.normalize(val);
	}

	const std::string& ParameterManager::getName(int ID) const noexcept
	{
		if (ID < traits.size())
		{
			if (auto* trait = traits[ID].load(std::memory_order_acquire))
				return trait->getName();
		}

		return unnamed;
	}

	int ParameterManager::getQuantization(int ID) const noexcept
	{
		if (ID < traits.size())
		{
			if (auto* trait = traits[ID].load(std::memory_order_acquire))
				return trait->getQuantization();
		}

		return 0;
	}
}

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
	{
		parameters.reserve(numParameters);

		for (std::size_t i = 0; i < numParameters; ++i)
		{
			auto& param = parameters.emplace_back(static_cast<int>(i), static_cast<Parameter::Callbacks&>(*this));
			parameterSet.registerParameter(&param);
		}

		parameterSet.seal();
	}

	std::size_t ParameterManager::numParams() const noexcept
	{
		return parameters.size();
	}

	void ParameterManager::setParameter(IndexHandle index, SFloat normalizedValue)
	{
		parameterSet.findParameter(index)->updateFromHostNormalized(normalizedValue);
	}

	SFloat ParameterManager::getParameter(IndexHandle index)
	{
		return parameterSet.findParameter(index)->getValueNormalized<SFloat>();
	}

	std::string ParameterManager::getParameterName(IndexHandle index)
	{
		return parameterSet.findParameter(index)->getExportedName();
	}

	std::string ParameterManager::getParameterText(IndexHandle index)
	{
		return parameterSet.findParameter(index)->getDisplayText();
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

	bool ParameterManager::format(int ID, const Parameter::ValueType & val, std::string & buf)
	{
		return defaultFormatter.format(val, buf);
	}

	bool ParameterManager::interpret(int ID, const cpl::string_ref buf, Parameter::ValueType & val)
	{
		return defaultFormatter.interpret(buf, val);
	}

	Parameter::ValueType ParameterManager::transform(int ID, Parameter::ValueType val) const noexcept
	{
		return defaultRange.transform(val);
	}

	Parameter::ValueType ParameterManager::normalize(int ID, Parameter::ValueType val) const noexcept
	{
		return defaultRange.normalize(val);
	}

	const std::string& ParameterManager::getName(int ID) const noexcept
	{
		return blah;
	}
}

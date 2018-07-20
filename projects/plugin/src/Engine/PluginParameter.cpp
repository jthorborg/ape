/*************************************************************************************

	Audio Programming Environment VST. 
		
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

	file:PluginParameter.cpp
		
		Various implementations of plugin parameters

*************************************************************************************/

#include "PluginParameter.h"
#include "PluginCommandQueue.h"
#include <cpl/gui/controls/Controls.h>

namespace ape
{

	class NormalParameter final : public PluginParameter
	{
	public:

		NormalParameter(std::string name, std::string_view unit, PFloat* value, Transformer transformer, Normalizer normalizer, PFloat min, PFloat max)
			: name(std::move(name))
			, formatter(unit)
			, param(value)
			, transformer(transformer)
			, normalizer(normalizer)
			, min(min)
			, max(max)
		{

		}


	private:

		std::unique_ptr<juce::Component> createController(cpl::ValueEntityBase& value) const override
		{
			return std::make_unique<cpl::CValueKnobSlider>(&value);
		}

		bool format(const UIFloat& val, std::string & buf) override
		{
			return formatter.format(val, buf);
		}

		bool interpret(const cpl::string_ref buf, UIFloat & val) override
		{
			return formatter.interpret(buf, val);
		}

		UIFloat transform(UIFloat val) const noexcept override
		{
			return transformer(val, min, max);
		}

		UIFloat normalize(UIFloat val) const noexcept override
		{
			return normalizer(val, min, max);
		}

		void setParameterRealtime(PFloat value) noexcept override
		{
			*param = value;
		}

		const std::string& getName() noexcept override
		{
			return name;
		}

		std::string name;
		cpl::UnitFormatter<PFloat> formatter;

		PFloat* param = nullptr;
		PFloat min = 0;
		PFloat max = 1;
		Transformer transformer;
		Normalizer normalizer;
	};

	class BooleanParameter final : public PluginParameter
	{
	public:

		BooleanParameter(std::string name, PFloat* value)
			: name(std::move(name))
			, param(value)
		{

		}


	private:

		std::unique_ptr<juce::Component> createController(cpl::ValueEntityBase& value) const override
		{
			auto button = std::make_unique<cpl::CButton>(&value);

			button->setToggleable(true);
			button->setSingleText(name);

			return std::move(button);
		}

		bool format(const UIFloat& val, std::string & buf) override
		{
			return formatter.format(val, buf);
		}

		bool interpret(const cpl::string_ref buf, UIFloat & val) override
		{
			return formatter.interpret(buf, val);
		}

		UIFloat transform(UIFloat val) const noexcept override
		{
			return range.transform(val);
		}

		UIFloat normalize(UIFloat val) const noexcept override
		{
			return range.normalize(val);
		}

		void setParameterRealtime(PFloat value) noexcept override
		{
			*param = value;
		}

		const std::string& getName() noexcept override
		{
			return name;
		}

		std::string name;
		cpl::BooleanFormatter<PFloat> formatter;
		cpl::BooleanRange<PFloat> range;

		PFloat* param = nullptr;
	};


	std::unique_ptr<PluginParameter> PluginParameter::FromRecord(const ParameterRecord & record)
	{
		switch (record.type)
		{
		case ParameterRecord::ParameterType::ScaledFloat:
			return std::unique_ptr<PluginParameter>{ new NormalParameter(record.name, record.unit, record.value, record.transformer, record.normalizer, record.min, record.max) };
		case ParameterRecord::ParameterType::Boolean:
			return std::unique_ptr<PluginParameter>{ new BooleanParameter(record.name, record.value) };
		}

		CPL_RUNTIME_EXCEPTION("Unknown mapping from parameter record to plugin parameter");
	}

}
#ifndef CPPAPE_PARAMETER_H
#define CPPAPE_PARAMETER_H

#include "baselib.h"
#include <cmath>
#include <string>
#include <string_view>
#include <atomic>
#include <initializer_list>

namespace ape
{

	template<class Type, typename Select = std::true_type>
	class Param;

	namespace
	{
		static PFloat scaleLin(PFloat x, PFloat mi, PFloat ma)
		{
			return mi + x * (ma - mi);
		}

		static PFloat scaleExp(PFloat x, PFloat mi, PFloat ma)
		{
			return mi * std::pow(ma / mi, x);
		}

		static PFloat scaleLinQ(PFloat x, PFloat mi, PFloat ma)
		{
			return std::floor(static_cast<PFloat>(0.5) + mi + x * (ma - mi));
		}

		static PFloat scaleExpQ(PFloat x, PFloat mi, PFloat ma)
		{
			return std::floor(static_cast<PFloat>(0.5) + mi * std::pow(ma / mi, x));
		}

		// inverted

		static PFloat invScaleLin(PFloat y, PFloat mi, PFloat ma)
		{
			return (y - mi) / (ma - mi);
		}

		static PFloat invScaleExp(PFloat y, PFloat mi, PFloat ma)
		{
			return std::log(y / mi) / std::log(ma / mi);
		}

		static PFloat invScaleLinQ(PFloat y, PFloat mi, PFloat ma)
		{
			return (std::floor(static_cast<PFloat>(0.5) + y) - mi) / (ma - mi);
		}

		static PFloat invScaleExpQ(PFloat y, PFloat mi, PFloat ma)
		{
			return std::log(std::floor(static_cast<PFloat>(0.5) + y) / mi) / std::log(ma / mi);
		}
	}



	class Range
	{
		template<class Type, typename Select>
		friend class Param;

	public:

		enum Mapping
		{
			Lin,
			Exp
		};

		Range()
			: min(0), max(1), mapping(Lin)
		{

		}

		Range(PFloat minValue, PFloat maxValue, Mapping parameterMapping = Lin)
			: min(minValue), max(maxValue), mapping(parameterMapping)
		{

		}

		PFloat operator()(PFloat value) const noexcept
		{
			if (mapping == Lin)
				return min + value * (max - min);
			else
				return min * pow(max / min, value);
		}

		PFloat inverse(bool quantized, PFloat value) const noexcept
		{
			return getNormalizer(quantized)(value, min, max);
		}

		APE_Transformer getTransformer(bool quantized) const noexcept
		{
			return !quantized ? (mapping == Lin ? scaleLin : scaleExp)  : (mapping == Lin ? scaleLinQ : scaleExpQ);
		}

		APE_Normalizer getNormalizer(bool quantized) const noexcept
		{
			return !quantized ? (mapping == Lin ? invScaleLin : invScaleExp) : (mapping == Lin ? invScaleLinQ : invScaleExpQ);
		}

	private:

		PFloat min, max;
		Mapping mapping;
	};

	template<class Type, class Derived>
	class ParameterBase
	{
	public:

		ParameterBase(const std::string_view paramName = "", const Range parameterRange = Range())
			: name(paramName.size() == 0 ? "Unnamed" : paramName)
			, range(parameterRange)
		{
		}

		static Type convert(PFloat normalized) noexcept
		{
			return Derived::convert(normalized);
		}

		int id() const noexcept
		{
			return param.id;
		}

		bool changed() const noexcept
		{
			return param.changeFlags != 0;
		}

		operator Type() const noexcept
		{
			return convert(range(param.next));
		}

		template<typename Index>
		Type operator [] (Index idx) const noexcept
		{
			return at(idx);
		}

		template<typename Index>
		Type at(Index idx) const noexcept
		{
			return convert(range(static_cast<Type>(param.old + param.step * idx)));
		}

		Derived& operator = (Type t) noexcept
		{
			param.next = range.inverse(false, Derived::representationFor(t));
			return static_cast<Derived&>(*this);
		}

		~ParameterBase()
		{
			getInterface().destroyResource(&getInterface(), param.id, -1);
		}

	protected:

		APE_Parameter param;
		Range range;
		std::string name;
	};


	template<>
	class Param<bool> : public ParameterBase<bool, Param<bool>>
	{
	public:
		typedef ParameterBase<bool, Param<bool>> Base;
		typedef bool ParameterType;

		using Base::operator =;

		Param(const std::string_view paramName = "", const Range parameterRange = Range())
			: Param(paramName, "", parameterRange)
		{

		}

		Param(const std::string_view paramName, const std::string& unit, const Range parameterRange = Range())
			: Base(paramName, parameterRange)
		{
			getInterface().createBooleanParameter(&getInterface(), this->name.c_str(), &this->param);
		}

		static ParameterType convert(PFloat value) noexcept
		{
			return static_cast<int>(std::floor((value + static_cast<PFloat>(0.5f)))) ? true : false;
		}

		static PFloat representationFor(ParameterType value) noexcept
		{
			return static_cast<PFloat>(value ? 1.0f : 0.0f);
		}
	}; 

	template<typename T>
	class Param<T, typename std::is_enum<T>::type> : public ParameterBase<T, Param<T>>
	{
	public:
		typedef ParameterBase<T, Param<T>> Base;
		typedef T ParameterType;

		using Base::operator =;

		Param(const std::string_view paramName, const std::initializer_list<const char*> values)
			: Base(paramName, Range(0, values.size() - 1))
		{
			this->internalID = getInterface().createListParameter(
				&getInterface(),
				this->name.c_str(),
				&this->storage,
				values.size(),
				values.begin()
			);
		}

		static ParameterType convert(PFloat value) noexcept
		{
			return static_cast<ParameterType>(std::round(value));
		}

		static PFloat representationFor(ParameterType value) noexcept
		{
			return static_cast<float>(value);
		}
	};

	template<>
	class Param<float> : public ParameterBase<float, Param<float>>
	{
	public:
		typedef ParameterBase<float, Param<float>> Base;
		typedef float ParameterType;

		using Base::operator =;

		Param(const std::string_view paramName = "", const Range parameterRange = Range())
			: Param(paramName, "", parameterRange)
		{

		}

		Param(const std::string_view paramName, const std::string& unit, const Range parameterRange = Range())
			: Base(paramName, parameterRange)
		{
			getInterface().createNormalParameter(
				&getInterface(),
				this->name.c_str(),
				unit.c_str(),
				&this->param,
				this->range.getTransformer(false),
				this->range.getNormalizer(false),
				this->range.min,
				this->range.max
			);
		}

		static constexpr ParameterType convert(PFloat value) noexcept
		{
			return static_cast<ParameterType>(value);
		}

		static constexpr PFloat representationFor(ParameterType value) noexcept
		{
			return static_cast<PFloat>(value);
		}
	};

	template<>
	class Param<double> : public ParameterBase<double, Param<double>>
	{
	public:
		typedef ParameterBase<double, Param<double>> Base;
		typedef double ParameterType;

		using Base::operator =;

		Param(const std::string_view paramName = "", const Range parameterRange = Range())
			: Param(paramName, "", parameterRange)
		{

		}

		Param(const std::string_view paramName, const std::string& unit, const Range parameterRange = Range())
			: Base(paramName, parameterRange)
		{
			getInterface().createNormalParameter(
				&getInterface(),
				this->name.c_str(),
				unit.c_str(),
				&this->param,
				this->range.getTransformer(false),
				this->range.getNormalizer(false),
				this->range.min,
				this->range.max
			);
		}

		static constexpr ParameterType convert(PFloat value) noexcept
		{
			return static_cast<ParameterType>(value);
		}

		static constexpr PFloat representationFor(ParameterType value) noexcept
		{
			return static_cast<PFloat>(value);
		}

	};

	template<>
	class Param<int> : public ParameterBase<int, Param<int>>
	{
	public:
		typedef ParameterBase<int, Param<int>> Base;
		typedef int ParameterType;

		using Base::operator =;

		Param(const std::string_view paramName = "", const Range parameterRange = Range())
			: Param(paramName, "", parameterRange)
		{

		}

		Param(const std::string_view paramName, const std::string& unit, const Range parameterRange = Range())
			: Base(paramName, parameterRange)
		{
			getInterface().createNormalParameter(
				&getInterface(),
				this->name.c_str(),
				unit.c_str(),
				&this->param,
				this->range.getTransformer(true),
				this->range.getNormalizer(true),
				this->range.min,
				this->range.max
			);
		}

		static ParameterType convert(PFloat value) noexcept
		{
			return static_cast<ParameterType>(std::round(value));
		}

		static constexpr PFloat representationFor(ParameterType value) noexcept
		{
			return static_cast<PFloat>(value);
		}
	}; 

}
#endif
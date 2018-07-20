#ifndef CPPAPE_PARAMETER_H
#define CPPAPE_PARAMETER_H

#include "baselib.h"
#include <cmath>
#include <string>
#include <string_view>
#include <atomic>

template<class Type>
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
	template<typename T>
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
		: internalID()
		, name(paramName.size() == 0 ? "Unnamed" : paramName)
		, range(parameterRange)
	{
	}

	static Type convert(PFloat normalized) noexcept
	{
		return Derived::convert(normalized);
	}

	int id() const noexcept
	{
		return internalID;
	}

	operator Type() const noexcept
	{
		// TODO: Atomic read
		const volatile PFloat& read = storage;
		return convert(range(read));
	}

protected:

	int internalID;
	std::string name;
	Range range;
	PFloat storage;
};

template<class Type>
class Param;


template<>
class Param<bool> : public ParameterBase<bool, Param<bool>>
{
public:
	typedef ParameterBase<bool, Param<bool>> Base;
	typedef bool ParameterType;

	static ParameterType convert(float value) noexcept
	{
		return static_cast<int>(std::floor((value + static_cast<PFloat>(0.5f)))) ? true : false;
	}

	Param(const std::string_view paramName = "", const Range parameterRange = Range())
		: Param(paramName, "", parameterRange)
	{

	}

	Param(const std::string_view paramName, const std::string& unit, const Range parameterRange = Range())
		: Base(paramName, parameterRange)
	{
		this->internalID = getInterface().createBooleanParameter(&getInterface(), this->name.c_str(), &this->storage);
	}

}; 

template<>
class Param<float> : public ParameterBase<float, Param<float>>
{
public:
	typedef ParameterBase<float, Param<float>> Base;
	typedef float ParameterType;

	static constexpr ParameterType convert(PFloat value) noexcept
	{
		return static_cast<ParameterType>(value);
	}

	Param(const std::string_view paramName = "", const Range parameterRange = Range())
		: Param(paramName, "", parameterRange)
	{

	}

	Param(const std::string_view paramName, const std::string& unit, const Range parameterRange = Range())
		: Base(paramName, parameterRange)
	{
		this->internalID = getInterface().createNormalParameter(
			&getInterface(),
			this->name.c_str(),
			unit.c_str(),
			&this->storage,
			this->range.getTransformer(false),
			this->range.getNormalizer(false),
			this->range.min,
			this->range.max
		);
	}

};

template<>
class Param<double> : public ParameterBase<double, Param<double>>
{
public:
	typedef ParameterBase<double, Param<double>> Base;
	typedef double ParameterType;

	static constexpr ParameterType convert(PFloat value) noexcept
	{
		return static_cast<ParameterType>(value);
	}

	Param(const std::string_view paramName = "", const Range parameterRange = Range())
		: Param(paramName, "", parameterRange)
	{

	}

	Param(const std::string_view paramName, const std::string& unit, const Range parameterRange = Range())
		: Base(paramName, parameterRange)
	{
		this->internalID = getInterface().createNormalParameter(
			&getInterface(),
			this->name.c_str(),
			unit.c_str(),
			&this->storage,
			this->range.getTransformer(false),
			this->range.getNormalizer(false),
			this->range.min,
			this->range.max
		);
	}

};

template<>
class Param<int> : public ParameterBase<int, Param<int>>
{
public:
	typedef ParameterBase<int, Param<int>> Base;
	typedef int ParameterType;

	static ParameterType convert(PFloat value) noexcept
	{
		return static_cast<ParameterType>(std::round(value));
	}

	Param(const std::string_view paramName = "", const Range parameterRange = Range())
		: Param(paramName, "", parameterRange)
	{

	}

	Param(const std::string_view paramName, const std::string& unit, const Range parameterRange = Range())
		: Base(paramName, parameterRange)
	{
		this->internalID = getInterface().createNormalParameter(
			&getInterface(),
			this->name.c_str(),
			unit.c_str(),
			&this->storage,
			this->range.getTransformer(true),
			this->range.getNormalizer(true),
			this->range.min,
			this->range.max
		);
	}

}; 

#endif
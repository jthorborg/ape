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
	static float scaleLin(float v, float mi, float ma)
	{
		return mi + v * (ma - mi);
	}

	static float scaleExp(float v, float mi, float ma)
	{
		return mi * pow(ma / mi, v);
	}

	static float scaleLinQ(float v, float mi, float ma)
	{
		return floor(0.5f + mi + v * (ma - mi));
	}

	static float scaleExpQ(float v, float mi, float ma)
	{
		return floor(0.5f + mi * pow(ma / mi, v));
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

	Range(float minValue, float maxValue, Mapping parameterMapping = Lin)
		: min(minValue), max(maxValue), mapping(parameterMapping)
	{

	}

	float operator()(float value) const
	{
		if (mapping == Lin)
			return min + value * (max - min);
		else
			return min * pow(max / min, value);

	}

	APE_ScaleFunc getScaler(bool quantized) const
	{
		return !quantized ? (mapping == Lin ? scaleLin : scaleExp)  : (mapping == Lin ? scaleLinQ : scaleExpQ);
	}

private:

	float min, max;
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

	static Type convert(float normalized) noexcept
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
		const volatile float& read = storage;
		return convert(range(read));
	}

protected:

	int internalID;
	std::string name;
	Range range;
	float storage;
};

template<class Type>
class Param;


template<>
class Param<bool> : public ParameterBase<bool, Param<bool>>
{
public:
	typedef ParameterBase<bool, Param<bool>> Base;
	typedef bool ParameterType;

	static bool convert(float value) noexcept
	{
		return static_cast<int>(std::floor((value + 0.5f))) ? true : false;
	}

	Param(const std::string_view paramName = "", const Range parameterRange = Range())
		: Param(paramName, "", parameterRange)
	{

	}

	Param(const std::string_view paramName, const std::string& unit, const Range parameterRange = Range())
		: Base(paramName, parameterRange)
	{
		this->internalID = getInterface().createToggle(&getInterface(), this->name.c_str(), &this->storage);
	}

};

template<>
class Param<float> : public ParameterBase<float, Param<float>>
{
public:
	typedef ParameterBase<float, Param<float>> Base;
	typedef float ParameterType;

	static constexpr float convert(float value) noexcept
	{
		return value;
	}

	Param(const std::string_view paramName = "", const Range parameterRange = Range())
		: Param(paramName, "", parameterRange)
	{

	}

	Param(const std::string_view paramName, const std::string& unit, const Range parameterRange = Range())
		: Base(paramName, parameterRange)
	{
		this->internalID = getInterface().createRangeKnob(&getInterface(), this->name.c_str(), unit.c_str(), &this->storage, this->range.getScaler(false), this->range.min, this->range.max);
	}

};

template<>
class Param<double> : public ParameterBase<double, Param<double>>
{
public:
	typedef ParameterBase<double, Param<double>> Base;
	typedef double ParameterType;

	static constexpr double convert(float value) noexcept
	{
		return value;
	}

	Param(const std::string_view paramName = "", const Range parameterRange = Range())
		: Param(paramName, "", parameterRange)
	{

	}

	Param(const std::string_view paramName, const std::string& unit, const Range parameterRange = Range())
		: Base(paramName, parameterRange)
	{
		this->internalID = getInterface().createRangeKnob(&getInterface(), this->name.c_str(), unit.c_str(), &storage, this->range.getScaler(false), this->range.min, this->range.max);
	}

};

template<>
class Param<int> : public ParameterBase<int, Param<int>>
{
public:
	typedef ParameterBase<int, Param<int>> Base;
	typedef int ParameterType;

	static int convert(float value) noexcept
	{
		return static_cast<int>(std::round(value));
	}

	Param(const std::string_view paramName = "", const Range parameterRange = Range())
		: Param(paramName, "", parameterRange)
	{

	}

	Param(const std::string_view paramName, const std::string& unit, const Range parameterRange = Range())
		: Base(paramName, parameterRange)
	{
		this->internalID = getInterface().createRangeKnob(&getInterface(), this->name.c_str(), unit.c_str(), &this->storage, this->range.getScaler(true), this->range.min, this->range.max);
	}

}; 

#endif
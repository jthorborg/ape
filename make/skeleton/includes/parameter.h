#ifndef CPPAPE_PARAMETER_H
#define CPPAPE_PARAMETER_H

#include "baselib.h"
#include "mathlib.h"

template<class Type>
class Param;

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

class Range
{
	friend class Param;

public:

	enum Mapping
	{
		Lin,
		Exp
	};

	Range()
		: min(0), max(1)
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


Range defaultRange()
{
	return Range();
}

template<class Type>
class Param
{
	static void convert(float value, float& result)
	{
		result = value;
	}

	static void convert(float value, double& result)
	{
		result = value;
	}

	static void convert(float value, int& result)
	{
		result = (int)floor((value + 0.5f));
	}

	static void convert(float value, bool& result)
	{
		result = (int)floor((value + 0.5f)) ? true : false;
	}


public:


	Param(const char * paramName = NULL, Range parameterRange = defaultRange())
		: name(paramName), range(parameterRange)
	{
		if (name == NULL)
			name = "Unnamed";

		internalID = createSuitableExternalParameter(name, "", parameterRange, &storage, Type());

	}

	Param(const char * paramName, const char * unit, Range parameterRange = defaultRange())
		: name(paramName), range(parameterRange)
	{
		if (name == NULL)
			name = "Unnamed";

		internalID = createSuitableExternalParameter(name, unit, parameterRange, &storage, Type());
	}

	operator Type() const
	{
		Type ret;
		convert(range(storage), ret);
		return ret;
	}

	int id() const
	{
		return internalID;
	}

private:

	int createSuitableExternalParameter(const char * name, const char * unit, const Range& range, float * storage, float tag)
	{
		(void)tag;
		return getInterface().createRangeKnob(&getInterface(), name, unit, storage, range.getScaler(false), range.min, range.max);
	}

	int createSuitableExternalParameter(const char * name, const char * unit, const Range& range, float * storage, double tag)
	{
		(void)tag;
		return getInterface().createRangeKnob(&getInterface(), name, unit, storage, range.getScaler(false), range.min, range.max);
	}

	int createSuitableExternalParameter(const char * name, const char * unit, const Range& range, float * storage, int tag)
	{
		(void)tag;
		return getInterface().createRangeKnob(&getInterface(), name, unit, storage, range.getScaler(true), range.min, range.max);
	}

	int createSuitableExternalParameter(const char * name, const char * unit, const Range& range, float * storage, bool tag)
	{
		(void)unit;
		(void)range;
		(void)storage;
		(void)tag;
		return getInterface().createToggle(&getInterface(), name, storage);
	}

	int internalID;
	const char * name;
	Range range;
	float storage;
};

#endif
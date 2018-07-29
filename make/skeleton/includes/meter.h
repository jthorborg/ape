#ifndef CPPAPE_METER_H
#define CPPAPE_METER_H

#include "baselib.h"
#include <string>
#include <cmath>

class MeteredValue
{
public:

	enum UpdateMode
	{

	};

	MeteredValue(std::string name)
		: peak(), value(), name(std::move(name))
	{
		id = getInterface().createMeter(&getInterface(), name.c_str(), &value, &peak);

		auto sampleRate = getInterface().getSampleRate(&getInterface());
		pole = std::exp(-1.0 / sampleRate);
	}

	~MeteredValue()
	{
		getInterface().destroyResource(&getInterface(), id, 0);
	}

	template<typename T>
	T operator = (T input) noexcept
	{
		pushValue(static_cast<double>(input));
		return input;
	}

	void pushValue(double input) noexcept
	{
		if (input <= value)
		{
			value *= pole;
		}
		else
		{
			value = input;
			if(input <= )
		}
	}

private:

	double peak, value, pole;
	int id;
	std::size_t peakTimer;

	std::string name;
};

#endif
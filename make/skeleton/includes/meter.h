#ifndef CPPAPE_METER_H
#define CPPAPE_METER_H

#include "baselib.h"
#include <string>
#include <cmath>

namespace ape
{
	/// <summary>
	/// A value you can continously assign to (at audio rate), 
	/// and a meter with a decay in the GUI will display this value.
	/// </summary>
	class MeteredValue : public UIObject
	{
	public:
		/// <summary>
		/// Create a new metered value.
		/// </summary>
		/// <param name="name">
		/// Title of this value
		/// </param>
		MeteredValue(std::string name)
			: peak(), value(), peakTimer() /* , name(std::move(name)) */
		{
			id = getInterface().createMeter(&getInterface(), name.c_str(), &value, &peak);

			auto sampleRate = getInterface().getSampleRate(&getInterface());
			pole = std::exp(-1.0 / (0.25 * sampleRate));
			peakStop = static_cast<std::size_t>(sampleRate);
		}

		~MeteredValue()
		{
			getInterface().destroyResource(&getInterface(), id, 0);
		}

		/// <summary>
		/// Alias for <see cref="pushValue"/>
		/// </summary>
		template<typename T>
		T operator = (T input) noexcept
		{
			pushValue(static_cast<double>(input));
			return input;
		}

		/// <summary>
		/// Update the meter with the <paramref name="input"/> value.
		/// This will also calculate peak hold positions and decay envelope.
		/// </summary>
		void pushValue(double input) noexcept
		{
			input = std::abs(input);
			if (input <= value)
			{
				value *= pole;
			}
			else
			{
				value = input;
			}

			if (input <= peak)
			{
				if (peakTimer > peakStop)
				{
					peak *= pole;
				}
				else
				{
					peakTimer++;
				}
			}
			else
			{
				peak = input;
				peakTimer = 0;
			}
		}

	private:

		double peak, value, pole;
		int id;
		std::size_t peakTimer, peakStop;

		//std::string name;
	};

}
#endif
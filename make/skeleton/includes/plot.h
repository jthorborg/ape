#ifndef CPPAPE_PLOT_H
#define CPPAPE_PLOT_H

#include "baselib.h"
#include <string>
#include <cmath>
#include <vector>

namespace ape
{

	template<std::size_t Size>
	class Plot
	{
	public:

		typedef double value_type;

		Plot(std::string name)
			: values(Size)
		{
			id = getInterface().createPlot(&getInterface(), name.c_str(), values.data(), values.size());
		}

		~Plot()
		{
			getInterface().destroyResource(&getInterface(), id, 0);
		}

		value_type& operator [] (std::size_t index)
		{
			if (index < Size)
				return values[index];

			abort("Index out of range for Plot");
		}

		const value_type& operator [] (std::size_t index) const
		{
			if (index < Size)
				return values[index];

			abort("Index out of range for Plot");
		}

		const std::size_t size() const noexcept
		{
			return Size;
		}

	private:

		std::vector<value_type> values;
		int id;
	};
}
#endif
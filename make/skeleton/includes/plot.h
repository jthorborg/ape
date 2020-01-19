#ifndef CPPAPE_PLOT_H
#define CPPAPE_PLOT_H

#include "baselib.h"
#include <string>
#include <cmath>
#include <vector>

namespace ape
{
	/// <summary>
	/// Fixed buffer of <see cref="Plot::value_type"/> elements that is plotted in the UI.
	/// </summary>
	template<std::size_t Size>
	class Plot : public UIObject
	{
	public:

		/// <summary>
		/// The type of value being plotted
		/// </summary>
		typedef double value_type;
		/// <summary>
		/// Assignable value to update the plot
		/// </summary>
		typedef value_type Proxy;

		/// <summary>
		/// Create a plot with a <paramref name="name"/>.
		/// </summary>
		/// <param name="name"></param>
		Plot(std::string name)
			: values(Size)
		{
			id = getInterface().createPlot(&getInterface(), name.c_str(), values.data(), values.size());
		}

		/// <summary>
		/// Destroys a plot.
		/// </summary>
		~Plot()
		{
			getInterface().destroyResource(&getInterface(), id, 0);
		}

		/// <summary>
		/// Writable access to an element in the plot.
		/// </summary>
		Proxy& operator [] (std::size_t index)
		{
			if (index < Size)
				return values[index];

			abort("Index out of range for Plot");
		}

		/// <summary>
		/// Read-only access to an element
		/// </summary>
		const value_type& operator [] (std::size_t index) const
		{
			if (index < Size)
				return values[index];

			abort("Index out of range for Plot");
		}

		/// <summary>
		/// Read-only access to the underlying buffer
		/// </summary>
		const value_type* data() const noexcept
		{
			return values.data();
		}

		/// <summary>
		/// Returns the <typeparamref name="Size"/>
		/// </summary>
		constexpr std::size_t size() const noexcept
		{
			return Size;
		}

	private:

		std::vector<value_type> values;
		int id;
	};
}
#endif
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
	/// <summary>
	/// Template for parameter specializations on different types.
	/// Parameters are based on an normalized <see cref="PFloat"/> 
	/// the host automates, and automatically converts into a user-provided 
	/// semantic <see cref="Range"/> when evaluated at a specific sample.
	/// See <see cref="ParameterBase::at()"/>.
	/// 
	/// The engine automatically provides linear interpolation steps
	/// and flags for discerning changes.
	/// 
	/// Parameters can be user extended, see <see cref="ParameterBase"/>
	/// for required CRTP functionality.
	/// </summary>
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

	/// <summary>
	/// Represents a mapping function in a interval, suitable for evaluation between 0 .. 1 inclusive, and inversely evaluatable given original interval.
	/// Useful for transforming normalized ranges back and forth between a semantic mapping.
	/// </summary>
	class Range
	{
		template<class Type, typename Select>
		friend class Param;

	public:

		/// <summary>
		/// Different curve mappings for the interval
		/// </summary>
		enum Mapping
		{
			/// <summary>
			/// Maps linearly from min to max
			/// </summary>
			Lin,
			/// <summary>
			/// Maps exponentially from min to max.
			/// </summary>
			/// <remarks>
			/// Min and max cannot be zero in this case
			/// </remarks>
			Exp
		};

		/// <summary>
		/// Construct a default linear range from 0 to 1
		/// </summary>
		Range()
			: min(0), max(1), mapping(Lin)
		{

		}

		/// <summary>
		/// Constructs a range from <paramref name="minValue"/> to <paramref name="maxValue"/> with <paramref name="parameterMapping"/> mapping.
		/// </summary>
		Range(PFloat minValue, PFloat maxValue, Mapping parameterMapping = Lin)
			: min(minValue), max(maxValue), mapping(parameterMapping)
		{
			assert(parameterMapping != Exp && minValue != 0 && "Exponential mapping cannot have a minimum of 0");
			assert(parameterMapping != Exp && maxValue != 0 && "Exponential mapping cannot have a maximum of 0");
		}

		/// <summary>
		/// Evaluate the range as a f(x) function, where <paramref name="value"/> is x
		/// <seealso cref="inverse()"/>
		/// </summary>
		PFloat operator()(PFloat value) const noexcept
		{
			if (mapping == Lin)
				return min + value * (max - min);
			else
				return min * pow(max / min, value);
		}

		/// <summary>
		/// Normalizes <paramref name="value"/> back to a 0 .. 1 range.
		/// <seealso cref="operator()()"/>
		/// </summary>
		/// <param name="quantized">
		/// Optionally rounds <paramref name="value"/> before inverse transformation
		/// </param>
		PFloat inverse(bool quantized, PFloat value) const noexcept
		{
			return getNormalizer(quantized)(value, min, max);
		}

		/// <summary>
		/// Retrieve a function pointer with appropriate <see cref="Mapping"/> and selectively <paramref name="quantized"/>.
		/// </summary>
		/// <returns>
		/// A function pointer. Invoking this with <see cref="getMin()"/> and <see cref="getMax()"/> is equivalent to invoking <see cref="operator()()"/>.
		/// </returns>
		APE_Transformer getTransformer(bool quantized) const noexcept
		{
			return !quantized ? (mapping == Lin ? scaleLin : scaleExp) : (mapping == Lin ? scaleLinQ : scaleExpQ);
		}

		/// <summary>
		/// Retrieve a function pointer with appropriate <see cref="Mapping"/> and selectively <paramref name="quantized"/>, for inverse transformation
		/// </summary>
		/// <returns>
		/// A function pointer. Invoking this with <see cref="getMin()"/> and <see cref="getMax()"/> is equivalent to invoking <see cref="inverse()"/>.
		/// </returns>
		APE_Normalizer getNormalizer(bool quantized) const noexcept
		{
			return !quantized ? (mapping == Lin ? invScaleLin : invScaleExp) : (mapping == Lin ? invScaleLinQ : invScaleExpQ);
		}

		/// <summary>
		/// Returns the <see cref="Mapping"/> this range was constructed with
		/// </summary>
		auto getMapping() const noexcept { return mapping; }
		/// <summary>
		/// return the minimum of the constructed range
		/// </summary>
		auto getMin() const noexcept { return min; }
		/// <summary>
		/// Returns  the maximum of the constructed range
		/// </summary>
		auto getMax() const noexcept { return max; }

	private:

		const PFloat min, max;
		const Mapping mapping;
	};

	/// <summary>
	/// Shared functionality for all specializations of parameters.
	/// <seealso cref="ParameterBase"/>
	/// </summary>
	/// <typeparam name="Type">
	/// Any type the <typeparamref name="Derived"/> class can convert and automate.
	/// <seealso cref="Param{float}"/>
	/// </typeparam>
	/// <typeparam name="Derived">
	/// The sub class of this that statically provides the following functions:
	/// <see cref="convert"/>
	/// <see cref="representationFor"/>
	/// </typeparam>
	template<class Type, class Derived>
	class ParameterBase : public UIObject
	{
	protected:

		/// <summary>
		/// Constructs a named parameter.
		/// </summary>
		ParameterBase(const std::string_view paramName = "", const Range parameterRange = Range())
			: name(paramName.size() == 0 ? "Unnamed" : paramName)
			, range(parameterRange)
		{
		}

	public:

		/// <summary>
		/// Convert an engine <see cref="PFloat"/> to the native <typeparamref name="Type"/> of this parameter.
		/// <seealso cref="representationFor"/>
		/// </summary>
		static Type convert(PFloat normalized) noexcept
		{
			return Derived::convert(normalized);
		}

		/// <summary>
		/// Convert a native <typeparamref name="Type"/> to an engine <see cref="PFloat"/> type.
		/// <seealso cref="convert"/>
		/// </summary>
		static PFloat representationFor(Type value) noexcept
		{
			return Derived::representationFor(value);
		}

		/// <summary>
		/// A unique identifier for this parameter
		/// </summary>
		int id() const noexcept
		{
			return param.id;
		}

		/// <summary>
		/// Whether the parameter has changed in this processing frame.
		/// Only sensible when called from within <see cref="Processor::process"/>
		/// </summary>
		/// <remarks>
		/// Only sensible when called from within <see cref="Processor::process"/>
		/// </remarks>
		bool changed() const noexcept
		{
			return param.changeFlags != 0;
		}

		/// <summary>
		/// Evaluate the value of the parameter at the start of this processing frame.
		/// </summary>
		/// <remarks>
		/// Only sensible when called from within <see cref="Processor::process"/>
		/// </remarks>
		operator Type() const noexcept
		{
			return convert(range(param.next));
		}

		/// <summary>
		/// Alias for <see cref="at()"/>.
		/// </summary>
		template<typename Index>
		Type operator [] (Index idx) const noexcept
		{
			return at(idx);
		}

		/// <summary>
		/// Evaluate the value of the parameter at at a specific sample index,
		/// where 0 equals the start of the processing frame.
		/// </summary>
		/// <remarks>
		/// Only sensible when called from within <see cref="Processor::process"/>
		/// </remarks>
		template<typename Index>
		Type at(Index idx) const noexcept
		{
			return convert(range(param.old + param.step * idx));
		}

		/// <summary>
		/// Assign a value to this parameter.
		/// Depending on host support, this might only make sense inside the constructor of a 
		/// <see cref="Effect"/> or <see cref="Generator"/> (so it is a way of setting default 
		/// / initial value of a parameter).
		/// </summary>
		Derived& operator = (Type t) noexcept
		{
			param.next = range.inverse(false, Derived::representationFor(t));
			return static_cast<Derived&>(*this);
		}

		/// <summary>
		/// Delete this parameter instance.
		/// </summary>
		~ParameterBase()
		{
			getInterface().destroyResource(&getInterface(), param.id, -1);
		}

		/// <summary>
		/// Returns the <see cref="Range"/> this parameter was constructed with.
		/// </summary>
		auto getRange() const noexcept { return range; }

	protected:

		APE_Parameter param;
		const Range range;
		std::string name;
	};

	/// <summary>
	/// A parameter suitable for automated boolean values.
	/// <seealso cref="ParameterBase"/>
	/// </summary>
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

	/// <summary>
	/// A parameter suitable for automating enumeration values.
	/// Choices are presented to the user in a combo box.
	/// <seealso cref="ParameterBase"/>
	/// </summary>
	template<typename T>
	class Param<T, typename std::is_enum<T>::type> : public ParameterBase<T, Param<T>>
	{
	public:
		typedef ParameterBase<T, Param<T>> Base;
		typedef T ParameterType;
		/// <summary>
		/// A list-initialization type suitable for a string names of enumerations, e.g.:
		/// <code>
		/// enum class FilterChoice
		/// {
		///		LowPass, HighPass, BandPass
		/// };
		/// constexpr Param<FilterChoice>::Names myFilterNames = {
		///		"LowPass", "HighPass", "BandPass"
		/// };
		/// Param<FilterChoice> myParam {"Type", myFilterNames };
		/// //...
		/// FilterChoice current = myParam;
		/// </code>
		/// </summary>
		typedef std::initializer_list<const char*> Names;
		using Base::operator =;

		/// <summary>
		/// Construct a parameter that can automate a list of string values, and convert them to <typeparamref name="T"/>.
		/// </summary>
		/// <param name="paramName">
		/// The name of this parameter.
		/// </param>
		/// <param name="values">
		/// An initializer list of literal string values.
		/// <code>
		/// Param<MyEnum> myEnum {"Title for my enum", {"choice 1", "choice 2"}};
		/// </code>
		/// <seealso cref="Names"/>.
		/// </param>
		/// <remarks>
		/// Take care to match the number of string values in <paramref name="values"/> to the valid number of
		/// enumeration values in <typeparamref name="T"/>.
		/// </remarks>
		Param(const std::string_view paramName, const std::initializer_list<const char*> values)
			: Base(paramName, Range(0, values.size() - 1))
		{
			getInterface().createListParameter(
				&getInterface(),
				this->name.c_str(),
				&this->param,
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

	/// <summary>
	/// A parameter suitable for automated float values.
	/// <seealso cref="ParameterBase"/>
	/// </summary>
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

	/// <summary>
	/// A parameter suitable for automated double values.
	/// <seealso cref="ParameterBase"/>
	/// </summary>
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

	/// <summary>
	/// A parameter suitable for automated integers.
	/// Note internally the integer is rounded from a floating point <see cref="PFloat"/>
	/// representation, but quantized before evaluated.
	/// <seealso cref="ParameterBase"/>
	/// </summary>
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
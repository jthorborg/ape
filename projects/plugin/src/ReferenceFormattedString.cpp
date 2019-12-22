/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
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

	file:ReferenceFormattedString.cpp
	
		Implementation of the ReferenceFormattedString class.

*************************************************************************************/

#include "ReferenceFormattedString.h"
#include <sstream>

namespace ape 
{
	template<typename T>
	class ReferenceValue : public FormattableValue
	{
	public:

		ReferenceValue(const T * val) : val(val) {}

		void toString(std::stringstream& buf) override
		{
			if (val)
				buf << *val;
		}

	private:
		const T* val;
	};

	/// <summary>
	///	Specialization needed for strings since apparently
	/// not all implementations of stringstream checks for
	///	nullptr - strings
	/// </summary>
	template<>
	class ReferenceValue<const char *> : public FormattableValue
	{
	public:

		ReferenceValue(const char* val) : val(val) {}

		void toString(std::stringstream& buf) override
		{
			if (val && *val)
				buf << *val;
		}

	private:
		const char* val;
	};

	template<typename T, class VaList>
    typename std::enable_if<!std::is_same<T, const char*>::value, std::unique_ptr<ReferenceValue<T>>>::type ValueFrom(VaList& args)
	{
		return std::make_unique<ReferenceValue<T>>(va_arg(args, const T *));
	}

    template<typename T, class VaList>
    typename std::enable_if<std::is_same<T, const char*>::value, std::unique_ptr<ReferenceValue<T>>>::type ValueFrom(VaList& args)
	{
		return std::make_unique<ReferenceValue<const char*>>(va_arg(args, const char*));
	}

	ReferenceFormattedString::ReferenceFormattedString(const char * fmt, ...)
		: format(fmt)
	{
		va_list args;
		va_start(args, fmt);

		getValues(args);

		va_end(args);
	}

	ReferenceFormattedString::ReferenceFormattedString(const char * fmt, va_list args)
		: format(fmt)
	{
		getValues(args);
	}

	ReferenceFormattedString::ReferenceFormattedString(ReferenceFormattedString&& other)
		: format(std::move(other.format))
		, valueList(std::move(other.valueList))
	{

	}


	ReferenceFormattedString::~ReferenceFormattedString()
	{

	}

	void ReferenceFormattedString::getValues(va_list args)
	{

		if(!format.length())
			return;

        for(std::size_t i = 0; i < format.size(); ++i) 
		{
			if (format[i] != '%')
				continue;

			auto delta = format.size() - i;

			if (delta == 1)
				break;

            ++i;

            switch(format[i])
            {
			case 'x':
                valueList.emplace_back(ValueFrom<void*>(args));
				break;

            case 'u':
                valueList.emplace_back(ValueFrom<unsigned int>(args));
                break;

			case 'i':
            case 'd':
                valueList.emplace_back(ValueFrom<signed int>(args));
                break;

            case 'f':
                valueList.emplace_back(ValueFrom<float>(args));
                break;

            case 'l':

				if (delta == 2)
					break;

				++i;

				// %lf is used for doubles, since float * isn't promoted to double, 
				// unlike float -> double when passing by value
				if (format[i] == 'f')
					valueList.emplace_back(ValueFrom<double>(args));
				else if (format[i] == 'd')
					valueList.emplace_back(ValueFrom<long double>(args));
				else if (format[i] == 'u')
					valueList.emplace_back(ValueFrom<unsigned long long>(args));
				else if (format[i] == 'i')
					valueList.emplace_back(ValueFrom<signed long long>(args));
				else
					--i;
                break;

			case 'c':
                valueList.emplace_back(ValueFrom<char>(args));
                break;

			case 's':
                valueList.emplace_back(ValueFrom<const char*>(args));
				break;
            }
        }
	}

	std::string ReferenceFormattedString::get() 
	{
		std::stringstream ss;
		std::size_t count(0);

		for(std::size_t i = 0; i < format.size(); i++) 
		{
			auto delta = format.size() - i;

			if(format[i] == '%') 
			{
				// ignore type specifiers, already polymorphically solved
				i++; 

				if (delta == 1)
					break;

				// fix for %l<> specifiers.
				if(format[i] == 'l')
					i++;

				if (delta == 2)
					break;

				// special case: %% is interpreted as a single %
				if(format[i] != '%' && count < valueList.size()) 
				{ 
					valueList[count++]->toString(ss); // magic
					continue;
				}
			}

			ss << format[i];
		}

		return ss.str();
	}
}

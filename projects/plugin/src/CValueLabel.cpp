/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2014 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:CValueLabel.cpp
	
		Implementation of the CValueLabel class.

*************************************************************************************/

#include "CValueLabel.h"

namespace APE 
{
	/*********************************************************************************************

	 	Inserts the pointers to the values of fmt string from args into the valueList.
		Note: setFormat must be called before this.

	 *********************************************************************************************/
	void CValueLabel::getValues(va_list args)
	{
		if(!format.length())
			return;
        for(unsigned i = 0; i < format.size(); ++i) {
            if(format[i] == '%') {
                ++i;
                switch(format[i])
                {
				case 'x':
                    valueList.push_back(	new CValue<void *>		( va_arg(args, void *) ));
					break;
                case 'u':
                    valueList.push_back(	new CValue<unsigned int>( va_arg(args, void *) ));
                    break;
				case 'i':
                case 'd':
                    valueList.push_back(	new CValue<signed int>	( va_arg(args, void *) ));
                    break;
                case 'f':
                    valueList.push_back(	new CValue<float>		( va_arg(args, void *) ));
                    break;
                case 'l':
					// %lf is used for doubles, since float * isn't promoted to double (obviously), 
					// unlike float -> double when passing
					if(format[i + 1] == 'f') {
						valueList.push_back(new CValue<double>		( va_arg(args, void *) ));
						++i;
					}
					else if (format[i + 1] == 'd') {
						valueList.push_back(new CValue<long double>	( va_arg(args, void *) ));
						++i;
					}
					else if (format[i + 1] == 'u') {
						valueList.push_back(new CValue<signed long long>(va_arg(args, void *)));
						++i;
					}
					else if (format[i + 1] == 'i') {
						valueList.push_back(new CValue<unsigned long long>(va_arg(args, void *)));
						++i;
					}
                    break;
				case 'c':
                    valueList.push_back(	new CValue<char>		( va_arg(args, void *) ));
                    break;
				case 's':
                    valueList.push_back(	new CValue<char *>		( va_arg(args, void *) ));
					break;
                }
            }
        }
	}

	/*********************************************************************************************

	 	Resets the state of the valueLabel.

	 *********************************************************************************************/
	void CValueLabel::reset() 
	{
		format.clear();
		ss.str("");
		for(unsigned i = 0; i < valueList.size(); i++) {
			delete valueList[i];
		}
		valueList.clear();
	}

	/*********************************************************************************************

	 	Sets the format string and calls getValues()

	 *********************************************************************************************/
	void CValueLabel::setFormat(const char * fmt, ...) 
	{

		reset();
		format = fmt;
		va_list args;
		va_start(args, fmt);

		getValues(args);

		va_end(args);

	}
	/*********************************************************************************************

	 	Sets the format string and calls getValues()

	 *********************************************************************************************/
	void CValueLabel::setFormat(const char * fmt, va_list args)
	{
		reset();
		format = fmt;
		getValues(args);
	}
	/*********************************************************************************************

	 	Returns the string as formatted by the formatstring, with the additional arguments given.

	 *********************************************************************************************/
	std::string CValueLabel::get() {

		ss.str("");
		unsigned count(0);

		for(unsigned i = 0; i < format.size(); i++) {
			if(format[i] == '%') {
				i++; // ignore type specifiers, already polymorphically solved
				// fix for %l<> specifiers.
				if(format[i] == 'l')
					i++;
				if(format[i] != '%') { // special case: %% is interpreted as a single %
					valueList[count++]->toString(ss); // magic
					continue;
				}
			}
			ss << format[i];
		}
		return ss.str();
	}
	/*********************************************************************************************

	 	Destructor

	 *********************************************************************************************/
	CValueLabel::~CValueLabel() {
		reset();
	}
}
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

	file:CValueLabel.h
		
		Declares the interface the CValueLabel class. This is a special label,
		initialized with a printf-alike string - except it keeps pointers
		to the arguments given and formats a new string with updated values
		each time CValueLabel::get() is called.

*************************************************************************************/

#ifndef _CVALUELABEL_H
	#define _CVALUELABEL_H

	#include <string>
	#include <iostream>
	#include <vector>
	#include <sstream>
	#include <stdarg.h> 

	namespace ape {

		class CBaseValue 
		{
		public:
			virtual void toString(std::stringstream & buf) = 0;
			virtual ~CBaseValue() {};
		};

		template< typename T >
			class CValue : public CBaseValue
			{
				typedef T type;
				typedef T * ptr_type;
				type * val;

			public:
				CValue(void * val) 
				{
					this->val = reinterpret_cast<ptr_type>(val);
				}

				CValue(type * val) : val(val) {}

				virtual void toString(std::stringstream & buf) {
					if(val)
						buf << *val;
				}
			};
			/*
				Specialization needed for strings since apparantly
				not all implementations of stringstream checks for 
				nullptr-strings
			*/
		template<>
			class CValue<char *> : public CBaseValue
			{
				typedef char * type;
				typedef char * * ptr_type;
				type * val;

			public:
				CValue(void * val) 
				{
					this->val = reinterpret_cast<ptr_type>(val);
				}

				CValue(type * val) : val(val) {}

				virtual void toString(std::stringstream & buf) {
					if(val && *val)
						buf << *val;
				}
			};

		class CValueLabel 
		{

			std::stringstream ss;
			std::vector<CBaseValue *> valueList;
			std::string format;

			void getValues(va_list args);

		public:

			CValueLabel() {};
			void reset();
			void setFormat(const char * fmt, ...);
			void setFormat(const char * fmt, va_list args);
			std::string get();
			~CValueLabel();
		};
	} // namespace ape
#endif
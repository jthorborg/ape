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

	file:ReferenceFormattedString.h
		
		Declares the interface the ReferenceFormattedString class. This is a special string,
		initialized with a printf-alike string - except it keeps pointers
		to the arguments given and formats a new string with updated values
		each time ReferenceFormattedString::get() is called.

*************************************************************************************/

#ifndef APE_REFERENCEFORMATTEDSTRING_H
	#define APE_REFERENCEFORMATTEDSTRING_H

	#include <string>
	#include <vector>
	#include <stdarg.h> 
	#include <memory>


	namespace ape 
	{
		class FormattableValue
		{
		public:
			virtual void toString(std::stringstream & buf) = 0;
			virtual ~FormattableValue() {};
		};

		class ReferenceFormattedString 
		{
		public:

			ReferenceFormattedString(const char* fmt, ...);
			ReferenceFormattedString(const char* fmt, va_list args);
			ReferenceFormattedString(ReferenceFormattedString&& other);
			std::string get();

			~ReferenceFormattedString();

		private:

			void getValues(va_list args);
			std::vector<std::unique_ptr<FormattableValue>> valueList;
			std::string format;

		};
	} // namespace ape
#endif
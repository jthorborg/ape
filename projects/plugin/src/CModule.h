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

	file:CModule.h
		
		Interface for the wrapper class CModule. CModule wraps loading and dynamic
		binding of external libraries like DLLs, dylibs, SOs etc. with safe
		copy construction (using reference counting abilities of underlying OS).

	options:
		#define _CMOD_USECF
			uses corefoundation instead of the dyld loader on mac (advised)
*************************************************************************************/


#ifndef _CMODULE_H

	#define _CMODULE_H

	#include <string>
	// if set, uses corefoundation instead of the dyld loader on mac
	#define _CMOD_USECF
	typedef void * ModuleHandle;

	namespace APE
	{
		class CModule
		{
			ModuleHandle module;
			std::string name;
		public:
			CModule();
			CModule(const std::string & moduleName);
			void * getFuncAddress(const std::string & functionName);
			int load(const std::string & moduleName);
			void increaseReference();
			void decreaseReference();
			bool release();
			ModuleHandle getHandle();
			CModule(const CModule &);
			~CModule();
		};
	}
#endif
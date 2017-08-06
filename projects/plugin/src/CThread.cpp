/*************************************************************************************

	Audio Programming Environment VST. 
		
		VST is a trademark of Steinberg Media Technologies GmbH.

    Copyright (C) 2013 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:Misc.cpp
		
		Implementation of CModule.h

*************************************************************************************/

#include "CModule.h"
#include "PlatformSpecific.h"

namespace APE
{
	CModule::CModule() 
		: module(nullptr)
	{	
			
	}
	CModule::CModule(const std::string & moduleName)
		: module(nullptr)
	{
		load(moduleName);
	}

	void * CModule::getFuncAddress(const std::string & functionName)
	{ 
		#ifdef __WINDOWS__
			return GetProcAddress(static_cast<HMODULE>(module), functionName.c_str());
		#elif defined(__MAC__)
			return dlsym(module, functionName.c_str());
		#else
			#error no implementation for getfuncaddress on anything but windows
		#endif
		return nullptr;
	}

	int CModule::load(const std::string & moduleName)
	{
		if(module != nullptr)
			return false;
		name = moduleName;
		#ifdef _WINDOWS_
			module = static_cast<ModuleHandle>(LoadLibraryA(moduleName.c_str()));
			if(module != nullptr)
				return 0;
			else
				return GetLastError();
		#elif defined(__MAC__)
			module = dlopen(moduleName.c_str(), RTLD_NOW);
			if(module != nullptr)
				return 0;
			else
				return 1;
				
		#else
			#error no implementation for LoadModule on anything but windows
		#endif
		return 0;
	};

	bool CModule::release()
	{
		if(module == nullptr)
			return false;
		bool result = false;
		#ifdef _WINDOWS_
			result = !!FreeLibrary(static_cast<HMODULE>(module));
			module = nullptr;
		#elif defined(__MAC__)
			result = !dlclose(module);
		#else
			#error no implementation for LoadModule on anything but windows
		#endif
		return result;
	}
	ModuleHandle CModule::getHandle()
	{
		return module;
	}
	void CModule::decreaseReference()
	{
		if(module == nullptr)
			return;
		#ifdef _WINDOWS_
			FreeLibrary(static_cast<HMODULE>(module));
		#elif defined(__MAC__)
			dlclose(module);
		#else
			#error no implementation for LoadModule on anything but windows
		#endif
	}
	void CModule::increaseReference()
	{
		if(module && name.length()) {
			#ifdef _WINDOWS_
				// only increases reference count
				LoadLibraryA(name.c_str());
			#elif defined(__MAC__)
				dlopen(name.c_str(), RTLD_NOW);
			#else
				#error no implementation for LoadModule on anything but windows
			#endif
		}
	}
	CModule::~CModule()
	{
		release();
	}
	CModule::CModule(const CModule & other)
	{
		this->module = other.module;
		this->name = other.name;
		increaseReference();
	}
};
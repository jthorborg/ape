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

	file:CModule.cpp
		
		Implementation of CModule.h

*************************************************************************************/

#include "CModule.h"
#include "PlatformSpecific.h"

namespace APE
{
	/*********************************************************************************************
	 
		Constructor.
	 
	 *********************************************************************************************/
	CModule::CModule() 
		: module(nullptr)
	{	
			
	}
	/*********************************************************************************************
	 
		Constructor that loads a module from a name
	 
	 *********************************************************************************************/
	CModule::CModule(const std::string & moduleName)
		: module(nullptr)
	{
		load(moduleName);
	}
	/*********************************************************************************************
	 
		Returns a pointer to a symbol inside the loaded module of this instance
	 
	 *********************************************************************************************/
	void * CModule::getFuncAddress(const std::string & functionName)
	{ 
		#ifdef __WINDOWS__
			return GetProcAddress(static_cast<HMODULE>(module), functionName.c_str());
		#elif defined(__MAC__) && defined(_CMOD_USECF)
			CFStringRef funcName = CFStringCreateWithCString(kCFAllocatorDefault, functionName.c_str(), kCFStringEncodingUTF8);
			void * ret(nullptr);
			if(funcName)
			{
				CFBundleRef bundle(nullptr);
				bundle = reinterpret_cast<CFBundleRef>(module);
				if(bundle)
				{
					ret = reinterpret_cast<void*>(CFBundleGetFunctionPointerForName(bundle, funcName));
				}
				CFRelease(funcName);
				
			}
			return ret;
		#elif defined(__MAC__)
			return dlsym(module, functionName.c_str());
		#else
			#error no implementation for getfuncaddress on anything but windows
		#endif
	}
	/*********************************************************************************************
	 
		If no module is loaded, loads moduleName
	 
	 *********************************************************************************************/
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
		#elif defined(__MAC__) && defined(_CMOD_USECF)
			/*
				This api is only slightly better than old-style K&R c-programming
			*/
			CFBundleRef bundle(nullptr);
			CFURLRef url(nullptr);
			CFStringRef path = CFStringCreateWithCString(kCFAllocatorDefault, name.c_str(), kCFStringEncodingUTF8);
			if(!path)
				return -1;
			url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, path, kCFURLPOSIXPathStyle, true);
			// remember to release all of this stuff.
			CFRelease(path);
			if(!url)
			{
				return -2;
			}
			bundle = CFBundleCreate( kCFAllocatorDefault, url );
			CFRelease(url);	
			if(!bundle)
			{
				return -3;
			}
			else
			{
				// standard clearly says reinterpret_cast to void * and back is welldefined
				module = reinterpret_cast<void *>(bundle);
				return 0;
			}
		#elif defined(__MAC__)
			module = dlopen(moduleName.c_str(), RTLD_NOW);
			if(module != nullptr)
				return 0;
			else
				return errno ? errno : -1;
				
		#else
			#error no implementation for LoadModule on anything but windows
		#endif
	};
	/*********************************************************************************************
	 
		Releases the module, decreasing it's reference count and losing the reference.
	 
	 *********************************************************************************************/
	bool CModule::release()
	{
		if(module == nullptr)
			return false;
		bool result = false;
		#ifdef _WINDOWS_
			result = !!FreeLibrary(static_cast<HMODULE>(module));
			module = nullptr;
		#elif defined(__MAC__) && defined(_CMOD_USECF)
			CFBundleRef bundle = nullptr;
			bundle = reinterpret_cast<CFBundleRef>(module);
			if(bundle)
			{
				CFRelease(bundle);
				module = nullptr;
				bundle = nullptr;
				return true;
			}
			else
			{
				module = nullptr;
				return false;
			}
		#elif defined(__MAC__)
			result = !dlclose(module);
		#else
			#error no implementation for LoadModule on anything but windows
		#endif
		return result;
	}
	/*********************************************************************************************
	 
		Returns the native handle to the module
	 
	 *********************************************************************************************/
	ModuleHandle CModule::getHandle()
	{
		return module;
	}
	/*********************************************************************************************
	 
		Decreases the reference count of the loaded module.
	 
	 *********************************************************************************************/
	void CModule::decreaseReference()
	{
		if(module == nullptr)
			return;
		#ifdef _WINDOWS_
			FreeLibrary(static_cast<HMODULE>(module));
		#elif defined(__MAC__) && defined(_CMOD_USECF)
			// so long as we dont use CFBundleLoadExecutable, references should be automatically counted by
			// core foundation, as long as we remember to release() the module and increaseReference() it
		#elif defined(__MAC__)
			dlclose(module);
		#else
			#error no implementation for LoadModule on anything but windows
		#endif
	}
	/*********************************************************************************************
	 
		Increases the reference of the loaded module
	 
	 *********************************************************************************************/
	void CModule::increaseReference()
	{
		if(module && name.length()) {
			#ifdef _WINDOWS_
				// only increases reference count
				LoadLibraryA(name.c_str());
			#elif defined(__MAC__) && defined(_CMOD_USECF)
				/*
					little bit of double code, but sligthly different semantics
					ie. keeps the old module if new one fails.
				*/
				CFBundleRef bundle(nullptr);
				CFURLRef url(nullptr);
				CFStringRef path = CFStringCreateWithCString(kCFAllocatorDefault, name.c_str(), kCFStringEncodingUTF8);
				if(!path)
					return;
				url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, path, kCFURLPOSIXPathStyle, true);
				// remember to release all of this stuff.
				CFRelease(path);
				if(!url)
				{
					return;
				}
				// this should increase reference count for the same bundle.
				// the reference is decreased when we CFRelease the bundle.
				bundle = CFBundleCreate( kCFAllocatorDefault, url );
				CFRelease(url);
				if(!bundle)
				{
					return;
				}
				else
				{
					module = reinterpret_cast<void*>(bundle);
				}
			#elif defined(__MAC__)
				dlopen(name.c_str(), RTLD_NOW);
			#else
				#error no implementation for LoadModule on anything but windows
			#endif
		}
	}
	/*********************************************************************************************
	 
		Destructor. Releases the module
	 
	 *********************************************************************************************/
	CModule::~CModule()
	{
		release();
	}
	/*********************************************************************************************
	 
		Copy constructor
	 
	 *********************************************************************************************/
	CModule::CModule(const CModule & other)
	{
		this->module = other.module;
		this->name = other.name;
		increaseReference();
	}
};
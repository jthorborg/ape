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

	file:CState.cpp
	
		Implementation of CState.h

*************************************************************************************/

#include <cpl/PlatformSpecific.h>
#include "SharedInterfaceEx.h"
#include "CState.h"
#include "Engine.h"
#include "CCodeGenerator.h"
#include "GraphicUI.h"
#include "CConsole.h"
#include <sstream>
#include <cpl/Protected.h>

namespace ape
{
	thread_local unsigned int fpuMask;

	CState::CState(Engine * engine) 
		: engine(engine), curProject(nullptr)
	{
		generator = std::make_unique<CCodeGenerator>(engine);
		generator->setErrorFunc(engine->errPrint, engine);
		createSharedObject();
		#ifndef __WINDOWS__
			registerHandlers();
		#endif
	}

	CState::~CState() 
	{
		if (curProject)  
		{
			generator->releaseProject(*curProject);
		}

	}

	void CState::createSharedObject() 
	{
		sharedObject = std::make_unique<SharedInterfaceEx>(*engine, *this);
	}

	SharedInterfaceEx & CState::getSharedInterface()
	{
		return *sharedObject.get();
	}

	void CState::setBounds(std::size_t numInputs, std::size_t numOutputs, std::size_t blockSize)
	{
		while (protectedMemory.size() < 2)
			protectedMemory.emplace_back().setProtect(CMemoryGuard::protection::readwrite);

		if (!protectedMemory[0].resize<float>(numInputs * blockSize) || !protectedMemory[1].resize<float>(numOutputs * blockSize))
			CPL_SYSTEM_EXCEPTION("Error allocating virtual protected memory for buffers");
	}

	/*********************************************************************************************

		Getter for the plugin allocator.

	 *********************************************************************************************/
	ape::CAllocator & CState::getPluginAllocator() 
	{
		return pluginAllocator;
	}
	/*********************************************************************************************

		Getter for the protected memory.

	 *********************************************************************************************/
	std::vector<ape::CMemoryGuard> & CState::getPMemory() 
	{
		return protectedMemory;
	}

	/*********************************************************************************************

		Tells compiler to compile this->project.

	 *********************************************************************************************/
	bool CState::compileCurrentProject()
	{
		if (curProject)
		{
			this->curProject->iface = sharedObject.get();
			if (!generator->compileProject(*curProject))
				return false;
			if (!generator->initProject(*curProject))
				return false;
		}
		return true;
	}
	/*********************************************************************************************

		Calls compilers activate function.

	 *********************************************************************************************/
	Status CState::activateProject()
	{
		Status ret = Status::STATUS_ERROR;
		cpl::CProtected::instance().runProtectedCode( 
			[&]
			{
				ret = generator->activateProject(*curProject);
			}
		);
		return ret;
	}
	/*********************************************************************************************

		Calls compilers disable function.

	 *********************************************************************************************/
	Status CState::disableProject(bool didMisbehave)
	{
		Status ret = Status::STATUS_ERROR;
		
		if (curProject)
		{
			cpl::CProtected::instance().runProtectedCode(
				[&]
				{
					ret = generator->disableProject(*curProject, didMisbehave);
				}
			);
		}

		return ret;
	}
	/*********************************************************************************************

		Calls plugin's processReplacer

	 *********************************************************************************************/
	Status CState::processReplacing(float ** in, float ** out, std::size_t sampleFrames)
	{
		Status ret = Status::STATUS_ERROR;
		if (curProject)
		{
			cpl::CProtected::instance().runProtectedCode(
				[&]
				{
					ret = generator->processReplacing(*curProject, in, out, sampleFrames);
				}
			);
		}
		return ret;

	}
	/*********************************************************************************************

		Calls plugin's onEvent handler

	 *********************************************************************************************/
	Status CState::onEvent(Event * e)
	{
		Status ret = Status::STATUS_ERROR;
		if (curProject)
		{
			cpl::CProtected::instance().runProtectedCode(
				[&]
				{
					ret = generator->onEvent(*curProject, e);
				}
			);
		}
		return ret;
	}
	/*********************************************************************************************

		Releases old project and updates it to new one. Does not check whether project is used!!

	 *********************************************************************************************/
	void CState::setNewProject(ape::ProjectEx * project)
	{
		if(curProject)  
		{
			generator->releaseProject(*curProject);
		}
		curProject.reset(project);
	}

	/*
		unless you want to have a heart attack, dont look at this code.
	*/
	#ifdef __MSVC__
		#pragma fenv_access (on)
	#else
		#pragma STDC FENV_ACCESS on
	#endif
	/*********************************************************************************************

		Toggle floating point exceptions on and off.

	 *********************************************************************************************/
	void CState::useFPUExceptions(bool bVal) 
	{
		// always call this!!#
		#ifdef CPL_MSVC
			_clearfp();
			unsigned int nfpcw = 0;
			if(bVal)
			{
				unsigned int temp;
				// save old environment
				_controlfp_s(&fpuMask, 0, 0);

				// Make the new fp env same as the old one,
				// except for the changes we're going to make
				nfpcw = _EM_INVALID | _EM_DENORMAL | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW; 
				//Update the control word with our changes
				_controlfp_s(&temp, 1, nfpcw);
			}
			else 
			{
				_controlfp_s(&nfpcw, 1, fpuMask);
			}
		#else
			if(bVal)
			{
				// always call this - changes in masks may trigger exceptions
				std::feclearexcept(FE_ALL_EXCEPT);
				threadData.fpuMask = _MM_GET_EXCEPTION_MASK();
				
				unsigned nfpcw = _MM_MASK_INVALID | _MM_MASK_DENORM | _MM_MASK_DIV_ZERO | _MM_MASK_OVERFLOW
								| _MM_MASK_UNDERFLOW |_MM_MASK_INEXACT;
				
				_MM_SET_EXCEPTION_MASK(fpuMask & ~nfpcw);

			}
			else
			{
				// always call this - changes in masks may trigger exceptions
				std::feclearexcept(FE_ALL_EXCEPT);
				// check return value here?
				_MM_SET_EXCEPTION_MASK(fpuMask);
			}
		#endif
	}

	void CState::projectCrashed()
	{
		curProject->state = CodeState::Disabled;
	}

}
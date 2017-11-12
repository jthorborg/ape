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

	file:CState.h
	
		Implements interface for the c-subsystem-controller and communication between
		them; handles compiling and run-time resolution of dispatches from C to C++ 
		member functions, as well as wrapping unsafe code turning hardware exceptions
		into the software exception CSystemException. Allows setting of FPU environment.

*************************************************************************************/

#ifndef APE_CSTATE_H
	#define APE_CSTATE_H

	#include <cpl/PlatformSpecific.h>
 #include <cpl/MacroConstants.h>
	#include "CMemoryGuard.h"
	#include <vector>
	#include <exception>
	#include <signal.h>
	#include "CAllocator.h"
	#include <ape/Project.h>
	#include <ape/Events.h>
	#include <thread>
	#include <cpl/CMutex.h>
	#include "ProjectEx.h"

	namespace ape
	{

		typedef void * RawFunctionPtr;
		typedef int VstInt32;

		class Engine;
		struct Module;
		class CState;
		class CCodeGenerator;
		struct SharedInterfaceEx;
		
		/*
			This class is responsible for interconnecting the C and C++ systems, dispatch calls correctly and hold the state of the C-system
		*/			
		class CState
		{
		public:

			SharedInterfaceEx & getSharedInterface();
			CAllocator & getPluginAllocator();
			std::vector<CMemoryGuard> & getPMemory();

			void setBounds(std::size_t numInputs, std::size_t numOutputs, std::size_t blockSize);

			CState(ape::Engine * effect);
			~CState();

			/*
				utility

			*/
			bool compileCurrentProject();
			void setNewProject(ProjectEx *);
			static void useFPUExceptions(bool b);
			void projectCrashed();

			/*
				safe wrappers around compiler
			*/
			Status activateProject();
			Status disableProject(bool didMisbehave);
			Status processReplacing(float ** in, float ** out, std::size_t sampleFrames);
			Status onEvent(Event * e);

		private:

			void createSharedObject();

			 
			std::unique_ptr<SharedInterfaceEx> sharedObject;
			std::unique_ptr<CCodeGenerator> generator;
			Engine * engine;
			CAllocator pluginAllocator;
			std::vector<CMemoryGuard> protectedMemory;

			std::unique_ptr<ProjectEx, ProjectDeleter> curProject;



		};
	};
#endif
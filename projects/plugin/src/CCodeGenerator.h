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

	file:CCodeGenerator.h
		File and classes responsible for responsing to Engine and CState's requests
		to compilations and access to plugins.

*************************************************************************************/

#ifndef _CCODEGENERATOR_H
	#define _CCODEGENERATOR_H

	#include "MacroConstants.h"
	#include <string>
	#include "CApi.h"
	#include <map>
	#include <vector>
	#include "Settings.h"
	#include "ProjectEx.h"
	#include <cassert>
	#include "Misc.h"
	#include "CModule.h"
	#include <ape/Events.h>

	namespace APE
	{
		class Engine;
		class CCodeGenerator;

		/*
			indexer for g_sExports
		*/
		enum EExports : unsigned
		{
			GetSymbol,
			CompileProject,
			SetErrorFunc,
			FreeProject,
			InitProject,
			GetState,
			AddSymbol,
			processReplacing,
			onEvent,
			zzz,
			eDummy

		};
		/*
			The exported symbol names for external compilers
		*/
		static const char * g_sExports[] =
		{
			"GetSymbol",
			"CompileProject",
			"ReleaseProject",
			"InitProject",
			"ActivateProject",
			"DisableProject",
			"GetState",
			"AddSymbol",
			"ProcessReplacing",
			"OnEvent"
		};
		/*
			A compiler instance.
		*/
		struct CCompiler
		{
			friend class CCodeGenerator;

		public:
			bool isInitialized() { return initialized; }
			bool compileProject(ProjectEx * project);
			// calls loadBindings() with settings
			bool initialize(const libconfig::Setting & languageSettings);
			virtual ~CCompiler() {};
			CCompiler();

		private:

			struct CBindings
			{
#pragma message("fix")
				// c-bindings
				typedef void *	(APE_STD_API * getSymbol_t)		(APE_Project * project, const char * symbol);
				typedef Status	(APE_STD_API * compileProject_t)(APE_Project * project, const void * opaque, ErrorFunc errorFunc);
				typedef Status	(APE_STD_API * releaseProject_t)(APE_Project * project);
				typedef Status	(APE_STD_API * initProject_t)	(APE_Project * project);
				typedef Status	(APE_STD_API * activateProject_t)(APE_Project * project);
				typedef Status	(APE_STD_API * disableProject_t)(APE_Project * project);
				typedef Status	(APE_STD_API * getState_t)		(APE_Project * project);
				typedef Status	(APE_STD_API * addSymbol_t)		(APE_Project * project, const char * symbol, const void * mem);
				typedef Status	(APE_STD_API * processReplacing_t)(APE_Project * project, Float ** in, Float ** out, Int sampleFrames);
				typedef Status	(APE_STD_API * onEvent_t)		(APE_Project * project, APE_Event * );

				bool valid;

				union {
					struct {
						getSymbol_t getSymbol;
						compileProject_t compileProject;
						releaseProject_t releaseProject;
						initProject_t initProject;
						activateProject_t activateProject;
						disableProject_t disableProject;
						getState_t getState;
						addSymbol_t addSymbol;
						processReplacing_t processReplacing;
						onEvent_t onEvent;
					};
					/* sizeof(previous struct) / sizeof (void*) */
					void * _table[EExports::eDummy];
				};

				bool loadBindings(CModule & module, const libconfig::Setting & exportSettings);

				CBindings();
			};

			bool initialized;
			CModule module;
			CBindings bindings;
			std::vector<std::string> extensions;
			std::string language;
			std::string compilerName;
			std::string compilerPath;
		};

		/*
			Class responsible for managing compilers opaquely and dispatching
			call requests to the matching compilers, according to the current project
		*/
		class CCodeGenerator
		{
			ErrorFunc errorPrinter;
			void * opaque;
			std::map<std::string, CCompiler> compilers;
			APE::Engine * engine;

		public:

			CCodeGenerator(APE::Engine * engine);
			void setErrorFunc(ErrorFunc f, void * op);
			void printError(const std::string & message);

#pragma message("wtf")
			template <typename type>
				type getSymbol(ProjectEx * project, const std::string & name)
				{
					assert(0 && "Shouldn't reach this point (deprecated)");
					return (type)nullptr;
				}
			template <typename type>
				int addSymbol(ProjectEx * project, const std::string & name, type symbol) {
					assert(0 && "Shouldn't reach this point (deprecated)");
					return (type)nullptr;
				}

			/*
				Important: Following functions must be RAII-free.
				It should be considered that the system might throw
				system exceptions anywhere in these functions.
				They are caught in the CState wrapper.
			*/
			Status activateProject(ProjectEx * project);
			Status processReplacing(ProjectEx * project, Float ** in, Float ** out, Int sampleFrames);
			Status disableProject(ProjectEx * project);
			Status onEvent(ProjectEx * project, Event * e);

			bool compileProject(ProjectEx * project);
			bool initProject(ProjectEx * project);
			bool releaseProject(ProjectEx * project);

		};
	};
#endif
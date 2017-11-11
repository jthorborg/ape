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

	#include <cpl/CModule.h>
	#include "MacroConstants.h"
	#include <string>
	#include "CApi.h"
	#include <map>
	#include <vector>
	#include "Settings.h"
	#include "ProjectEx.h"
	#include <cassert>
	#include <ape/Events.h>
	#include <ape/CompilerBindings.h>

	namespace APE
	{
		class Engine;
		class CCodeGenerator;

		/*
			indexer for g_sExports
		*/
		enum class ExportIndex
		{
			GetSymbol,
			CompileProject,
			ReleaseProject,
			InitProject,
			ActivateProject,
			DisableProject,
			GetState,
			AddSymbol,
			ProcessReplacing,
			OnEvent,
			end
		};

		constexpr inline std::size_t MaxExports() { return static_cast<std::size_t>(ExportIndex::end); }

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
				bool valid;

				union {
					struct {
						decltype(GetSymbol) * getSymbol;
						decltype(CompileProject) * compileProject;
						decltype(ReleaseProject) * releaseProject;
						decltype(InitProject) * initProject;
						decltype(ActivateProject) * activateProject;
						decltype(DisableProject) * disableProject;
						decltype(GetState) * getState;
						decltype(AddSymbol) * addSymbol;
						decltype(ProcessReplacing) * processReplacing;
						decltype(OnEvent) * onEvent;
					};
					/* sizeof(previous struct) / sizeof (void*) */
					void * _table[MaxExports()];
				};

				bool loadBindings(cpl::CModule & module, const libconfig::Setting & exportSettings);

				CBindings();
			};

			bool initialized;
			cpl::CModule module;
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

			/*
				Important: Following functions must be RAII-free.
				It should be considered that the system might throw
				system exceptions anywhere in these functions.
				They are caught in the CState wrapper.
			*/
			Status activateProject(ProjectEx * project);
			Status processReplacing(ProjectEx * project, Float ** in, Float ** out, Int sampleFrames);
			Status disableProject(ProjectEx * project, bool didMisbehave);
			Status onEvent(ProjectEx * project, Event * e);

			bool compileProject(ProjectEx * project);
			bool initProject(ProjectEx * project);
			bool releaseProject(ProjectEx * project);

		};
	};
#endif
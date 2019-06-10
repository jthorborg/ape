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
		File and classes responsible for responsing to Engine and PluginState's requests
		to compilations and access to plugins.

*************************************************************************************/

#ifndef _CCODEGENERATOR_H
	#define _CCODEGENERATOR_H

	#include <cpl/CModule.h>
	#include <cpl/MacroConstants.h>
	#include <cpl/Core.h>
	#include <string>
	#include "CApi.h"
	#include <map>
	#include <vector>
	#include "Settings.h"
	#include "ProjectEx.h"
	#include <cassert>
	#include <ape/Events.h>
	#include <ape/CompilerBindings.h>

	namespace ape
	{
		class Engine;
        class CompilerBinding;

		/// <summary>
		/// Class responsible for managing compilers opaquely and dispatching
		/// call requests to the matching compilers, according to the current project
		/// </summary>
		class CCodeGenerator
		{
		public:

			CCodeGenerator(ape::Engine& engine);
            ~CCodeGenerator();
            
			/*
				Important: Following functions must be RAII-free.
				It should be considered that the system might throw
				system exceptions anywhere in these functions.
				They are caught in the PluginState wrapper.
			*/
			Status activateProject(ProjectEx & project);
			Status processReplacing(ProjectEx & project, float ** in, float ** out, std::size_t sampleFrames);
			Status disableProject(ProjectEx & project, bool didMisbehave);
			Status onEvent(ProjectEx & project, Event * e);


			bool compileProject(ProjectEx & project);
			bool initProject(ProjectEx & project);
			bool createProject(ProjectEx & project); 
			bool releaseProject(ProjectEx & project);

			void cleanAllCaches();

		private:

			static void pluginDiagnostic(Project* project, Diagnostic diag, const char* text);

			void printError(const cpl::string_ref message, APE_TextColour colour = APE_TextColour_Error);

            std::map<std::string, std::unique_ptr<CompilerBinding>> compilers;
			ape::Engine& engine;

		};
	};
#endif

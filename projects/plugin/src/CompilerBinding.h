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

	file:CompilerBinding.h
        Responsible for loading and dynamically linking to an external plugin,
        adherring to APE's definition of a 'compiler'.

*************************************************************************************/

#ifndef APE_COMPILERBINDING_H
	#define APE_COMPILERBINDING_H

	#include <cpl/CModule.h>
	#include <cpl/MacroConstants.h>
	#include <cpl/Core.h>
	#include <string>
	#include "CApi.h"
	#include <vector>
	#include "Settings.h"
	#include "ProjectEx.h"
	#include <cassert>
	#include <ape/Events.h>
	#include <ape/CompilerBindings.h>

	namespace ape
	{
		class CCodeGenerator;

		/*
			indexer for g_sExports
		*/
		enum class ExportIndex
		{
			CreateProject,
			CompileProject,
			InitProject,
			ActivateProject,
			DisableProject,
			ProcessReplacing,
			OnEvent,
			ReleaseProject,
			CleanCache,
			end
		};

		constexpr inline std::size_t MaxExports() { return static_cast<std::size_t>(ExportIndex::end); }

		struct CompilerBinding final
		{
			friend class CCodeGenerator;

		public:

			const std::string& name() const noexcept { return compilerName; }
			bool compileProject(ProjectEx * project);
			CompilerBinding(const libconfig::Setting& languageSettings);

		private:

			struct Bindings
			{
				union {
					struct {
						decltype(CreateProject) * createProject;
						decltype(CompileProject) * compileProject;
						decltype(InitProject) * initProject;
						decltype(ActivateProject) * activateProject;
						decltype(DisableProject) * disableProject;
						decltype(ProcessReplacing) * processReplacing;
						decltype(OnEvent) * onEvent;
						decltype(ReleaseProject) * releaseProject;
						decltype(CleanCache) * cleanCache;
					};
					/* sizeof(previous struct) / sizeof (void*) */
					void * _table[MaxExports()];
				};

				void loadBindings(cpl::CModule & module, const libconfig::Setting & exportSettings);
			};

			cpl::CModule module;
			Bindings bindings;
			std::vector<std::string> extensions;
			std::string language;
			std::string compilerName;
			std::string compilerPath;
		};


	}

#endif

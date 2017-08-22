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

	file:Project.h
	
		Defines the common POD-struct ProjectEx, that is used for transferring information
		about current project, compilers, settings and state around.
		Is also the C-interface between the compiler and this program.

*************************************************************************************/

#ifndef APE_PROJECTEX_H
	#define APE_PROJECTEX_H

	#include <ape/Project.h>

	namespace APE
	{
		struct CCompiler;
		struct CSharedInterface;
	
		enum class CodeState
		{
			None,
			Compiled,
			Initialized,
			Disabled,
			Activated,
			Released
		};

		struct ProjectEx : APE_Project
		{
			/// <summary>
			/// A language string that uniquely identifies a corrosponding compiler as set
			/// in the settings(name of language group)
			/// </summary>
			const char * languageID;
			/// <summary>
			/// Associated compiler (see CodeGenerator::compileProject)
			/// Not to be deleted!
			/// </summary>
			struct CCompiler * compiler;
			/// <summary>
			/// See the definition. Defines state of project.
			/// </summary>
			CodeState state;
		};
	
		// global allocators to ensure the same allocator is used.
		ProjectEx * CreateProjectStruct();
		void FreeProjectStruct(ProjectEx * project);
	};
#endif
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
	
		Defines the common POD-struct CProject, that is used for transferring information
		about current project, compilers, settings and state around.
		Is also the C-interface between the compiler and this program.

*************************************************************************************/

#ifndef _PROJECT_H
	#define _PROJECT_H

	#include <cstring>
	#include "MacroConstants.h"

	namespace APE
	{

		typedef char char_t;
		class CCompiler;
		struct CSharedInterface;
		enum CodeState : int
		{
			None,
			Compiled,
			Initialized,
			Disabled,
			Activated,
			Released

		};
		
		struct __alignas(APE_DEF_ALIGN) CProject
		{
			/*
				A language string that uniquely identifies a corrosponding compiler as set
				in the settings (name of language group)
			*/
			char_t * languageID;
			/* 
				Associated compiler (see CodeGenerator::compileProject)
				Not to be deleted!
			*/
			CCompiler * compiler;
			/*
				Interface to the API
				Not to be deleted!
			*/
			CSharedInterface * iface;
			/*
				Version of the engine of this program
			*/
			unsigned engineVersion;
			/*
				Special case: Program consists of a single source strings, instead of files.
				If this is set to non-zero, sourceString will point to a valid string that 
				contains the source. If not, sourceString is not valid.
			*/
			unsigned isSingleString;
			/*
				amount of files contained in the array files
			*/
			unsigned int nFiles;
			/*
				an instance-unique ID for this current project
			*/
			unsigned int uniqueID;
			/*
				See the definition. Defines state of project.
			*/
			CodeState state;
			/*
				If isSingleString is set, this points to valid source.
			*/
			char_t * sourceString;
			/*
				Array of files in this project (source files)
			*/
			char_t ** files;
			/*
				Name of this project
			*/
			char_t * projectName;
			/*
				Directory of the project. Directory + \ + projectName must yield the valid path.
			*/
			char_t * workingDirectory;
			/*
				Root directory of APE
			*/
			char_t * rootPath;
			/*
				Compiler specific arguments and switches (like commandline arguments).
				Ïntended that it be set in the config.
			*/
			char_t * arguments;
			/*
				The compiler or runtime can keep it's instance data here.
			*/
			void * userData;
		};
		
		// global allocators to ensure the same allocator is used.
		CProject * CreateProjectStruct();
		void FreeProjectStruct(CProject * project);

	};
#endif
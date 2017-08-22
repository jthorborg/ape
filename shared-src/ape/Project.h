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

#ifndef APE_PROJECT_H
	#define APE_PROJECT_H

	#include "SharedInterface.h"

	struct APE_SharedInterface;
		
	struct APE_Project
	{
		/*
			Interface to the API
			Not to be deleted!
		*/
		struct APE_SharedInterface * iface;
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
			If isSingleString is set, this points to valid source.
		*/
		const char * sourceString;
		/*
			Array of files in this project (source files)
		*/
		char * const * files;
		/*
			Name of this project
		*/
		const char * projectName;
		/*
			Directory of the project. Directory + \ + projectName must yield the valid path.
		*/
		const char * workingDirectory;
		/*
			Root directory of APE
		*/
		const char * rootPath;
		/*
			Compiler specific arguments and switches (like commandline arguments).
			Intended that it be set in the config.
		*/
		const char * arguments;
		/*
			The compiler or runtime can keep it's instance data here.
		*/
		void * userData;
	};
	
	#ifdef __cplusplus

		namespace APE
		{
			//using CProject = APE_Project;
		};
	#endif
#endif
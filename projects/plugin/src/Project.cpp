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

	file:Project.cpp
		
		Implementation of Project.h, more specifically: constructors and destructors
		for ProjectEx

*************************************************************************************/

#include "ProjectEx.h"
#include <cstring>

namespace ape 
{
	ProjectEx * CreateProjectStruct()
	{ 
		ProjectEx * p = new ProjectEx; 
		std::memset(p, 0, sizeof (ProjectEx));
		return p; 
	}
	void FreeProjectStruct(ProjectEx * project)
	{ 
		/*
			Nullptr check is apparantly not needed, but why not.
		*/
		if(project->sourceString)
			delete[] project->sourceString;
		if(project->rootPath)
			delete[] project->rootPath;
		if(project->projectName)
			delete[] project->projectName;
		if (project->arguments)
			delete[] project->arguments;
		if (project->nFiles) {
			for (unsigned i = 0; i < project->nFiles; ++i) {
				if (project->files && project->files[i])
					delete[] project->files[i];
			}
		}
		if (project->files)
			delete[] project->files;
		if(project)
			delete project;
	}
};
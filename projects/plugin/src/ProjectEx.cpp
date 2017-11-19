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
	ProjectEx::ProjectEx()
	{ 
		std::memset(this, 0, sizeof(*this));
	}
	
	ProjectEx::~ProjectEx()
	{ 
		/*
			Nullptr check is apparantly not needed, but why not.
		*/
		if(sourceString)
			delete[] sourceString;
		if(rootPath)
			delete[] rootPath;
		if(projectName)
			delete[] projectName;
		if (arguments)
			delete[] arguments;
		if (nFiles) 
		{
			for (unsigned i = 0; i < nFiles; ++i) {
				if (files && files[i])
					delete[] files[i];
			}
		}

		if (files)
			delete[] files;
	}
};
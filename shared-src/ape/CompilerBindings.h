/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.4.0.
	 
	 Copyright (C) 2017 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:CompilerBindings.h
	
		Exported C bindings that ape should be able to find.

*************************************************************************************/

#include "APE.h"
#include "Project.h"

#ifdef __cplusplus
extern  "C"
{
#endif
	EXPORTED APE_Status APE_API CreateProject(APE_Project * p);
	EXPORTED APE_Status APE_API CompileProject(APE_Project * p);
	EXPORTED APE_Status APE_API InitProject(APE_Project * p);
	EXPORTED APE_Status APE_API ActivateProject(APE_Project * p);
	EXPORTED APE_Status APE_API DisableProject(APE_Project * p, int misbehaved);
	EXPORTED APE_Status	APE_API ProcessReplacing(APE_Project * p, const float * const * in, float * const * out, size_t sampleFrames);
	EXPORTED APE_Status	APE_API OnEvent(APE_Project * p, APE_Event * e);
	EXPORTED APE_Status APE_API ReleaseProject(APE_Project * p);
	// Guaranteed to never be called concurrently with any other command
	EXPORTED APE_Status APE_API CleanCache();

#ifdef __cplusplus
}
#endif

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

#ifndef APE_CSHAREDINTERFACE_H
	#define APE_CSHAREDINTERFACE_H

	#include "APE.h"

#ifdef __cplusplus
	namespace APE
	{
#endif
		struct APE_SIExtra
		{
			void * plugin_data;
			const void * const engine;
			const void * const csys;
		};

		struct APE_SharedInterface
		{
			float		(APE_API * getSampleRate)	(struct APE_SharedInterface * iface);
			int			(APE_API_VARI * printLine)	(struct APE_SharedInterface * iface, unsigned nColor, const char * fmt, ...);
			int			(APE_API * msgBox)			(struct APE_SharedInterface * iface, const char * text, const char * title, int nStyle, int nBlocking);
			APE_Status	(APE_API * setStatus)		(struct APE_SharedInterface * iface, APE_Status status);
			int			(APE_API * createKnob)		(struct APE_SharedInterface * iface, const char * name, float * extVal, int type);
			long long	(APE_API * timerGet)		(struct APE_SharedInterface * iface);
			double		(APE_API * timerDiff)		(struct APE_SharedInterface * iface, long long time);
			void *		(APE_API * alloc)			(struct APE_SharedInterface * iface, size_t size);
			void		(APE_API * free)			(struct APE_SharedInterface * iface, void * ptr);
			int			(APE_API * createKnobEx)	(struct APE_SharedInterface * iface, const char * name, float * extVal, char * values, char * unit);
			void		(APE_API * setInitialDelay)	(struct APE_SharedInterface * iface, int samples);
			int			(APE_API_VARI * createLabel)(struct APE_SharedInterface * iface, const char * name, const char * fmt, ...);
			int			(APE_API * getNumInputs)	(struct APE_SharedInterface * iface);
			int			(APE_API * getNumOutputs)	(struct APE_SharedInterface * iface);
			int			(APE_API * createMeter)		(struct APE_SharedInterface * iface, const char * name, float * extVal);
			int			(APE_API * createToggle)	(struct APE_SharedInterface * iface, const char * name, float * extVal);
			double		(APE_API * getBPM)			(struct APE_SharedInterface * iface);
			float		(APE_API * getCtrlValue)	(struct APE_SharedInterface * iface, int iD);
			void		(APE_API * setCtrlValue)	(struct APE_SharedInterface * iface, int iD, float value);
			int			(APE_API * createPlot)		(struct APE_SharedInterface * iface, const char * name, const float * const values, const unsigned int numVals);
			int			(APE_API * createRangeKnob)	(struct APE_SharedInterface * iface, const char * name, const char * unit, float * extVal, APE_ScaleFunc scaleCB, float min, float max);

			struct APE_SIExtra extra;
		};
	
#ifdef __cplusplus

		using SharedInterface = APE_SharedInterface;

	};
#endif
#endif
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

	file:Event.h
	
		Defines events messaged by APE.

*************************************************************************************/

#ifndef APE_EVENT_H
	#define APE_EVENT_H

	#include "SharedInterface.h"

	struct APE_Event_CtrlValueChanged 
	{
		float value;
		char text[64];
		char title[64];
		int tag;
		bool unused;
	};

	typedef enum 
	{
		CtrlValueChanged = 0

	} APE_EventType;

	struct APE_Event
	{
		APE_EventType eventType;
		union
		{
			APE_Event_CtrlValueChanged * eCtrlValueChanged;

		} event;

	};

	#ifdef __cplusplus
		namespace APE
		{
			using CEvent = APE_Event;
			using CEventType = APE_EventType;
		};
	#endif
#endif
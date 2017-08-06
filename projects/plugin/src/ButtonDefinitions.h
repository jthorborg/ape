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

	file:ButtonDefinitions.h
		
		Lists buttons used in the program, and additional details assoctiated with them.
		Only used if compiling for VSTGUI.

*************************************************************************************/

#ifndef _BUTTONDEFINITIONS_H
	#define _BUTTONDEFINITIONS_H

	#define BUTTON_HEIGHT 138
	#define BUTTON_LENGTH 53

	namespace APE {
		#ifdef APE_VST
			struct sButton { int nResourceID, left, top, right, bottom; bool sticky; };

			sButton ButtonDefs[] = {
				{	GUI_RESOURCE::kConsoleButton,		0, 0, BUTTON_HEIGHT		, BUTTON_LENGTH, true },
				{	GUI_RESOURCE::kCompileButton,		0, 0, BUTTON_HEIGHT		, BUTTON_LENGTH, false},
				{	GUI_RESOURCE::kActiveStateButton,	0, 0, BUTTON_HEIGHT		, BUTTON_LENGTH, true },
				{	GUI_RESOURCE::kEditorButton,		0, 0, BUTTON_HEIGHT		, BUTTON_LENGTH, true },
				{	GUI_RESOURCE::kAboutButton,			0, 0, BUTTON_HEIGHT		, BUTTON_LENGTH, false}
			};
		#elif defined(APE_JUCE)

		#endif
	};

	#undef BUTTON_HEIGHT
	#undef BUTTON_LENGTH
#endif
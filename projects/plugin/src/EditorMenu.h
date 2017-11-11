/*************************************************************************************

	Audio Programming Environment VST. 
		
		VST is a trademark of Steinberg Media Technologies GmbH.

    Copyright (C) 2013 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:EditorMenu.h
	
		Poor-Man's resource file for menu's, when you're working with a DLL.

*************************************************************************************/

#ifndef _EDITOR_MENU_H

	#define _EDITOR_MENU_H

	#include "MacroConstants.h"

	#ifdef __WINDOWS__

		#include <Windows.h>

		namespace ape {
				struct MenuItem { UINT type; char *title;};


				MenuItem FileItems[] = {
					{IDM_FILE_NEW, "&New\tCtrl+N"},
					{IDM_FILE_OPEN, "&Open...\tCtrl+O"},
					{IDM_FILE_SAVE, "&Save\tCtrl+S"},
					{IDM_FILE_SAVEAS, "Save &As..."},
					{IDM_FILE_EXIT, "E&xit"}
				};
				MenuItem EditItems[] = {
					{IDM_EDIT_UNDO, "&Undo\tCtrl+Z"},
					{IDM_EDIT_REDO, "&Redo\tCtrl+Y"},
					//AppendMenu (hMenuPopup, MF_SEPARATOR, NULL, NULL},
					{IDM_EDIT_CUT, "Cu&t\tCtrl+X"},
					{IDM_EDIT_COPY, "&Copy\tCtrl+C"},
					{IDM_EDIT_PASTE, "&Paste\tCtrl+V"},
					{IDM_EDIT_DELETE, "&Delete\tDel"},
					{IDM_EDIT_SELECTALL, "Select A&ll\tCtrl+A"}		
				};

				ACCEL Accs[] = {
					{ FVIRTKEY | FCONTROL, IDM_EDIT_SELECTALL,	'A'},
					{ FVIRTKEY | FCONTROL, IDM_EDIT_COPY,       'C'},
					{ FVIRTKEY | FCONTROL, IDM_FILE_NEW,        'N'},
					{ FVIRTKEY | FCONTROL, IDM_FILE_OPEN,       'O'},
					{ FVIRTKEY | FCONTROL, IDM_FILE_SAVE,       'S'},
					{ FVIRTKEY | FCONTROL, IDM_EDIT_PASTE,      'V'},
					{ FVIRTKEY,			   IDM_EDIT_DELETE,     VK_DELETE},
					{ FVIRTKEY | FCONTROL, IDM_EDIT_CUT,        'X'},
					{ FVIRTKEY | FCONTROL, IDM_EDIT_REDO,       'Y'},
					{ FVIRTKEY | FCONTROL, IDM_EDIT_UNDO,       'Z'}
				};
			}
	#else
		#error Incompatible definitions with windows.
	#endif
#endif
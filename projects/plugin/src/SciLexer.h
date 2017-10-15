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

	file:SciLexer.h
	
		Wrapper around the SciLexer DLL.

*************************************************************************************/
#ifndef _SCILEXER_H
	#define _SCILEXER_H

	#include "MacroConstants.h"

	#ifdef __WINDOWS__

		#include <windows.h>
		#include <richedit.h>
		#include "resource.h"
		#include "SDKs\scintilla\Scintilla.h"
		#include "SDKs\scintilla\SciLexer.h"
		#include <map>
		#include "CCodeEditor.h"
		#include "CModule.h"

		const int blockSize = 128 * 1024;

		const COLORREF black = RGB(0,0,0);
		const COLORREF white = RGB(0xff,0xff,0xff);

		namespace Scintilla {

			struct SciLexer;

			typedef void * WHandle;
			typedef void * MHandle;
			#ifdef __WINDOWS__
				typedef long __w64 OSRes;
				typedef OSRes LPtr;
				typedef UINT_PTR UPtr;
				#define OSCallBack CALLBACK
			#else
				#error NI
			#endif
			extern std::map<WHandle, SciLexer * > LookUpEditors;

			struct SciLexer : public APE::CCodeEditor{

			private:
				// handles
				MHandle hInstance;
				APE::CModule hScite;
				WHandle currentDialog;
				WHandle wMain;
				WHandle wEditor;

				// states
				bool isDirty;
				bool isInitialized;
				bool isSingleFile;
				std::string fullPath;
				bool isActualFile;
				struct {
					bool bSet;
					int nLine;
					int nError;
				} compiler_error;
				bool isVisible;
				SciFnDirect sciFunc;
				sptr_t sciData;
				OSRes sendEditor(unsigned int msg, UPtr param1 = 0, LPtr param2 = 0);
				/*
					These are mostly from a demo program from Scintilla
				*/
				void GetRange(int start, int end, char *text);
				void SetTitle();
				void New();
				void Open();
				void Save();
				void SaveAs();
				void SaveFile(const std::string & fileName);
				int SaveIfUnsure();
				void Command(int id);
				void EnableAMenuItem(int id, bool enable);
				void CheckMenus();
				void Notify(SCNotification *notification);
				void SetAStyle(int style, COLORREF fore, COLORREF back=white, int size=-1, const char *face=0);
				void setEditorStyling();


				//------------------------
			public:
				static OSRes OSCallBack WndProc(WHandle hWnd, unsigned int iMessage, UPtr wParam, LPtr lParam);
				static int wndThread(SciLexer * _this);
				SciLexer(APE::Engine * e);
				~SciLexer();
				void setErrorLine(int nLine);
				bool getDocumentText(std::string & buffer);
				void quit();
				void show();
				void hide();
				bool initEditor();
				bool openEditor();
				bool closeEditor();
				bool openFile(const std::string & fileName);
				bool isOpen() { return isVisible; }
				bool exists() { return true; }
				std::string getDocumentName();
				std::string getDocumentPath() {
					return fullPath;
				}
				std::string getProjectName();
				std::string getDirectory();
				std::string getExtension();
				APE::CProject * getProject();
				void registerWindowClass();
				HMENU createMenu();
				HACCEL createAccels();

				// overloads



			};

		};
	#else
		#error Rewrite this whole class for other systems than windows
	#endif
#endif
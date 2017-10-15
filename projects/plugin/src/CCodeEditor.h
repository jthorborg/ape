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

	file:CCodeEditor.h
	
		Interface for the Code Editor, any code editor should inherit from this
		and at least provide the methods that are purely virtual (obviously).
		Since the class doesn't carry any parent, it cannot report error per se -
		therefore the class provides exists() method so the validity of the instance
		can be proven (return true if proper implementation exists).
 
		CDefaultCodeEditor can be used as a 'valid' implementation, that returns false
		on exists()

*************************************************************************************/

#ifndef APE_CCODEEDITOR_H
	#define APE_CCODEEDITOR_H

	#include <string>

	namespace APE
	{
#pragma message("fix")
		struct ProjectEx;
		// ideally the editor should not know about the engine, but library design doesn't
		// really allow this.
		class Engine;

		class CCodeEditor
		{
		protected:
			Engine * engine;
		public:
			CCodeEditor(Engine * e) : engine(e) {};
			virtual ~CCodeEditor() {};
			virtual void setErrorLine(int nLine) = 0;
			virtual bool getDocumentText(std::string & buffer) = 0;
			virtual ProjectEx * getProject() { return nullptr; }
			virtual void quit() = 0;
			virtual bool initEditor() { return false; }
			virtual bool openEditor(bool initialVisibility = true) { return false; }
			virtual bool closeEditor() { return false; }
			virtual bool isOpen() { return false; }
			virtual std::string getDocumentName() { return ""; }
			virtual std::string getDocumentPath() { return ""; }
			virtual bool openFile(const std::string & fileName) { return false; }
			virtual void autoSave() {};
			// true: a project was restored, false: nothing happened
			virtual bool checkAutoSave() { return false; }
			virtual bool exists() = 0;
		};

		class CDefaultCodeEditor : public CCodeEditor
		{
		public:
			CDefaultCodeEditor(Engine * e) : CCodeEditor(e) {};
			void setErrorLine(int) {};
			bool getDocumentText(std::string &) { return false; }
			void quit() {};
			void show() {};
			void hide() {};
			bool exists() { return false; }


		};
	};
#endif
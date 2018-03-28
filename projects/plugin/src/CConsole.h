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

	file:CConsole.h
		
		Interface for the console control class used in this program.

*************************************************************************************/

#ifndef APE_CONSOLE_H
	#define APE_CONSOLE_H

	#include "Common.h"
	#include <vector>
	#include <string>
	#include <list>
	#include <stdarg.h>
	#include "GraphicComponents.h"
	#include <cpl/CExclusiveFile.h>
	#include <cpl/Core.h>
	#include <filesystem>

	namespace ape
	{
		class CConsole;
		class CTextLabel;
		const int nPixelPerLine = 15; // height of an text-line
		const float nPixelPerChar = 8.2f; // length of an single character in pixels (average, safe bet)

		/*
			struct ConsoleMessage - everything thats printed to the console is stored in a object like this
			in a linked list so we can keep the original messages and color through open() and close()'s.
		*/
		struct ConsoleMessage
		{
			std::string msg;
			juce::Colour color;
			ConsoleMessage(std::string msg, juce::Colour color) : msg(msg), color(color) {};
		};

		class CConsoleContainer : public CScrollableContainer
		{
		private:
			bool dirty;
			CConsole * parent;
		public:
			CConsoleContainer(CConsole * parent);
			void paint(juce::Graphics & g);
			void setDirty(bool toggle = true) { dirty = toggle; }
		};


		class CConsole : public cpl::CMutex::Lockable, juce::AsyncUpdater
		{
		private:
			int nLines; // number of lines
			int nLineSize; // calculated at runtime, length of a line 
			CConsole(const CConsole &);
			CConsoleContainer * cont;
		public:
			CConsole();
			~CConsole();
			void create(const CRect & inSize);
			void close();
			int printLine(CColour color, const char * fmt, ... );
			int printLine(CColour color, const char * fmt, va_list ap);
			int calculateTextLength(std::string & text);
			void handleAsyncUpdate() override;
			void renderConsole();
			void removeHistory(std::list<ConsoleMessage>::reverse_iterator & it);
			void setLogging(bool toggle, const cpl::fs::path& file);
			bool loggingEnabled() { return logging; }
			void setStdWriting(bool toggle);
			void setVisibilty(bool toggle) 
			{ 
				visible = toggle; 
			}
			CConsoleContainer * getView() { return cont; }
		protected:
			std::list<ConsoleMessage> msgs;
			std::vector<CTextLabel*> lines;
			bool logging, stdWriting, visible, dirty;
			cpl::CExclusiveFile debugFile;

		
		}; // class CConsole
	}; // class ape
#endif
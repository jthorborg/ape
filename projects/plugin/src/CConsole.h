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
	#include <cpl/CExclusiveFile.h>
	#include <cpl/Core.h>
	#include <cpl/filesystem.h>
	#include <mutex>
	#include "GraphicComponents.h"
	#include <cpl/Utility.h>
	#include <ape/SharedInterface.h>

	namespace ape
	{
		class CConsole;
		class CTextLabel;
		class CConsoleContainer;

		class CConsole 
			: private juce::AsyncUpdater
			, public juce::ChangeBroadcaster
			, private cpl::Utility::CNoncopyable
		{
			friend class CConsoleContainer;

		public:

			static constexpr APE_TextColour Default = APE_TextColour_Default;
			static constexpr APE_TextColour Error = APE_TextColour_Error;
			static constexpr APE_TextColour Warning = APE_TextColour_Warning;

			CConsole();
			~CConsole();

			std::unique_ptr<juce::Component> create();
			int printLine(const char * fmt, ...);
			int printLine(APE_TextColour color, const char * fmt, ...);
			int printLine(juce::Colour color, const char * fmt, ...);

			int printLine(APE_TextColour color, const char * fmt, va_list args);
			int printLine(CColour color, const char * fmt, va_list ap);

			void handleAsyncUpdate() override;
			void setLogging(bool toggle, const cpl::fs::path& file);
			bool loggingEnabled() { return logging; }
			void setStdWriting(bool toggle);
			void setVisibilty(bool toggle) 
			{ 
				visible = toggle; 
			}

			juce::Colour colourFromStandard(APE_TextColour colour) const;

		protected:

			int calculateTextLength(std::string & text);

			struct ConsoleMessage
			{
				std::string msg;
				juce::Colour color;
				ConsoleMessage(std::string msg, juce::Colour color) : msg(msg), color(color) {};
			};

			std::list<ConsoleMessage> msgs;
			bool logging, stdWriting, visible, dirty;
			cpl::CExclusiveFile debugFile;
			std::mutex mutex;

		private:

			std::mutex& getMutex() { return mutex; }

		}; // class CConsole
	}; // class ape
#endif

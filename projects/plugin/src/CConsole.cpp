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

	file:CConsole.cpp
	
		Implementation of the console class.
		A console instance is unique to a plugin instance, and must persist to exist
		throughout the instance's lifetime. It can be opened and close.

*************************************************************************************/

#include "CConsole.h"
#include <cpl/Common.h>
#include <cpl/Misc.h>
#include <cstdarg>
#include <cstdio>
#include <cctype>
#include <sstream>

namespace ape
{
	constexpr int nPixelPerLine = 15; // height of an text-line
	constexpr float nPixelPerChar = 8.2f; // length of an single character in pixels (average, safe bet)

	class CConsoleContainer 
		: public CScrollableContainer
		, private juce::ChangeListener
	{

	public:
		CConsoleContainer(CConsole& p) 
			: parent(p) 
		{ 
			setName("Console");
			parent.addChangeListener(this);
		}	

		void resized() override
		{
			CScrollableContainer::resized();
			lines.clear();
			const auto bounds = getBounds();

			CRect fakeSize(bounds);
			fakeSize.setHeight(bounds.getHeight() * 4);
			setVirtualHeight(fakeSize.getHeight());

			nLines = int(fakeSize.getHeight() / nPixelPerLine);
			// this is a rough estimate - rather have some api calculate the length of
			// the line on the fly, but can't find a cross platform alternative.
			nLineSize = int(fakeSize.getWidth() / nPixelPerChar);
			lines.reserve(nLines);
			for (int i = 0; i < nLines; i++)
			{
				lines.emplace_back(std::make_unique<CTextLabel>());
				lines[i]->setBounds(5, i * nPixelPerLine, fakeSize.getWidth() - getSCB().getWidth(), nPixelPerLine);
				lines[i]->setFontSize(13);
				lines[i]->setColour(juce::Colours::black);
				lines[i]->setFontName(juce::Font::getDefaultMonospacedFontName());
				//addAndMakeVisible(*lines[i]);
				getVContainer().addAndMakeVisible(*lines[i]);
			}

			bSetValue(1); // ?
			renderConsole();
		}

		~CConsoleContainer()
		{
			parent.removeChangeListener(this);
		}

	protected:

		void visibilityChanged() override
		{
			renderConsole();
		}

		virtual void changeListenerCallback(juce::ChangeBroadcaster * source) override
		{
			if (!isVisible())
				return;

			renderConsole();
			repaint();
		}

	private:

		void renderConsole();

		CConsole& parent;
		std::vector<std::unique_ptr<CTextLabel>> lines;
		int nLines{}; // number of lines
		int nLineSize{};
	};


	void CConsoleContainer::renderConsole()
	{
		std::lock_guard<std::mutex> lockGuard(parent.getMutex());

		// no point in wasting cycles if there's no lines to print on.
		if (!(nLines > 0))
			return;

		auto& msgs = parent.msgs;

		//nothing to print
		if (msgs.empty())
			return;

		int lines_used = 0;
		std::size_t msg_len; // total length of the current msg (eqv. to (*current).msg.lenght())
							 // traverse through the messages in backwards (newest is pushed backwards)
		for (auto current = msgs.rbegin(); current != msgs.rend(); ++current) {

			// if there's no more space on the console screen, stop
			// also it's possible to have more "used lines" than available, since the loop below "prints" out of the screen 
			// but ignores them, eventhough they are counted ("int temp = lines_needed").
			if (lines_used >= nLines) {
#ifdef _CONSOLE_CLEAR_HISTORY
				removeHistory(current);
#endif
				break;
			}

			auto& curMsg = *current;
			msg_len = curMsg.msg.length();

			// message is longer than one line
			if ((msg_len - 1) > this->nLineSize) {

				int lines_needed = (int)ceil(float(msg_len) / nLineSize);
				int temp = lines_needed; // store this as we subtract this from lines_used in the end
				int pos = 0;
				char old_c(0);
				// print the message
				for (int cur_line_index = nLines - lines_used - lines_needed; // this is the first line
					lines_needed; // we continue the loop as long as we need more lines to print the msg
								  // on each loop we subtract a line needed and advance a line's length in our pos, and increment current line
					lines_needed--, pos += nLineSize, ++cur_line_index)
				{
					// if the current line index is out of the screen, we ignore it
					if (cur_line_index < 0)
						continue;

					if (lines_needed > 1) 
					{

						// scan a bit backwards to see if we can find a logical place to seperate (ie. whitespace)

						int backwardsScanLength = nLineSize;

						for (int z = pos + nLineSize, // char iterator
							y = 0; // amount of chars to shave off (ie. if we find whitespace, it will be at pos + nLinesize + y
							y < backwardsScanLength - 1;
							--z, ++y)
						{
							if (!std::isalnum(curMsg.msg[z]))
							{
								pos -= y;
								break;
							}
						}

						// save the old character at the next linebreak so we can replace it with 
						// a null when we do a buffer copy
						old_c = curMsg.msg[pos + nLineSize];
						curMsg.msg[pos + nLineSize] = '\0';
					}
					// copying the buffer range into a line with corrosponding color
					// we avoid adding negative values of pos (caused by previous backscan) since it will return invalid memory
					// however, this will still work correct.
					lines[cur_line_index]->setText(curMsg.msg.c_str() + (pos > 0 ? pos : 0));
					lines[cur_line_index]->setColour(curMsg.color);

					// replace the null if we put one there with the old character
					if (lines_needed > 1)
						curMsg.msg[pos + nLineSize] = old_c;
				}
				lines_used += temp;
			}
			else 
			{
				/*
					Message is shorter than one line: We just grab the line from an index and print it.
				*/
				CTextLabel& label = *lines[nLines - lines_used - 1];

				label.setText(curMsg.msg.c_str());
				label.setColour(curMsg.color);

				lines_used++;
			}
		}
	}


	CConsole::CConsole() 
		: logging(false)
		, stdWriting(false)
	{

	}

	std::unique_ptr<juce::Component> CConsole::create()
	{
		std::lock_guard<std::mutex> lockGuard(mutex);
		return std::make_unique<CConsoleContainer>(*this);
	}

	void CConsole::setLogging(bool toggle, const cpl::fs::path& file)
	{
		logging = toggle;
		if(logging)
		{
			debugFile.open(file.string());
		}
	}

	void CConsole::setStdWriting(bool toggle)
	{
		stdWriting = toggle;
	}

	CConsole::~CConsole() 
	{
	}

	int CConsole::printLine(CColour color, const char * fmt, ... ) 
	{
		std::va_list args;
		va_start(args, fmt);
		const auto len = this->printLine(color, fmt, args);
		va_end(args);

		return len;
	}

	void CConsole::handleAsyncUpdate()
	{
		sendChangeMessage();
	}

	int CConsole::printLine(CColour color, const char * fmt, va_list args) 
	{
		std::lock_guard<std::mutex> lockGuard(mutex);
		int nBufLen(0);
		char * fmtd_str;
		nBufLen = cpl::Misc::GetSizeRequiredFormat(fmt, args);

		if(nBufLen < 0)
			return 0;
		std::string prefix = "[";
		prefix += cpl::Misc::GetTime() + "]";
		prefix += " > ";
		auto size = nBufLen + 5 + prefix.length();
		fmtd_str = new char[size];

		std::memcpy(fmtd_str, prefix.c_str(), prefix.length());
		// print to the buffer, and store the amount of characters printed.
		// we add one to buflen to make space for nulltermination (extra allocation has been done
		// previously, see "auto size = ..."), and vsnprintf returns the amount of characters,
		// that would have been printed, NOT including the nulltermination.
		#ifdef CPL_MSVC
			auto charsPrinted = vsnprintf_s(fmtd_str + prefix.length(), size - prefix.length(), nBufLen + 1, fmt, args);
		#else
			auto charsPrinted = vsnprintf(fmtd_str + prefix.length(), nBufLen + 1, fmt, args);
		#endif
		if (charsPrinted != nBufLen )
		{
			// we printed a different amount of characters than we expected,
			// report the error.
			std::stringstream fmt;
			fmt << "Console error: Mismatch in printed chars (expected "
				<< nBufLen << ", printed " << charsPrinted << "), check your parameters:";
			msgs.push_back(ConsoleMessage(fmt.str(), CColours::red));
		}
		// log to file
#ifdef _DEBUG
		if (cpl::Misc::IsBeingDebugged())
		{
			CPL_DEBUGOUT(fmtd_str);
			CPL_DEBUGOUT("\n");
		}
#endif

		if(logging && debugFile.isOpened())
		{
			debugFile.write(fmtd_str);
			debugFile.write("\n");
		}

		// write to standard streams
		if (stdWriting)
			// avoid using printf for potential exploits
			std::fputs(fmtd_str, stdout);

		// Here we make line breaks into seperate messages.
		auto len = nBufLen + prefix.length();
		char * start = fmtd_str;
		unsigned i, offset = 0;
		for (i = 0; i < len; ++i)
		{

			if (fmtd_str[i] == '\n') 
			{
				msgs.emplace_back(std::string(start, offset), color);
				start = fmtd_str + i + 1;
				offset = 0;
			}
			else
				offset++;
		}
		// last part OR the whole message if no linebreaks were found (start == fmtd_str, i = strlen(fmtd_str))
		// OR nothing
		if (offset)
			msgs.emplace_back(std::string(start, offset), color);

		delete[] fmtd_str;

		triggerAsyncUpdate();
		return nBufLen;
	}
}
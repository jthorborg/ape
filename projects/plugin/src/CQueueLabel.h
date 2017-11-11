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

	file:CQueueLabel.h
		
		A label deriving from CTextLabel, that allows showing a default message as well
		as managing a queue of messages that will be showed for a interval, afterwards 
		showing the default message again.
 
		requires calling updateMessage repeatadly

*************************************************************************************/

#ifndef _QUEUELABEL_H
	#define _QUEUELABEL_H

	#include "MacroConstants.h"
	#include <cpl/Misc.h>
	#include <cpl/CMutex.h>
	#include "GraphicComponents.h"
	#include <queue>

	namespace ape
	{
		class CQueueLabel : public cpl::CMutex::Lockable, public CTextLabel
		{
			typedef std::pair<juce::String, CColour> colouredString;
			
			struct Message
			{
				Message(const colouredString & m, const int & to)
				: msg(m), timeOut(to), timeStamp(0)
				{

				}
				colouredString msg;
				unsigned int timeOut;
				unsigned int timeStamp;

			};

			std::queue < Message > messageStack;
			colouredString defaultMessage;
			colouredString * currentMessage;

		private:

			void paint(juce::Graphics & g)
			{
				cpl::CMutex lock(this);
				g.setFont(size);
				g.setColour(currentMessage->second);
				g.drawText(currentMessage->first, CRect(0, 0, getWidth(), getHeight()), just, false);

			}

		public:

			CQueueLabel()
			{
				currentMessage = &defaultMessage;
			}

			void setDefaultMessage(const juce::String & msg, CColour colour)
			{
				cpl::CMutex lock(this);
				defaultMessage.first = msg;
				defaultMessage.second = colour;
			}

			void pushMessage(const juce::String & msg, CColour colour, int timeout)
			{
				cpl::CMutex lock(this);
				messageStack.push(Message(std::make_pair(msg, colour), timeout));
			}

			void updateMessage()
			{
				cpl::CMutex lock(this);
				if (messageStack.size())
				{

					auto & front = messageStack.front();
					if (!front.timeStamp)
					{
						// new message: no timestamp yet
						// set it as the message, and stamp it
						currentMessage = &front.msg;
						front.timeStamp = cpl::Misc::QuickTime();
						repaint();
						return;
					}
					// message already has a timestop (ie. it is shown) - check if it's been
					// shown for it's period
					else if (cpl::Misc::QuickTime() > (front.timeStamp + front.timeOut))
					{
						messageStack.pop();
						repaint();
						if (messageStack.size())
						{
							// we have now invalidated currentMessage, so we need to get a new
							// front - this looks like a case for recursion, however
							// the mutex lock makes the code non-reentrant.
							auto & front = messageStack.front();
							if (!front.timeStamp)
							{
								// new message: no timestamp yet
								// set it as the message, and stamp it
								currentMessage = &front.msg;
								front.timeStamp = cpl::Misc::QuickTime();
							}
							return;
						} // no elements left in container: fallback to default message at bottom
					}
					else
					{
						// no new message: and old one hasn't expired yet, so early return here - 
						// instead of default case currentMessage = defaultMessage
						return;
					}
				}
				// default case - set current shown message to default
				currentMessage = &defaultMessage;
			}

		};
	};
#endif
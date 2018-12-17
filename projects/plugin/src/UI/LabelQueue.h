/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2018 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:LabelQueue.h
		
		A label deriving from CTextLabel, that allows showing a default message as well
		as managing a queue of messages that will be showed for a interval, afterwards 
		showing the default message again.
 
		requires calling pulseQueue repeatedly

*************************************************************************************/

#ifndef APE_LABELQUEUE_H
	#define APE_LABELQUEUE_H

	#include <queue>
	#include <mutex>

	#include <cpl/Common.h>
	#include <cpl/Misc.h>
	#include <cpl/CMutex.h>
	#include <cpl/Exceptions.h>

	#include "../GraphicComponents.h"

	namespace ape
	{
		class LabelQueue;

		class LabelQueue
		{
		public:

			LabelQueue();

			void setDefaultMessage(juce::String msg, juce::Colour colour);
			void pushMessage(juce::String msg, juce::Colour colour, int timeout);
			void setDefaultPrefix(juce::String prefix);
			void pulseQueue();

		private:

			class Listener
			{
			public:
				virtual void onNewMessage(const LabelQueue& queue) = 0;
				virtual ~Listener() {}
			};

			friend class LabelQueueDisplay;

			typedef std::pair<juce::String, juce::Colour> ColouredString;

			struct Message
			{
				Message(ColouredString m, const int to)
					: msg(std::move(m))
					, timeOut(to)
					, timeStamp(0)
				{

				}

				ColouredString msg;
				unsigned int timeOut;
				unsigned int timeStamp;

			};

			void updateQueue();
			void notifyListeners();
			void addListener(Listener& l);
			void removeListener(Listener& l);

			std::mutex mutex;
			ColouredString defaultMessage;
			ColouredString* currentMessage;
			std::queue<Message> queue;
			Listener* listener;
			juce::String prefix;
		};

		class LabelQueueDisplay 
			: public CTextLabel
			, private LabelQueue::Listener
		{
		public:

			LabelQueueDisplay(LabelQueue& queue);
			~LabelQueueDisplay();

		protected:

			void paint(juce::Graphics & g) override;
			void onNewMessage(const LabelQueue & queue) override;

		private:

			LabelQueue& queue;
		};
	};
#endif
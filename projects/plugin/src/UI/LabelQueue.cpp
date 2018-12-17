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

	file:LabelQueue.cpp
		
		Implementation of LabelQueue.h

*************************************************************************************/

#include "LabelQueue.h"

namespace ape
{
	LabelQueueDisplay::LabelQueueDisplay(LabelQueue& queueToDisplay)
		: queue(queueToDisplay)
	{
		queue.addListener(*this);
	}

	LabelQueueDisplay::~LabelQueueDisplay()
	{
		queue.removeListener(*this);
	}

	void LabelQueueDisplay::paint(juce::Graphics & g)
	{
		std::lock_guard<std::mutex> lock(queue.mutex);
		g.setFont(size);
		g.setColour(queue.currentMessage->second);
		g.drawText(queue.prefix + queue.currentMessage->first, CRect(0, 0, getWidth(), getHeight()), just, false);
	}

	LabelQueue::LabelQueue()
		: currentMessage(&defaultMessage)
		, listener(nullptr)
	{

	}

	void LabelQueueDisplay::onNewMessage(const LabelQueue & queue)
	{
		repaint();
	}

	void LabelQueue::setDefaultMessage(juce::String msg, juce::Colour colour)
	{
		std::lock_guard<std::mutex> lock(mutex);
		defaultMessage.first = std::move(msg);
		defaultMessage.second = colour;
	}

	void LabelQueue::pushMessage(juce::String msg, juce::Colour colour, int timeout)
	{
		std::lock_guard<std::mutex> lock(mutex);
		queue.emplace(std::make_pair(std::move(msg), colour), timeout);
	}

	void LabelQueue::setDefaultPrefix(juce::String newPrefix)
	{
		std::lock_guard<std::mutex> lock(mutex);
		prefix = std::move(newPrefix);
	}

	void LabelQueue::pulseQueue()
	{
#ifdef _DEBUG
		if (auto* manager = juce::MessageManager::getInstance(); !manager || !manager->isThisTheMessageThread())
			CPL_RUNTIME_EXCEPTION("Message queue pulsed outside of the main thread");
#endif
		std::lock_guard<std::mutex> lock(mutex);

		updateQueue();
	}

	void LabelQueue::addListener(Listener& l) { listener = &l; }

	void LabelQueue::removeListener(Listener& l) { listener = nullptr; }

	void LabelQueue::updateQueue()
	{
		if (queue.size())
		{
			const auto time = cpl::Misc::QuickTime();

			auto & front = queue.front();
			if (!front.timeStamp)
			{
				// new message: no timestamp yet
				// set it as the message, and stamp it
				currentMessage = &front.msg;
				front.timeStamp = cpl::Misc::QuickTime();

				notifyListeners();

				return;
			}
			// message already has a timestop (ie. it is shown) - check if it's been
			// shown for it's period
			else if (time > (front.timeStamp + front.timeOut))
			{
				queue.pop();

				notifyListeners();

				if (queue.size())
				{
					// we have now invalidated currentMessage, so we need to get a new
					// front - this looks like a case for recursion, however
					// the mutex lock makes the code non-reentrant.
					auto & front = queue.front();
					if (!front.timeStamp)
					{
						// new message: no timestamp yet
						// set it as the message, and stamp it
						currentMessage = &front.msg;
						front.timeStamp = time;
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

	void LabelQueue::notifyListeners()
	{
		if (listener)
			listener->onNewMessage(*this);
	}

}
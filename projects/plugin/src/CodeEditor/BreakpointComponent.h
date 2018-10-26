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

	file:BreakpointComponent.h
	
		A component that manages red dots aligned with source code lines in a code editor

*************************************************************************************/

#ifndef BREAKPOINTCOMPONENT_H
#define BREAKPOINTCOMPONENT_H

#include <cpl/Common.h>
#include <set>
#include "CodeTextEditor.h"

namespace ape
{

	class BreakpointComponent
		: public juce::Component
		, private juce::CodeDocument::Listener
		, private CodeTextEditor::Listener
		, private juce::AsyncUpdater
	{
	public:

		class Listener
		{
		public:
			virtual void onBreakpointsChanged(const std::set<int>& breakpoints) = 0;
			virtual ~Listener() {}
		};


		BreakpointComponent(CodeTextEditor& codeEditorView)
			: document(codeEditorView.getDocument())
			, codeEditor(codeEditorView)
		{
			document.addListener(this);
			codeEditor.addCompositionListener(this);
		}


		void paint(juce::Graphics& g) override
		{
			auto const offset = 0;
			auto const radius = getBounds().getWidth() * 0.5f;
			auto numLines = codeEditor.getNumLinesOnScreen();
			auto scale = codeEditor.getLineHeight();
			auto start = codeEditor.getFirstLineOnScreen();

			g.fillAll({ 0x3E, 0x3E, 0x3E });
			g.setColour(juce::Colours::red);

			for (auto line : breakpoints)
			{
				auto position = line - start;
				juce::Rectangle<float> space{ 0, position * scale + 0.0f, radius * 2, radius * 2 };
				g.fillRoundedRectangle(space.reduced(1, 1), radius - 1);
			}
		}

		void mouseDown(const juce::MouseEvent& e) override
		{
			auto fractionalPosition = e.position.y / (getBounds().getHeight() - 1);
			auto scale = codeEditor.getNumLinesOnScreen() + 1;

			int sourceLine = static_cast<int>(e.position.y / codeEditor.getLineHeight() + codeEditor.getFirstLineOnScreen());
			auto it = breakpoints.find(sourceLine);

			if (it != breakpoints.end())
			{
				breakpoints.erase(it);
			}
			else
			{
				breakpoints.emplace(sourceLine);
			}

			repaint();

			notifyListeners();
		}

		void compositionChanged() override
		{
			triggerAsyncUpdate();
		}

		void handleAsyncUpdate() override
		{
			repaint();
		}

		const std::set<int>& getBreakpoints() const noexcept { return breakpoints; }

		void setBreakpoints(std::set<int> newBreakpoints)
		{
			breakpoints = std::move(newBreakpoints);
			repaint();
		}

		void addBreakpointListener(Listener* l)
		{
			listeners.emplace(l);
		}

		void removeBreakpointListener(Listener* l)
		{
			listeners.erase(l);
		}

		~BreakpointComponent()
		{
			codeEditor.getDocument().removeListener(this);
			codeEditor.removeCompositionListener(this);
		}

	private:

		void codeDocumentTextInserted(const juce::String& newText, int insertIndex) override
		{
			if (!breakpoints.empty() && newText.containsAnyOf("\r\n"))
			{
				breakpoints.clear();
				repaint();
				notifyListeners();
			}
		}

		virtual void codeDocumentTextDeleted(int startIndex, int endIndex) override
		{
			auto const text = codeEditor.getTextInRange({ startIndex, endIndex });

			if (!breakpoints.empty() && text.containsAnyOf("\r\n"))
			{
				breakpoints.clear();
				repaint();
				notifyListeners();
			}
		}

		void notifyListeners()
		{
			for (auto list : listeners)
				list->onBreakpointsChanged(breakpoints);
		}

		juce::CodeDocument& document;
		CodeTextEditor& codeEditor;
		std::set<int> breakpoints;
		std::set<Listener*> listeners;
	};
}

#endif
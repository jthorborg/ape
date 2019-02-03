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

	file:CodeTextEditor.h
	
		Component handling editing of code

*************************************************************************************/

#ifndef CODETEXTEDITOR_H
#define CODETEXTEDITOR_H

#include <cpl/Common.h>
#include "../Settings.h"

namespace ape
{
	class CodeTextEditor
		: public juce::CodeEditorComponent
	{
	public:

		class Listener
		{
		public:
			virtual void compositionChanged() = 0;
			virtual ~Listener() { }
		};

		CodeTextEditor(const Settings& setting, juce::CodeDocument& document, juce::CodeTokeniser* tokenizer)
			: juce::CodeEditorComponent(document, tokenizer)
		{
			setColour(backgroundColourId, setting.lookUpValue(juce::Colour{ 0x1E, 0x1E, 0x1E }, "editor", "colours", "background"));
			setColour(highlightColourId, setting.lookUpValue(juce::Colour{ 0x26, 0x4f, 0x78 }, "editor", "colours", "highlight"));
			setColour(lineNumberBackgroundId, setting.lookUpValue(juce::Colour{ 0x1E, 0x1E, 0x1E }, "editor", "colours", "line_number", "background"));
			setColour(lineNumberTextId, setting.lookUpValue(juce::Colour{ 0x7E, 0x7E, 0xAE }, "editor", "colours", "line_number", "text"));
			setColour(juce::CaretComponent::caretColourId, setting.lookUpValue(juce::Colours::white, "editor", "colours", "caret"));

			autoIndent = setting.lookUpValue(true, "editor", "auto_indent");
		}

		void paint(juce::Graphics& g) override
		{
			auto const current = getFirstLineOnScreen();

			if (current != previousLine)
			{
				previousLine = current;

				for (auto listener : compositionListeners)
					listener->compositionChanged();
			}

			juce::CodeEditorComponent::paint(g);
		}

		void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& d) override
		{
			if (e.mods.isCtrlDown())
				return;

			juce::CodeEditorComponent::mouseWheelMove(e, d);
		}

		void handleReturnKey() override
		{
			juce::CodeEditorComponent::handleReturnKey();

			if (!autoIndent)
				return;

			auto position = getCaretPos();
			auto previousLine = position.movedByLines(-1).getLineText();

			int length = previousLine.length();
			juce::String things;

			for (int i = 0; i < previousLine.length(); ++i)
			{
				bool stop = false;
				auto current = previousLine[i];
				switch (current)
				{
				case ' ':
				case '\t':
					things += current;
					break;
				default:
					stop = true;
					break;
				}

				if (stop)
					break;
			}

			getDocument().insertText(position, things);
		}

		void addCompositionListener(Listener* l) { compositionListeners.emplace(l); }
		void removeCompositionListener(Listener* l) { compositionListeners.erase(l); }

	private:
		bool autoIndent = true;
		int previousLine = -1;
		std::set<Listener*> compositionListeners;
	};

}

#endif
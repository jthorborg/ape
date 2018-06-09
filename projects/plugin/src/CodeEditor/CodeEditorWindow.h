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

	file:CodeEditorWindow.h
		
		The JUCE code editor editorWindow

*************************************************************************************/

#ifndef APE_JUCEEDITORWINDOW_H
	#define APE_JUCEEDITORWINDOW_H

	#include "../Common.h"
	#include <cpl/CExclusiveFile.h>
	#include <string>
	#include "CodeTokeniser.h"
	#include <set>

	namespace ape
	{
		enum Command
		{
			InvalidCommand = -1,
			Start = 1,
			FileNew = Start,
			FileOpen,
			FileSave,
			FileSaveAs,
			FileExit,
			/* -- following are natively supported
			EditCut,
			EditCopy,
			EditUndo,
			EditRedo,
			EditPaste,
			EditDelete,
			EditSelectAll
			*/

			End
		};

		/// <summary>
		/// Enumerator to index arrays after name
		/// </summary>
		enum Menus
		{
			File,
			Edit
		};

		/// <summary>
		/// Describes an entry in the menu, with optional shortcut
		/// </summary>
		struct MenuEntry
		{
			MenuEntry(const std::string & name, int key = 0, juce::ModifierKeys modifier = juce::ModifierKeys(), Command c = InvalidCommand)
				: name(name)
				, key(key)
				, modifier(modifier)
				, command(c)
			{

			}

			bool hasShortCut() { return key || modifier.testFlags(juce::ModifierKeys::noModifiers); }

			std::string name;
			int key;
			juce::ModifierKeys modifier;
			Command command;

		};

		extern const MenuEntry CommandTable[][6];

		class CodeEditorComponent : public juce::CodeEditorComponent
		{
		public:

			CodeEditorComponent(juce::CodeDocument& document, juce::CodeTokeniser* tokenizer)
				: juce::CodeEditorComponent(document, tokenizer)
			{
				setColour(backgroundColourId, { 0x1E, 0x1E, 0x1E });
				setColour(highlightColourId, { 0x26, 0x4f, 0x78 });
				setColour(lineNumberBackgroundId, { 0x1E, 0x1E, 0x1E });
				setColour(lineNumberTextId, { 0x7E, 0x7E, 0xAE });
				setColour(juce::CaretComponent::caretColourId, juce::Colours::white);
			}

			class Listener
			{
			public:
				virtual void compositionChanged() = 0;
				virtual ~Listener() { }
			};

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

			void addCompositionListener(Listener* l) { compositionListeners.emplace(l); }
			void removeCompositionListener(Listener* l) { compositionListeners.erase(l); }

		private:

			int previousLine = -1;
			std::set<Listener*> compositionListeners;
		};


		class BreakpointComponent 
			: public juce::Component
			, private juce::CodeDocument::Listener
			, private CodeEditorComponent::Listener
			, private juce::AsyncUpdater
		{
		public:

			class BreakpointListener
			{
			public:
				virtual void onBreakpointsChanged(const std::set<int>& breakpoints) = 0;
				virtual ~BreakpointListener() {}
			};

			BreakpointComponent(CodeEditorComponent& codeEditorView)
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

			const std::set<int> getTracedLines() const noexcept { return breakpoints; }
			
			void addTraceListener(BreakpointListener* l)
			{
				listeners.emplace(l);
			}

			void removeTraceListener(BreakpointListener* l)
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
			CodeEditorComponent& codeEditor;
			std::set<int> breakpoints;
			std::set<BreakpointListener*> listeners;
		};

		class InternalCodeEditorComponent : public juce::Component
		{
		public:

			InternalCodeEditorComponent(juce::CodeDocument& doc)
				: cec(doc, &tokeniser)
				, tracer(cec)
				, scale(1.0f)
			{
				wrapper.setVisible(true);
				addChildComponent(wrapper);
				wrapper.addChildComponent(cec);
				wrapper.addChildComponent(tracer);
				cec.setLineNumbersShown(true);

				cec.setVisible(true);
				tracer.setVisible(true);
				cec.addMouseListener(this, true);
			}

			void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& details) override
			{
				if (e.mods.isCtrlDown())
				{
					scale *= 1.0f + details.deltaY / 16.0f;
					wrapper.setTransform(juce::AffineTransform::identity.scaled(scale));
					resized();
				}
			}

			void resized() override
			{
				auto bounds = juce::Rectangle<float>(0, 0, getWidth() / scale, getHeight() / scale);
				wrapper.setBounds(bounds.toType<int>());
				tracer.setBounds(bounds.withRight(10).toType<int>());
				cec.setBounds(bounds.withLeft(10).toType<int>());
			}

			BreakpointComponent& getLineTracer() noexcept { return tracer; }

		private:
			float scale;
			juce::Component wrapper;
			CodeTokeniser tokeniser;
			CodeEditorComponent cec;
			BreakpointComponent tracer;

		};

		class CodeEditorWindow : public juce::DocumentWindow, public juce::MenuBarModel
		{
		public:

			CodeEditorWindow(juce::CodeDocument& cd);
			void paint(juce::Graphics& g) override;
			virtual ~CodeEditorWindow();
			void closeButtonPressed() override;
			void setAppCM(juce::ApplicationCommandManager* acm);

			BreakpointComponent& getLineTracer() { return codeEditor.getLineTracer(); }

		private:

			juce::StringArray getMenuBarNames() override;
			juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
			void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

			// instance data
			juce::ApplicationCommandManager* appCM;
			InternalCodeEditorComponent codeEditor;
		};

	}; // class ape
#endif
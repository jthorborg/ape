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

	file:JuceEditorWindow.h
		
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

			using juce::CodeEditorComponent::CodeEditorComponent;

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

			void addCompositionListener(Listener* l) { compositionListeners.emplace(l); }
			void removeCompositionListener(Listener* l) { compositionListeners.erase(l); }

		private:

			int previousLine = -1;
			std::set<Listener*> compositionListeners;
		};


		class LineTraceComponent : public juce::Component, private juce::CodeDocument::Listener, private CodeEditorComponent::Listener, private juce::AsyncUpdater
		{
		public:

			class TraceListener
			{
			public:
				virtual void onTracesChanged(const std::set<int>& traces) = 0;
				virtual ~TraceListener() {}
			};

			LineTraceComponent(CodeEditorComponent& codeEditorView)
				: document(codeEditorView.getDocument())
				, codeEditor(codeEditorView)
			{
				document.addListener(this);
				codeEditor.addCompositionListener(this);
			}


			void paint(juce::Graphics& g) override
			{
				auto const radius = getBounds().getWidth() * 0.5f;
				auto numLines = codeEditor.getNumLinesOnScreen();
				auto scale = codeEditor.getLineHeight();
				auto start = codeEditor.getFirstLineOnScreen();

				g.fillAll(juce::Colours::grey);
				g.setColour(juce::Colours::red);

				for (auto line : traces)
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
				auto it = traces.find(sourceLine);

				if (it != traces.end())
				{
					traces.erase(it);
				}
				else
				{
					traces.emplace(sourceLine);
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

			const std::set<int> getTracedLines() const noexcept { return traces; }
			
			void addTraceListener(TraceListener* l)
			{
				listeners.emplace(l);
			}

			void removeTraceListener(TraceListener* l)
			{
				listeners.erase(l);
			}

			~LineTraceComponent()
			{
				codeEditor.getDocument().removeListener(this);
				codeEditor.removeCompositionListener(this);
			}

		private:

			void codeDocumentTextInserted(const juce::String& newText, int insertIndex) override
			{
				if (!traces.empty() && newText.containsAnyOf("\r\n"))
				{
					traces.clear();
					repaint();
					notifyListeners();
				}
			}

			virtual void codeDocumentTextDeleted(int startIndex, int endIndex) override
			{
				auto const text = codeEditor.getTextInRange({ startIndex, endIndex });

				if (!traces.empty() && text.containsAnyOf("\r\n"))
				{
					traces.clear();
					repaint();
					notifyListeners();
				}
			}

			void notifyListeners()
			{
				for (auto list : listeners)
					list->onTracesChanged(traces);
			}

			juce::CodeDocument& document;
			CodeEditorComponent& codeEditor;
			std::set<int> traces;
			std::set<TraceListener*> listeners;
		};

		class InternalCodeEditorComponent : public juce::Component
		{
		public:

			InternalCodeEditorComponent(juce::CodeDocument& doc)
				: cec(doc, &tokeniser)
				, tracer(cec)
			{
				addChildComponent(cec);
				addChildComponent(tracer);
				cec.setLineNumbersShown(true);

				cec.setVisible(true);
				tracer.setVisible(true);
			}

			void resized() override
			{
				auto bounds = getBounds().withPosition({ 0, 5 });

				tracer.setBounds(bounds.withRight(10));
				cec.setBounds(bounds.withLeft(10));
			}

			LineTraceComponent& getLineTracer() noexcept { return tracer; }

		private:
			CodeTokeniser tokeniser;
			CodeEditorComponent cec;
			LineTraceComponent tracer;

		};



		class JuceEditorWindow : public juce::DocumentWindow, public juce::MenuBarModel
		{
		public:

			JuceEditorWindow(juce::CodeDocument& cd);
			virtual ~JuceEditorWindow();
			void closeButtonPressed() override;
			void setAppCM(juce::ApplicationCommandManager* acm);

			LineTraceComponent& getLineTracer() { return codeEditor.getLineTracer(); }

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
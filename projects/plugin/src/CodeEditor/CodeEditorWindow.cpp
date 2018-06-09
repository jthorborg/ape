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
 
	 file:SourceProjectManager.cpp
	 
		Implementation of the juce editor
 
 *************************************************************************************/


#include "CodeEditorWindow.h"

namespace ape
{
	namespace fs = std::experimental::filesystem;

	#ifdef __MAC__
		#define CTRLCOMMANDKEY juce::ModifierKeys::Flags::commandModifier
	#else
		#define CTRLCOMMANDKEY juce::ModifierKeys::Flags::ctrlModifier
	#endif

	const MenuEntry CommandTable[][6] =
	{
		// File
		{
			{"",			0,		0,				Command::InvalidCommand}, // dummy element - commands are 1-based index cause of juce
			{ "New File",	'n',	CTRLCOMMANDKEY, Command::FileNew },
			{ "Open...",	'o',	CTRLCOMMANDKEY, Command::FileOpen },
			{ "Save",		's',	CTRLCOMMANDKEY,	Command::FileSave },
			{ "Save As...",	0,		0,				Command::FileSaveAs },
			{ "Exit",		0,		0,				Command::FileExit}
		},
		// Edit

	};

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

		void handleReturnKey() override
		{
			if (!autoIndent)
				return;

			juce::CodeEditorComponent::handleReturnKey();

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


	class BreakpointComponent
		: public juce::Component
		, private juce::CodeDocument::Listener
		, private CodeEditorComponent::Listener
		, private juce::AsyncUpdater
	{
	public:

		using BreakpointListener = CodeEditorWindow::BreakpointListener;

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

	CodeEditorWindow::CodeEditorWindow(juce::CodeDocument& cd)
		: DocumentWindow(cpl::programInfo.name + " editor", juce::Colours::grey, DocumentWindow::TitleBarButtons::allButtons)
		, codeEditor(std::make_unique<InternalCodeEditorComponent>(cd))
		, appCM(nullptr)
	{
		codeEditor->setVisible(true);
		setMenuBar(this);
		setResizable(true, true);
		setUsingNativeTitleBar(true);
		setContentNonOwned(codeEditor.get(), false);

		setBounds(getBounds().withPosition(100, 100));
	}

	void CodeEditorWindow::paint(juce::Graphics& g)
	{
		g.fillAll({ 0x1E, 0x1E, 0x1E });
	}


	CodeEditorWindow::~CodeEditorWindow()
	{
		setMenuBar(nullptr);
	}

	juce::PopupMenu CodeEditorWindow::getMenuForIndex(int topLevelMenuIndex, const juce::String & menuName)
	{
		juce::PopupMenu ret;

		switch (topLevelMenuIndex)
		{
			case Menus::File:
			{
				for (int i = Command::Start; i < Command::End; ++i)
					ret.addCommandItem(appCM, i);
				break;
			}
			case Menus::Edit:
			{

				break;
			}
		}
		return ret;
	}

	void CodeEditorWindow::menuItemSelected(int menuItemID, int topLevelMenuIndex) { }


	juce::StringArray CodeEditorWindow::getMenuBarNames()
	{
		juce::StringArray ret;
		ret.add("File");
		//ret.add("Edit");
		return ret;
	}

	void CodeEditorWindow::closeButtonPressed()
	{
		setVisible(false);
	}

	void CodeEditorWindow::setAppCM(juce::ApplicationCommandManager* acm)
	{
		// set the application command manager that is associated with this editorWindow
		appCM = acm;
		if (appCM)
			addKeyListener(appCM->getKeyMappings());
	}

	void CodeEditorWindow::addBreakpointListener(BreakpointListener * listener)
	{
		codeEditor->getLineTracer().addTraceListener(listener);
	}

	void CodeEditorWindow::removeBreakpointListener(BreakpointListener * listener)
	{
		codeEditor->getLineTracer().removeTraceListener(listener);
	}

	const std::set<int>& CodeEditorWindow::getBreakpoints()
	{
		return codeEditor->getLineTracer().getTracedLines();
	}

}
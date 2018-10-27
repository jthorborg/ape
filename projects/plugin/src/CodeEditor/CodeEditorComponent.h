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

	file:CodeEditorComponent.h
	
		Complete code editor with features and breakpoints

*************************************************************************************/

#ifndef CODEEDITORCOMPONENT_H
#define CODEEDITORCOMPONENT_H

#include <memory>
#include <set>
#include <cpl/Common.h>
#include <cpl/state/Serialization.h>
#include "cpl/gui/Tools.h"

#include "../Settings.h"
#include "CLangCodeTokeniser.h"
#include "CodeTextEditor.h"
#include "BreakpointComponent.h"
#include "CodeDocumentListener.h"

namespace ape
{
	class CodeEditorComponent
		: public juce::Component
		, public cpl::SafeSerializableObject
		, public cpl::DestructionNotifier
		, private CodeDocumentListener
	{
	public:

		CodeEditorComponent(const Settings& settings, std::shared_ptr<juce::CodeDocument> doc, CodeDocumentSource& documentSource)
			: document(doc)
			, tokeniser(settings)
			, textEditor(settings, *document, &tokeniser)
			, tracer(textEditor)
			, scale(1.0f)
			, dirty(false)
			, source(documentSource)
		{
			wrapper.setVisible(true);
			addChildComponent(wrapper);
			wrapper.addChildComponent(textEditor);
			wrapper.addChildComponent(tracer);
			textEditor.setLineNumbersShown(true);

			textEditor.setVisible(true);
			tracer.setVisible(true);
			textEditor.addMouseListener(this, true);

			scale = settings.lookUpValue(1.0f, "editor", "zoom");
			rescale(scale);

			source.addListener(*this);
		}

		~CodeEditorComponent()
		{
			source.removeListener(*this);
			notifyDestruction();
		}

		void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& details) override
		{
			if (e.mods.isCtrlDown())
			{
				rescale(scale * 1.0f + details.deltaY / 16.0f);
			}
		}

		void rescale(float newScale)
		{
			scale = newScale;
			wrapper.setTransform(juce::AffineTransform::identity.scaled(scale));
			resized();
		}

		void resized() override
		{
			auto bounds = juce::Rectangle<float>(0, 0, getWidth() / scale, getHeight() / scale);
			wrapper.setBounds(bounds.toType<int>());
			tracer.setBounds(bounds.withRight(10).toType<int>());
			textEditor.setBounds(bounds.withLeft(10).toType<int>());
		}

		BreakpointComponent& getLineTracer() noexcept { return tracer; }

		void serialize(cpl::CSerializer::Archiver& ar, cpl::Version version) override
		{
			ar << scale;
		}

		void deserialize(cpl::CSerializer::Builder& builder, cpl::Version version) override
		{
			float newScale;
			builder >> newScale;
			if (newScale != scale)
				rescale(newScale);
		}

		void documentChangedName(const cpl::string_ref newName) override
		{
			currentName = newName;
			setName(dirty ? "* " : "" + currentName);
		}

		void documentDirtynessChanged(bool isDirty) override
		{
			dirty = isDirty;
			setName(isDirty ? "* " : "" + currentName);
		}

	private:

		CodeDocumentSource& source;
		std::string currentName;
		std::shared_ptr<juce::CodeDocument> document;
		CLangCodeTokeniser tokeniser;
		juce::Component wrapper;
		CodeTextEditor textEditor;
		BreakpointComponent tracer;
		float scale;
		bool dirty;
	};
}

#endif
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
 
	 file:CodeEditorComponent.cpp
	 
		Implementation of the juce editor
 
 *************************************************************************************/

#include <cpl/filesystem.h>
#include "CodeEditorComponent.h"
#include "SourceProjectManager.h"

namespace ape
{
    CodeEditorComponent::CodeEditorComponent(const Settings & settings, std::shared_ptr<juce::CodeDocument> doc, SourceProjectManager & manager)
        : document(doc)
        , tokeniser(settings)
        , textEditor(settings, *document, &tokeniser)
        , tracer(textEditor)
        , scale(1.0f)
        , dirty(false)
        , source(manager)
        , menuModel(manager)
        , menuComponent(&menuModel)
    {
        addChildComponent(wrapper);
        addAndMakeVisible(menuComponent);
        addKeyListener(manager.getCommandManager().getKeyMappings());

        wrapper.setVisible(true);
        wrapper.addChildComponent(textEditor);
        wrapper.addChildComponent(tracer);
        textEditor.setLineNumbersShown(true);

        textEditor.setVisible(true);
        tracer.setVisible(true);
        tracer.setEditable(settings.lookUpValue(false, "editor", "enable_scopepoints"));

        textEditor.addMouseListener(this, true);

        scale = settings.lookUpValue(1.0f, "editor", "zoom");
        rescale(scale);

        source.addListener(*this);
    }

    inline CodeEditorComponent::~CodeEditorComponent()
    {
        source.removeListener(*this);
        notifyDestruction();
    }

    inline void CodeEditorComponent::mouseWheelMove(const juce::MouseEvent & e, const juce::MouseWheelDetails & details)
    {
        if (e.mods.isCtrlDown())
        {
            rescale(scale * 1.0f + details.deltaY / 16.0f);
        }
    }

    inline void CodeEditorComponent::rescale(float newScale)
    {
        scale = newScale;
        
		textEditor.setFont(textEditor.getFont().withHeight(12 * scale));
        resized();
    }

    inline void CodeEditorComponent::resized()
    {
        const int kMenuHeight = getLookAndFeel().getDefaultMenuBarHeight();

        menuComponent.setBounds(getLocalBounds().withHeight(kMenuHeight));

        auto bounds = juce::Rectangle<float>(0, kMenuHeight, getWidth(), getHeight() - kMenuHeight);
        wrapper.setBounds(bounds.toType<int>());
		// local space
        tracer.setBounds(bounds.withRight(10).withTop(0).toType<int>());
        textEditor.setBounds(bounds.withTop(0).withLeft(10).toType<int>());
    }

    BreakpointComponent & CodeEditorComponent::getLineTracer() noexcept 
    { 
        return tracer; 
    }

    inline void CodeEditorComponent::serialize(cpl::CSerializer::Archiver & ar, cpl::Version version)
    {
        ar << scale;
    }

    inline void CodeEditorComponent::deserialize(cpl::CSerializer::Builder & builder, cpl::Version version)
    {
        float newScale;
        builder >> newScale;
        if (newScale != scale)
            rescale(newScale);
    }

    inline void CodeEditorComponent::documentChangedName(const cpl::string_ref newName)
    {
        currentName = newName;
        setName("edit " + ((dirty ? " * " : " - ") + currentName));
    }

    inline void CodeEditorComponent::documentDirtynessChanged(bool isDirty)
    {
        dirty = isDirty;
        setName("edit" + ((isDirty ? " * " : " - ") + currentName));
    }
}

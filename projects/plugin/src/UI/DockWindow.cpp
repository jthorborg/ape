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

	file:DockWindow.cpp
	
		Implementation of DockWindow.h

*************************************************************************************/

#include "DockWindow.h"
#include "../MainEditor/MainEditor.h"

namespace ape
{

	DockWindow::DockWindow()
		: juce::DocumentWindow("DockWindow", juce::Colours::red, juce::DocumentWindow::allButtons)
		, editor(nullptr)
		, child(nullptr)
	{
		setUsingNativeTitleBar(true);
	}

	DockWindow::~DockWindow()
	{
		if (child)
			child->removeComponentListener(this);
	}

	const std::string & DockWindow::getNamePrefix()
	{
		return prefix;
	}

	void DockWindow::paint(juce::Graphics & g)
	{
		juce::ColourGradient backgroundGradient(juce::Colours::black, 0, 0, juce::Colour(37, 3, 55), getWidth() * 0.5f, getHeight() * 2, false);
		g.setGradientFill(backgroundGradient);
		g.fillAll();
	}

	void DockWindow::componentNameChanged(juce::Component & child)
	{
		setName(getNamePrefix() + " - " + child.getName());
	}

	void DockWindow::closeButtonPressed()
	{
		if(editor && child)
			editor->dockManager.attachComponentToDock(*child, editor->tabs);
	}
		
	void DockWindow::injectDependencies(MainEditor& mainEditor, juce::Component& childComponent)
	{
		setName(getNamePrefix() + " - " + childComponent.getName());
		editor = &mainEditor;
		child = &childComponent;
		child->addComponentListener(this);
	}

	void DockWindow::setPrefix(std::string newPrefix)
	{
		prefix = newPrefix;
	}

}

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

	file:DockWindow.h
	
		Document window super class for docks in ape

*************************************************************************************/

#ifndef DOCKWINDOW_H
#define DOCKWINDOW_H

#include <cpl/Common.h>
#include <cpl/gui/Tools.h>
#include <string>

namespace ape
{
	class MainEditor;

	class DockWindow 
		: public juce::DocumentWindow
		, public cpl::DestructionNotifier
		, private juce::ComponentListener
	{
	public:

		friend class MainEditor;

		DockWindow();

		~DockWindow();

	protected:

		const std::string& getNamePrefix();
		void paint(juce::Graphics& g) override final;

	private:

		void componentNameChanged(juce::Component& child) override;

		void closeButtonPressed() override final;
		void injectDependencies(MainEditor& editor, juce::Component& child);
		void setPrefix(std::string prefix);

		std::string prefix;
		MainEditor* editor;
		juce::Component* child;
	};
}

#endif
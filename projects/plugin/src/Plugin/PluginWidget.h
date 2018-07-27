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

	file:PluginWidget.h
	
		Definitions for plugin-created widgets (stuff that isn't parameters)

*************************************************************************************/

#ifndef APE_PLUGINWIDGET_H
	#define APE_PLUGINWIDGET_H

	#include <cpl/Common.h>

	namespace ape
	{
		class WidgetRecord;

		class PluginWidget
		{
		public:
			
			enum Type
			{
				Meter, Label, Plot
			};

			static std::unique_ptr<PluginWidget> FromRecord(WidgetRecord&& record);

			Type getType() const noexcept { return type; }
			virtual std::unique_ptr<juce::Component> createController() = 0;
			virtual ~PluginWidget() {}

		protected:

			PluginWidget(Type type) : type(type) {}

			Type type;
		};

	};
#endif
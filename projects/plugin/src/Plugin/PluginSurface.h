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

	file:PluginSurface.h
	
		A suitable controller for a given plugin instance

*************************************************************************************/

#ifndef APE_PLUGINSURFACE_H
	#define APE_PLUGINSURFACE_H

	#include <cpl/Common.h>
	#include <memory>
	
	namespace ape
	{
		class PluginState;
		class Engine;

		class PluginSurface : public juce::Component
		{
		public:
			friend class PluginState;
			PluginSurface(Engine& engine, PluginState& state);

		protected:

			void resized() override;

		private:

			void clearComponents();
			std::vector<std::unique_ptr<juce::Component>> components;
		};

	};
#endif
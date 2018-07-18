/*************************************************************************************

	Audio Programming Environment VST. 
		
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

	file:PluginParameter.h
		
		Polymorphic wrapper of a plugin parameter based on the C apis

*************************************************************************************/

#ifndef APE_PLUGINPARAMETER_H
	#define APE_PLUGINPARAMETER_H

	#include <ape/APE.h>
	#include "ParameterManager.h"

	namespace ape 
	{
		class ParameterRecord;

		class PluginParameter : ParameterManager::ExternalParameterTraits
		{
		public:

			static std::unique_ptr<PluginParameter> FromRecord(const ParameterRecord& record);

			virtual void setParameterRealtime(PFloat value) noexcept = 0;
			virtual ~PluginParameter() {}

		};

	}
#endif
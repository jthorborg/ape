/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.4.0.
	 
	 Copyright (C) 2017 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:SharedInterfaceEx.h
	
		TODO

*************************************************************************************/

#ifndef APE_SHAREDINTERFACEEX_H
	#define APE_SHAREDINTERFACEEX_H

	#include <ape/SharedInterface.h>
	#include "CApi.h"
	#include <string>

	namespace ape
	{
		class Engine;
		class PluginState;

		struct BindingsInterfaceResolver : public APE_SharedInterface
		{
			BindingsInterfaceResolver()
			{
#define APE_BIND(func) this->func = ape::api::func

				APE_BIND(abortPlugin);
				APE_BIND(getSampleRate);
				APE_BIND(printLine);
				APE_BIND(printThemedLine);
				APE_BIND(msgBox);
				APE_BIND(timerGet);
				APE_BIND(timerDiff);
				APE_BIND(alloc);
				APE_BIND(free);
				APE_BIND(setInitialDelay);
				APE_BIND(createLabel);
				APE_BIND(getNumInputs);
				APE_BIND(getNumOutputs);
				APE_BIND(createMeter);
				APE_BIND(getBPM);
				APE_BIND(createPlot);
				APE_BIND(presentTrace);
				APE_BIND(createNormalParameter);
				APE_BIND(createBooleanParameter);
				APE_BIND(createListParameter);
				APE_BIND(destroyResource);
				APE_BIND(loadAudioFile);
				APE_BIND(createFFT);
				APE_BIND(performFFT);
				APE_BIND(releaseFFT);
				APE_BIND(setTriggeringChannel);
				APE_BIND(createAudioOutputFile);
				APE_BIND(writeAudioFile);
				APE_BIND(closeAudioFile);
                APE_BIND(getPlayHeadPosition);
#undef APE_BIND
			}
		};

		struct SharedInterfaceEx : public BindingsInterfaceResolver
		{
			Engine & getEngine() noexcept { return engine; }
			PluginState & getCurrentPluginState() noexcept { return cstate; }
			const Engine & getEngine() const noexcept { return engine; }
			const PluginState & getCurrentPluginState() const noexcept { return cstate; }

			static SharedInterfaceEx & downcast(APE_SharedInterface & base) noexcept { return static_cast<SharedInterfaceEx &>(base); }

			SharedInterfaceEx(Engine & engine, PluginState & cstate)
				: engine(engine), cstate(cstate)
			{

			}

		private:
			Engine& engine;
			PluginState& cstate;
		};
	
	};
#endif

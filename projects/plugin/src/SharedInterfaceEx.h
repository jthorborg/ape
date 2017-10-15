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

	namespace APE
	{
		class Engine;
		class CState;

		struct BindingsInterfaceResolver : public APE_SharedInterface
		{
			BindingsInterfaceResolver()
			{
#define APE_GTOL(func) this->func = APE::func

				APE_GTOL(getSampleRate);
				APE_GTOL(printLine);
				APE_GTOL(msgBox);
				APE_GTOL(setStatus);
				APE_GTOL(createKnob);
				APE_GTOL(timerGet);
				APE_GTOL(timerDiff);
				APE_GTOL(alloc);
				APE_GTOL(free);
				APE_GTOL(createKnobEx);
				APE_GTOL(setInitialDelay);
				APE_GTOL(createLabel);
				APE_GTOL(getNumInputs);
				APE_GTOL(getNumOutputs);
				APE_GTOL(createMeter);
				APE_GTOL(createToggle);
				APE_GTOL(getBPM);
				APE_GTOL(getCtrlValue);
				APE_GTOL(setCtrlValue);
				APE_GTOL(createPlot);
				APE_GTOL(createRangeKnob);

				std::memset(&extra, 0, sizeof(extra));
#undef APE_GTOL
			}
		};

		struct SharedInterfaceEx : public BindingsInterfaceResolver
		{
			Engine & getEngine() noexcept { return engine; }
			CState & getCState() noexcept { return cstate; }
			const Engine & getEngine() const noexcept { return engine; }
			const CState & getCState() const noexcept { return cstate; }

			static SharedInterfaceEx & upcast(APE_SharedInterface & base) noexcept { return static_cast<SharedInterfaceEx &>(base); }

			SharedInterfaceEx(Engine & engine, CState & cstate)
				: engine(engine), cstate(cstate)
			{

			}

		private:
			Engine & engine;
			CState & cstate;
		};
	
	};
#endif
/*************************************************************************************

	Audio Programming Environment VST. 
		
		VST is a trademark of Steinberg Media Technologies GmbH.

    Copyright (C) 2013 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:PluginState.cpp
	
		Implementation of PluginState.h

*************************************************************************************/

#include <cpl/PlatformSpecific.h>
#include "SharedInterfaceEx.h"
#include "PluginState.h"
#include "Engine.h"
#include "CCodeGenerator.h"
#include "UIController.h"
#include "CConsole.h"
#include <sstream>
#include <cpl/Protected.h>
#include <cpl/gui/Tools.h>
#include "Plugin/PluginCommandQueue.h"
#include "Plugin/PluginParameter.h"
#include "Plugin/PluginSurface.h"
#include "Plugin/PluginWidget.h"
#include "Plugin/PluginAudioFile.h"
#include "Plugin/PluginFFT.h"
#include "Plugin/PluginAudioWriter.h"

namespace ape
{
	thread_local unsigned int fpuMask;

	struct ProjectReleaser
	{
		ProjectEx* project = nullptr;
		CCodeGenerator* gen = nullptr;
		void reset() { project = nullptr; gen = nullptr; }
		~ProjectReleaser() { if (project && gen) gen->releaseProject(*project);	}
	};

	PluginState::PluginState(Engine& engine, CCodeGenerator& codeGenerator, std::unique_ptr<ProjectEx> projectToUse) 
		: engine(engine)
		, generator(codeGenerator)
		, project(std::move(projectToUse))
		, state(STATUS_DISABLED)
		, config{}
		, playing(false)
		, enabled(false)
		, currentlyDisabling(false)
		, currentlyAborting(false)
		, abnormalBehaviour(false)
		, activating(false)
		, triggerSetThroughAPI(false)
		, pluginAllocator(64)
	{
		sharedObject = std::make_unique<SharedInterfaceEx>(engine, *this);
		project->iface = sharedObject.get();

		if (!generator.createProject(*project))
			throw CreateException("Error creating project...");

		ProjectReleaser scopedRelease { project.get(), &generator };

		if (!generator.compileProject(*project))
			throw CompileException("Error compiling project...");

		if (!generator.initProject(*project))
			throw InitException("Error initializing project...");

		outputFiles.emplace_back(std::unique_ptr<PluginStreamProducer>());

		scopedRelease.reset();
	}

	PluginState::~PluginState() 
	{
		ProjectReleaser scopedRelease { project.get(), &generator };
		if(enabled)
			disableProject();
	}

	/*bool PluginState::valueChanged(CBaseControl * control)
	{
		ScopedRefCount ref(*this);
		if (!ref.valid())
			return false;

		Event e;
		// set type to a change of ctrl value.
		e.eventType = CtrlValueChanged;
		// construct ctrlValueChange event object
		Events::CtrlValueChanged aevent = {};
		// set new values
		aevent.value = control->bGetValue();
		aevent.tag = control->bGetTag();
		e.event.eCtrlValueChanged = &aevent; 

		auto ret = WrapPluginCall("valueChanged() event",
			[&]
			{
				return generator.onEvent(*project, &e);
			}
		);

		if (ret.second)
		{
			internalDisable(std::move(ref), ret.first);
		}
		else if(ret.first == STATUS_HANDLED)
		{
			control->bRedraw();
			return true;
		}

		return false;
	} */

	void PluginState::setPlayState(bool shouldPlay)
	{
		CPL_RUNTIME_ASSERTION(enabled || activating);

		if (shouldPlay == playing)
			return;

		playing = shouldPlay;

		Event e;
		e.eventType = PlayStateChanged;
		Events::PlayStateChanged aevent = { shouldPlay };
		e.event.ePlayStateChanged = &aevent;

		dispatchEvent("playStateChanged()", e);
	}

	void PluginState::syncParametersToEngine(bool takeEngineValue)
	{
		auto& pManager = engine.getParameterManager();

		if (takeEngineValue)
		{
			for (std::size_t i = 0; i < parameters.size(); ++i)
			{
				parameters[i]->setParameterRealtime(pManager.getParameter(static_cast<ParameterManager::IndexHandle>(i)));
			}
		}
		else
		{
			if(!triggerSetThroughAPI)
				api::setTriggeringChannel(sharedObject.get(), static_cast<int>(config.inputs + 1));

			for (std::size_t i = 0; i < parameters.size(); ++i)
			{
				pManager.setParameter(static_cast<ParameterManager::IndexHandle>(i), parameters[i]->getValue());
			}
		}
	}

	std::shared_ptr<PluginSurface> PluginState::getOrCreateSurface()
	{
		CPL_RUNTIME_ASSERTION(enabled);

		if (auto ptr = surface.lock())
		{
			return ptr;
		}

		auto instance = std::make_shared<PluginSurface>(engine, *this);
		instance->setName(project->projectName);
		surface = instance;

		return instance;
	}


	bool PluginState::processReplacing(const float * const * in, float * const * out, std::size_t sampleFrames, std::size_t * profiledCycles) noexcept
	{
		if (!enabled)
			return false;

		processing.store(true, std::memory_order_release);

		auto ret = WrapPluginCall("processReplacing()",
			[&]
			{
				for (std::size_t i = 0; i < config.inputs; ++i)
				{
					pluginInputs[i] = protectedMemory[0].get<float>() + sampleFrames * i;
					std::memcpy(pluginInputs[i], in[i], sampleFrames * sizeof(float));
				}

				for (std::size_t i = 0; i < config.outputs; ++i)
				{
					pluginOutputs[i] = protectedMemory[1].get<float>() + sampleFrames * i;
				}

				for (std::size_t i = 0; i < parameters.size(); ++i)
				{
					parameters[i]->swapParameters(sampleFrames);
				}

				auto start = cpl::Misc::ClockCounter();
				auto result = generator.processReplacing(*project, pluginInputs.data(), pluginOutputs.data(), sampleFrames);

				if (profiledCycles != nullptr)
					*profiledCycles = cpl::Misc::ClockCounter() - start;

				for (std::size_t i = 0; i < config.outputs; ++i)
				{
					std::memcpy(out[i], pluginOutputs[i], sampleFrames * sizeof(float));
				}

				return result;
			}
		);

		// TODO: Needs to be here?
		abnormalBehaviour = ret.first != STATUS_OK || ret.second;
		processing.store(false, std::memory_order_release);

		return ret.first == STATUS_OK && !ret.second;
	}

	bool PluginState::initializeActivation()
	{
		CPL_RUNTIME_ASSERTION(!enabled);
		CPL_RUNTIME_ASSERTION(state == STATUS_DISABLED);
		CPL_RUNTIME_ASSERTION(!activating);

		activating = true;
		triggerSetThroughAPI = false;
		currentlyAborting = false;

		commandQueue = std::make_unique<PluginCommandQueue>();

		auto ret = WrapPluginCall("activating project",
			[&]
			{
				return generator.activateProject(*project);
			}
		);

		if (ret.second || ret.first != Status::STATUS_READY)
		{
			state = STATUS_ERROR;
			return false;
		}

		return true;
	}

	bool PluginState::finalizeActivation()
	{
		CPL_RUNTIME_ASSERTION(!enabled);
		CPL_RUNTIME_ASSERTION(state == STATUS_DISABLED);
		CPL_RUNTIME_ASSERTION(activating);

		consumeCommands();
		commandQueue.reset();

		state = Status::STATUS_READY;

		enabled = true;
		activating = false;
		return true;
	}

	bool PluginState::disableProject()
	{
		CPL_RUNTIME_ASSERTION(enabled);
		CPL_RUNTIME_ASSERTION(state == STATUS_READY);

		setPlayState(false);

		currentlyDisabling.store(true, std::memory_order_release);

		auto ret = WrapPluginCall("Disabling plugin", AlwaysPerformInvocation,
			[&]
			{
				return generator.disableProject(*project, abnormalBehaviour);
			}
		);

		// TODO: check RET code.
		if (ret.first != STATUS_OK || ret.second)
			state = STATUS_ERROR;
		else
			state = STATUS_DISABLED;

		currentlyDisabling.store(false, std::memory_order_release);

		cleanupResources();
		abnormalBehaviour = false;
		enabled = false;

		return state == STATUS_DISABLED;
	}


	SharedInterfaceEx & PluginState::getSharedInterface()
	{
		return *sharedObject.get();
	}

	void PluginState::setConfig(const IOConfig& newSettings)
	{
		CPL_RUNTIME_ASSERTION(enabled || activating);
		CPL_RUNTIME_ASSERTION(!playing);

		if (newSettings == config)
			return;

		while (protectedMemory.size() < 2)
			protectedMemory.emplace_back().setProtect(CMemoryGuard::protection::readwrite);

		if (!protectedMemory[0].resize<float>(newSettings.inputs * newSettings.blockSize) || !protectedMemory[1].resize<float>(newSettings.outputs * newSettings.blockSize))
			CPL_SYSTEM_EXCEPTION("Error allocating virtual protected memory for buffers");

		pluginInputs.resize(newSettings.inputs);
		pluginOutputs.resize(newSettings.outputs);

		config = newSettings;
		
		Event e;
		e.eventType = IOChanged;
		Events::IOChanged aevent{ config.inputs, config.outputs, config.blockSize, config.sampleRate };
		e.event.eIOChanged = &aevent;

		dispatchEvent("ioChanged() event", e);
	}

	Status PluginState::dispatchEvent(const char * reason, APE_Event& event)
	{
		auto ret = WrapPluginCall(reason,
			[&]
			{
				return generator.onEvent(*project, &event);
			}
		);

		return ret.first;
	}

	void PluginState::consumeCommands()
	{
		auto& queue = *commandQueue;
		for (std::size_t i = 0; i < queue.size(); ++i)
		{
			switch (queue[i].getCommandType())
			{
				case CommandType::Parameter:
				{
					auto& parameterRecord = static_cast<ParameterRecord&>(queue[i]);
					parameters.emplace_back(PluginParameter::FromRecord(std::move(parameterRecord)));
					break;
				}

				case CommandType::Widget:
				{
					auto& widgetRecord = static_cast<WidgetRecord&>(queue[i]);
					widgets.emplace_back(PluginWidget::FromRecord(std::move(widgetRecord)));
				}
			}
		}

		auto& pManager = engine.getParameterManager();
		for (std::size_t i = 0; i < parameters.size(); ++i)
		{
			pManager.emplaceTrait(
				static_cast<ParameterManager::IndexHandle>(i), 
				*parameters[i]
			);
		}

		pManager.getParameterSet().addRTListener(this, true);
	}

	void PluginState::cleanupResources()
	{
		engine.getParameterManager().getParameterSet().removeRTListener(this, true);

		if (auto ptr = surface.lock())
		{
			ptr->clearComponents();
		}

		surface.reset();

		// cleanup parameter manager

		for (std::size_t i = 0; i < parameters.size(); ++i)
		{
			engine.getParameterManager().clearTraitIfMatching(static_cast<ParameterManager::IndexHandle>(i), *parameters[i]);
		}

		// kill parameters
		parameters.clear();
		widgets.clear();

		pluginAllocator.clear();
	}

	void PluginState::parameterChangedRT(cpl::Parameters::Handle localHandle, cpl::Parameters::Handle globalHandle, ParameterSet::BaseParameter * param) 
	{
		if (localHandle < parameters.size())
			parameters[localHandle]->setParameterRealtime(param->getValue());
	}

	template<typename Function>
	std::pair<Status, bool> PluginState::WrapPluginCall(const char * reason, Function&& f)
	{
		return WrapPluginCall(reason, DisregardInvocationIfErrorState, std::move(f));
	}

	template<typename Function>
	std::pair<Status, bool> PluginState::WrapPluginCall(const char * reason, PluginState::InvocationSemantics semantics, Function&& f)
	{
		api::clearThreadFaults();

		if (semantics == DisregardInvocationIfErrorState && (abnormalBehaviour || state == STATUS_ERROR))
			return { state, true };

		try
		{
			Status ret;

			cpl::CProtected::instance().runProtectedCode(
				[&] { ret = f(); }
			);

			return { ret, false };
		}
		catch (const cpl::CProtected::CSystemException& exp)
		{
			engine.getController().getConsole().printLine(
				CConsole::Error,
				"[PluginState] : Exception 0x%X occured in plugin at operation: %s: \"%s\". Plugin disabled.",
				exp.data.exceptCode, 
				reason,
				cpl::CProtected::formatExceptionMessage(exp).c_str()
			);

			switch (exp.data.exceptCode)
			{
				case cpl::CProtected::CSystemException::access_violation:
				{
					bool bad = true;
					for (auto& mem : protectedMemory)
					{
						if (mem.in_range(exp.data.attemptedAddr))
						{
							bad = false;
							break;
						}
					}
					
					if (!bad)
					{
						engine.getController().getConsole().printLine(CConsole::Warning, "[PluginState] : Plugin accessed memory out of bounds. "
							"Consider saving your project and restarting your application.");
					}
					else
					{
						const char * errString = "Your plugin has performed an illegal operation and has crashed! "
							"it is adviced that you immediately save your project (perhaps in a new file) and restart your "
							"application, as memory might have been corrupted. Consider going through "
							"your code and double-check it for errors, especially pointer dereferences "
							"and loops that might cause segmentation faults.";

						cpl::Misc::MsgBox(errString, cpl::programInfo.name + " Fatal Error",
							cpl::Misc::MsgStyle::sOk | cpl::Misc::MsgIcon::iStop, engine.getController().getSystemWindow(), false);
					}
				}
			}
		}
		catch (const AbortException& exp)
		{
			engine.getController().getConsole().printLine(
				CConsole::Warning,
				"[PluginState] : Plugin aborted at operation: %s: \"%s\". Plugin disabled.",
				reason,
				exp.what()
			);
		}
		catch (const std::runtime_error& err)
		{
			engine.getController().getConsole().printLine(
				CConsole::Warning,
				"[PluginState] : Runtime error at operation: %s: \"%s\". Plugin disabled.",
				reason,
				err.what()
			);
		}
		catch (const std::exception& err)
		{
			engine.getController().getConsole().printLine(
				CConsole::Warning,
				"[PluginState] : Unknown exception at operation: %s: \"%s\". Plugin disabled.",
				reason,
				err.what()
			);
		}

		abnormalBehaviour = true;
		return { STATUS_ERROR, true };
	}

	/*
		unless you want to have a heart attack, dont look at this code.
	*/
	#ifdef CPL_MSVC
		#pragma fenv_access (on)
	#else
		#pragma STDC FENV_ACCESS on
	#endif
	/*********************************************************************************************

		Toggle floating point exceptions on and off.

	 *********************************************************************************************/
	void PluginState::useFPUExceptions(bool bVal) 
	{
		// always call this!!#
		#ifdef CPL_MSVC
			_clearfp();
			unsigned int nfpcw = 0;
			if(bVal)
			{
				unsigned int temp;
				// save old environment
				_controlfp_s(&fpuMask, 0, 0);

				// Make the new fp env same as the old one,
				// except for the changes we're going to make
				nfpcw = _EM_INVALID | _EM_DENORMAL | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW; 
				//Update the control word with our changes
				_controlfp_s(&temp, 1, nfpcw);
			}
			else 
			{
				_controlfp_s(&nfpcw, 1, fpuMask);
			}
		#else
			if(bVal)
			{
				// always call this - changes in masks may trigger exceptions
				std::feclearexcept(FE_ALL_EXCEPT);
				fpuMask = _MM_GET_EXCEPTION_MASK();
				
				unsigned nfpcw = _MM_MASK_INVALID | _MM_MASK_DENORM | _MM_MASK_DIV_ZERO | _MM_MASK_OVERFLOW
								| _MM_MASK_UNDERFLOW |_MM_MASK_INEXACT;
				
				_MM_SET_EXCEPTION_MASK(fpuMask & ~nfpcw);

			}
			else
			{
				// always call this - changes in masks may trigger exceptions
				std::feclearexcept(FE_ALL_EXCEPT);
				// check return value here?
				_MM_SET_EXCEPTION_MASK(fpuMask);
			}
		#endif
	}

}

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
#include "Engine/PluginCommandQueue.h"

namespace ape
{
	thread_local unsigned int fpuMask;

	struct ProjectReleaser
	{
		ProjectEx* project = nullptr;
		CCodeGenerator* gen = nullptr;
		~ProjectReleaser() { if (project && gen) gen->releaseProject(*project);	}
	};

	PluginState::PluginState(Engine& engine, CCodeGenerator& codeGenerator, std::unique_ptr<ProjectEx> projectToUse) 
		: engine(engine)
		, generator(codeGenerator)
		, project(std::move(projectToUse))
		, ctrlManager(this, CRect(138, 0, 826 - 138, 245), kTagEnd)
		, expired(true)
		, useCount(0)
		, state(STATUS_DISABLED)
		, config{}
		, playing(false)
		, pendingDisable(false)
		, abnormalBehaviour(false)
	{
		sharedObject = std::make_unique<SharedInterfaceEx>(engine, *this);
		commandQueue = std::make_unique<PluginCommandQueue>();
		project->iface = sharedObject.get();

		if (!generator.createProject(*project))
			throw CreateException("Error creating project...");

		ProjectReleaser scopedRelease { project.get(), &generator };

		if (!generator.compileProject(*project))
			throw CompileException("Error compiling project...");

		if (!generator.initProject(*project))
			throw InitException("Error initializing project...");

		scopedRelease = ProjectReleaser{};
	}

	PluginState::~PluginState() 
	{
		ProjectReleaser scopedRelease { project.get(), &generator };
		disableProject();
	}

	bool PluginState::valueChanged(CBaseControl * control)
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
	}

	void PluginState::setPlayState(bool isPlaying)
	{
		if (isPlaying == playing)
			return;

		playing = isPlaying;

		Event e;
		e.eventType = PlayStateChanged;
		Events::PlayStateChanged aevent = { isPlaying };
		e.event.ePlayStateChanged = &aevent;

		dispatchEvent("playStateChanged()", e);
	}


	Status PluginState::processReplacing(const float * const * in, float * const * out, std::size_t sampleFrames, std::size_t * profiledCycles) noexcept
	{
		ScopedRefCount ref(*this);
		if (!ref.valid())
			return Status::STATUS_DISABLED;
		
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

		processing.store(false, std::memory_order_release);

		if(ret.second)
		{
			internalDisable(std::move(ref), ret.first);
		}

		return ret.first;
	}

	Status PluginState::disableProject()
	{
		ScopedRefCount ref(*this);
		if (ref.valid())
			internalDisable(std::move(ref), Status::STATUS_OK);

		waitDisable();

		return state == STATUS_DISABLED ? Status::STATUS_OK : Status::STATUS_ERROR;
	}

	Status PluginState::activateProject()
	{
		{
			cpl::CMutex lock(expiredMutex);

			if (!expired)
				throw InvalidStateException("Double activation of project");

			if (useCount != 0)
				throw InvalidStateException("Corrupt reference count");
		}


		auto ret = WrapPluginCall("activating project",
			[&]
			{
				return generator.activateProject(*project);
			}
		);

		if (ret.second || ret.first != Status::STATUS_READY)
			return Status::STATUS_ERROR;

		cpl::CMutex lock(expiredMutex);
		expired = false;
		useCount = 1;

		state = Status::STATUS_READY;

		Event e;
		e.eventType = IOChanged;
		Events::IOChanged aevent{ config.inputs, config.outputs, config.blockSize, config.sampleRate };
		e.event.eIOChanged = &aevent;
		dispatchEvent("initial ioChanged() event", e);

		if (playing)
		{
			Events::PlayStateChanged pevent{ true };
			e.event.ePlayStateChanged = &pevent;
			e.eventType = PlayStateChanged;
			dispatchEvent("initial playStateChanged() event", e);
		}

		return Status::STATUS_READY;
	}

	void PluginState::waitDisable()
	{
		if (pendingDisable.load())
		{
			if (juce::MessageManager::getInstance()->isThisTheMessageThread())
			{
				notifyDestruction();
				performDisable();
			}
			else
			{
				// TODO: Potential deadlock on the message thread.
				while (pendingDisable.load(std::memory_order_relaxed))
					std::this_thread::yield();
			}
		}
	}

	void PluginState::internalDisable(PluginState::ScopedRefCount::ScopedDisable disable, Status errorCode)
	{
		{
			cpl::CFastMutex lock(expiredMutex);
			bool prev = pendingDisable.exchange(true);
			if (prev || state == STATUS_DISABLED)
				return;
			abnormalBehaviour = errorCode != Status::STATUS_OK;
			state = STATUS_DISABLED;
			expired = true;
		}

		if (juce::MessageManager::getInstance()->isThisTheMessageThread())
			performDisable();
		else
			cpl::GUIUtils::MainEvent(*this, [this] { if(pendingDisable) performDisable(); });
	}

	void PluginState::performDisable()
	{
		useCount--;

		while (useCount.load(std::memory_order_relaxed) > 0)
			std::this_thread::yield();

		auto ret = WrapPluginCall("Disabling plugin",
			[&]
			{
				return generator.disableProject(*project, abnormalBehaviour);
			}
		);

		// TODO: check RET code.

		pluginAllocator.clear();
		ctrlManager.reset();
		abnormalBehaviour = false;
		pendingDisable = false;
	}

	SharedInterfaceEx & PluginState::getSharedInterface()
	{
		return *sharedObject.get();
	}

	void PluginState::setBounds(const IOConfig& newSettings)
	{
		while (protectedMemory.size() < 2)
			protectedMemory.emplace_back().setProtect(CMemoryGuard::protection::readwrite);

		if (!protectedMemory[0].resize<float>(newSettings.inputs * newSettings.blockSize) || !protectedMemory[1].resize<float>(newSettings.outputs * newSettings.blockSize))
			CPL_SYSTEM_EXCEPTION("Error allocating virtual protected memory for buffers");

		pluginInputs.resize(newSettings.inputs);
		pluginOutputs.resize(newSettings.outputs);

		if (newSettings == config)
			return;

		config = newSettings;

		Event e;
		e.eventType = IOChanged;
		Events::IOChanged aevent{ config.inputs, config.outputs, config.blockSize, config.sampleRate };
		e.event.eIOChanged = &aevent;

		dispatchEvent("ioChanged() event", e);
	}

	Status PluginState::dispatchEvent(const char * reason, APE_Event& event)
	{
		ScopedRefCount ref(*this);
		if (!ref.valid())
			return STATUS_DISABLED;

		auto ret = WrapPluginCall(reason,
			[&]
			{
				return generator.onEvent(*project, &event);
			}
		);

		if (ret.second)
		{
			internalDisable(std::move(ref), ret.first);
		}

		return ret.first;
	}

	template<typename Function>
	std::pair<Status, bool> PluginState::WrapPluginCall(const char * reason, Function&& f)
	{
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
			engine.getController().console().printLine(
				CColours::red,
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
						engine.getController().console().printLine(CColours::red, "[PluginState] : Plugin accessed memory out of bounds. "
							"Consider saving your project and restarting your application.");
					}
					else
					{
						const char * errString = "Your plugin has performed an illegal operation and has crashed! "
							"it is adviced that you immediately save your project (perhaps in a new file) and restart your "
							"application, as memory might have been corrupted. Consider going through "
							"your code and double-check it for errors, especially pointer dereferences "
							"and loops that might cause segmentation faults.";

						auto errorNotification = [&]()
						{
							cpl::Misc::MsgBox(errString, cpl::programInfo.name + " Fatal Error",
								cpl::Misc::MsgStyle::sOk | cpl::Misc::MsgIcon::iStop, engine.getController().getSystemWindow(), false);
						};

						if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
							errorNotification();
						else
							cpl::GUIUtils::MainEvent(*this, errorNotification);
					}
				}
			}
		}
		catch (const AbortException& exp)
		{
			engine.getController().console().printLine(
				CColours::orange,
				"[PluginState] : Plugin aborted at operation: %s: \"%s\". Plugin disabled.",
				reason,
				exp.what()
			);
		}

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
				threadData.fpuMask = _MM_GET_EXCEPTION_MASK();
				
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
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

	file:PluginState.h
	
		Implements interface for the c-subsystem-controller and communication between
		them; handles compiling and run-time resolution of dispatches from C to C++ 
		member functions, as well as wrapping unsafe code turning hardware exceptions
		into the software exception CSystemException. Allows setting of FPU environment.

*************************************************************************************/

#ifndef APE_CSTATE_H
	#define APE_CSTATE_H

	#include <cpl/PlatformSpecific.h>
	#include <cpl/MacroConstants.h>
	#include "CMemoryGuard.h"
	#include <vector>
	#include <exception>
	#include "CAllocator.h"
	#include <ape/Project.h>
	#include <ape/Events.h>
	#include <thread>
	#include "CControlManager.h"
	#include <cpl/Protected.h>
	#include <atomic>
	#include <cpl/gui/Tools.h>
	#include "Settings.h"
	#include "Engine/ParameterManager.h"
	#include <memory>

	namespace ape
	{

		class Engine;
		struct Module;
		class PluginState;
		class CCodeGenerator;
		struct SharedInterfaceEx;
		struct ProjectEx;
		class PluginCommandQueue;
		class PluginParameter;
		class PluginSurface;
		class PluginWidget;

		class PluginState final
			: private CBaseControl::CListener
			, private cpl::DestructionNotifier
			, private ParameterSet::RTListener
		{
		public:

			friend class PluginSurface;

#define DEFINE_EXCEPTION(X) class X : public std::runtime_error { using std::runtime_error::runtime_error; }

			DEFINE_EXCEPTION(AbortException);
			DEFINE_EXCEPTION(CompileException);
			DEFINE_EXCEPTION(InitException);
			DEFINE_EXCEPTION(CreateException);
			DEFINE_EXCEPTION(DisabledException);
			DEFINE_EXCEPTION(InvalidStateException);

#undef DEFINE_EXCEPTION

			SharedInterfaceEx& getSharedInterface();
			CAllocator& getPluginAllocator() noexcept { return pluginAllocator; }
			std::vector<CMemoryGuard> & getPMemory() noexcept { return protectedMemory; }

			void setBounds(const IOConfig& o);

			PluginState(Engine& effect, CCodeGenerator& codeGenerator, std::unique_ptr<ProjectEx> project);
			~PluginState();

			static void useFPUExceptions(bool b);

			Status activateProject();
			Status disableProject();
			Status processReplacing(const float * const * in, float * const * out, std::size_t sampleFrames, std::size_t * profiledCycles = nullptr) noexcept;
			bool isProcessing() const noexcept { return processing.load(std::memory_order_acquire); }
			void setPlayState(bool isPlaying);

			Status getState() const noexcept { return state; }
			CPluginCtrlManager& getCtrlManager() noexcept { return ctrlManager; }
			PluginCommandQueue* getCommandQueue() noexcept { return commandQueue.get(); }
			std::shared_ptr<PluginSurface> getOrCreateSurface();

		private:

			struct ScopedRefCount
			{
				ScopedRefCount(PluginState& parent)
					: parent(parent), good(false)
				{
					cpl::CFastMutex expiredLock(parent.expiredMutex);
					if (!parent.expired)
					{
						parent.useCount.fetch_add(1, std::memory_order_acquire);
						good = true;
					}
				}

				ScopedRefCount(ScopedRefCount&& other)
					: parent(other.parent), good(other.good)
				{
					other.good = false;
				}

				struct ScopedDisable
				{
					ScopedDisable(ScopedRefCount ref)
					{
					}
				};

				bool valid() const noexcept { return good; }

				~ScopedRefCount()
				{
					if (good)
						parent.useCount.fetch_sub(1, std::memory_order_release);
				}

				bool good;
				PluginState& parent;
			};

			template<typename Function>
			std::pair<Status, bool> WrapPluginCall(const char * reason, Function&& f);

			void performDisable();
			void waitDisable();

			void internalDisable(ScopedRefCount::ScopedDisable disable, Status errorCode);
			bool valueChanged(CBaseControl *) override;
			void parameterChangedRT(cpl::Parameters::Handle localHandle, cpl::Parameters::Handle globalHandle, ParameterSet::BaseParameter * param) override;

			void dispatchPlayEvent();
			Status dispatchEvent(const char * reason, APE_Event& event);
			void consumeCommands();
			void cleanupResources();

			std::unique_ptr<SharedInterfaceEx> sharedObject;
			std::unique_ptr<PluginCommandQueue> commandQueue;
			std::vector<std::unique_ptr<PluginParameter>> parameters;
			std::vector<std::unique_ptr<PluginWidget>> widgets;
			CCodeGenerator& generator;
			Engine& engine;
			CAllocator pluginAllocator;
			std::vector<CMemoryGuard> protectedMemory;
			std::vector<float*> pluginInputs, pluginOutputs;
			std::unique_ptr<ProjectEx> project;
			std::weak_ptr<PluginSurface> surface;

			CPluginCtrlManager ctrlManager;

			bool playing;
			IOConfig config;
			cpl::CMutex::Lockable expiredMutex;
			bool expired;
			std::atomic<int> useCount;
			std::atomic<Status> state;
			std::atomic<bool>
				abnormalBehaviour,
				pendingDisable,
				processing;
			

};
	};
#endif
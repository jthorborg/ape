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
	#include <cpl/Protected.h>
	#include <atomic>
	#include <cpl/gui/Tools.h>
	#include "Settings.h"
	#include "Engine/ParameterManager.h"
	#include <memory>
	#include <string>
	#include <map>

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
		class PluginAudioFile;
		class PluginStreamProducer;

		class PluginState final
			: private ParameterSet::RTListener
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

			PluginState(Engine& effect, CCodeGenerator& codeGenerator, std::unique_ptr<ProjectEx> project);
			~PluginState();

			SharedInterfaceEx& getSharedInterface();
			CAllocator& getPluginAllocator() noexcept { return pluginAllocator; }
			auto& getPluginAudioFiles() { return audioFiles; }
			auto& getPluginFFTs() { return ffts; }
			auto& getOriginalFiles() noexcept { return originalSampleRateFiles; }
			auto& getPMemory() noexcept { return protectedMemory; }
			auto& getOutputFiles() noexcept { return outputFiles; }
			const ProjectEx& getProject() const noexcept { return *project; }

			void setConfig(const IOConfig& o);
			const IOConfig& getConfig() const noexcept { return config; }

			static void useFPUExceptions(bool b);

			bool initializeActivation();
			bool finalizeActivation();
			bool disableProject();
			bool processReplacing(const float * const * in, float * const * out, std::size_t sampleFrames, std::size_t * profiledCycles = nullptr) noexcept;
			bool isProcessing() const noexcept { return processing.load(std::memory_order_acquire); }
			bool isDisabling() const noexcept { return currentlyDisabling.load(std::memory_order_acquire); }
			bool isEnabled() const noexcept { return enabled; }
			void setPlayState(bool isPlaying);
			bool getPlayState() const noexcept { return playing; }
			void apiTriggerOverride() noexcept { triggerSetThroughAPI = true; }

			void syncParametersToEngine(bool takeEngineValues);

			PluginCommandQueue* getCommandQueue() noexcept { return commandQueue.get(); }
			std::shared_ptr<PluginSurface> getOrCreateSurface();

		private:

			enum InvocationSemantics { DisregardInvocationIfErrorState, AlwaysPerformInvocation};

			template<typename Function>
			std::pair<Status, bool> WrapPluginCall(const char * reason, InvocationSemantics semantics, Function&& f);

			template<typename Function>
			std::pair<Status, bool> WrapPluginCall(const char * reason, Function&& f);

			void parameterChangedRT(cpl::Parameters::Handle localHandle, cpl::Parameters::Handle globalHandle, ParameterSet::BaseParameter * param) override;

			Status dispatchEvent(const char * reason, APE_Event& event);
			void consumeCommands();
			void cleanupResources();

			std::unique_ptr<SharedInterfaceEx> sharedObject;
			std::unique_ptr<PluginCommandQueue> commandQueue;
			std::vector<std::unique_ptr<PluginParameter>> parameters;
			std::vector<std::unique_ptr<PluginWidget>> widgets;
			std::vector<std::unique_ptr<PluginAudioFile>> audioFiles;
			std::vector<std::unique_ptr<APE_FFT>> ffts;
			std::vector<std::unique_ptr<PluginStreamProducer>> outputFiles;
			std::map<std::string, PluginAudioFile*> originalSampleRateFiles;

			CCodeGenerator& generator;
			Engine& engine;
			CAllocator pluginAllocator;
			std::vector<CMemoryGuard> protectedMemory;
			std::vector<float*> pluginInputs, pluginOutputs;
			std::unique_ptr<ProjectEx> project;
			std::weak_ptr<PluginSurface> surface;

			bool
				playing,
				triggerSetThroughAPI;

			IOConfig config;
			std::atomic<Status> state;
			std::atomic<bool>
				abnormalBehaviour,
				currentlyDisabling,
				processing,
				enabled,
				activating;
			

};
	};
#endif

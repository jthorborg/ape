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

	file:ParameterManager.h
		
		Manages parameters for the engine

*************************************************************************************/

#ifndef APE_PARAMETERMANAGER_H
	#define APE_PARAMETERMANAGER_H

	#include <cpl/infrastructure/parameters/ParameterSystem.h>
	#include <cpl/infrastructure/values/Values.h>
	#include <ape/APE.h>

	namespace ape 
	{
		/*
			Quick write-up of what's going on here, because there are several closely related concepts, with the excuse being that 
			dynamic parameters is not something that's easy to implement.

			The LowLevelParameter always exists and is the ground truth value of the parameter. The host updates this one through 
			automation. For UI-related tasks (like parameter printing, scaling) it defers this back to the main parameter manager
			that relies on hot-swappable parameter traits, so we can change formatting and scaling of parameters on the fly,
			as we swap and recompile plugins.

			The GUI library used relies on ValueEntities, abstractions of value (parameter) sources. We use ParameterValueWrappers to provide
			the ValueEntity interfaces so that knobs can automate parameters, but they really just go through the LowLevelParameter (wrap)
			and back into the traits through the ParameterManager.

			Any deferring through parameters into the ParameterManager happens through a zero-based index, a "parameter handle". This 
			is because we need a general way of referring uniquely to a parameter through different APIs and languages.

			The traits exists as another layer of indirection for implementing normal transformers/formatters based on the low-level
			C API.

			The concurrency issues are as follows:
			set/get parameter can and will be called on any thread. This means the set/get parameter path must be threadsafe, and 
			always just go through "static" codepaths. The ParameterGroup structure from cpl provides means for pushing these changes
			back to the UI thread safely, and to listen to these changes in realtime through RTListeners. We use the RTListener for
			writing fresh parameter values into the plugin local memory. 
			
			*anything* else is done on the main thread, and thus is safe to change on the main thread. Any API exposed by the parameter manager
			should only be interacted with on the main thread.
		*/

		class Engine;
		class PluginState;

		static const constexpr std::size_t NumParameters = 50;

		/// <summary>
		/// Floating point type used for audio
		/// </summary>
		typedef float AFloat;
		/// <summary>
		/// Floating point type used for parameters of the host system
		/// </summary>
		typedef float HostFloat;

		/// <summary>
		/// Floating point type used in the UI system
		/// </summary>
		typedef cpl::ValueT UIFloat;

		class LowLevelParameter 
			: private cpl::Utility::COnlyPubliclyMovable
			, private cpl::VirtualFormatter<UIFloat>
			, private cpl::VirtualTransformer<UIFloat>
		{
		public:

			typedef UIFloat ValueType;

			typedef cpl::VirtualTransformer<ValueType> Transformer;
			typedef cpl::VirtualFormatter<ValueType> Formatter;

			class Callbacks
			{
			public:
				virtual bool format(int ID, const ValueType & val, std::string & buf) = 0;
				virtual bool interpret(int ID, const cpl::string_ref buf, ValueType & val) = 0;
				virtual ValueType transform(int ID, ValueType val) const noexcept = 0;
				virtual ValueType normalize(int ID, ValueType val) const noexcept = 0;
				virtual const std::string& getName(int ID) const noexcept = 0;

				virtual ~Callbacks() {}
			};

			LowLevelParameter(int identifier, Callbacks& callbacks)
				: identifier(identifier), callbacks(callbacks), value(0)
			{
				
			}

			LowLevelParameter(LowLevelParameter&& other)
				: identifier(other.identifier), callbacks(other.callbacks), value(other.value.load(std::memory_order_acquire))
			{

			}

			PFloat getValue() const { return value.load(std::memory_order_acquire); }
			void setValue(PFloat newValue) { value.store(std::clamp<PFloat>(newValue, 0, 1), std::memory_order_release); }

			const std::string& getName() { return callbacks.getName(identifier); }
			Transformer& getTransformer() { return *this; }
			const Transformer& getTransformer() const { return *this; }
			Formatter& getFormatter() { return *this; }
			const Formatter& getFormatter() const { return *this; }

		protected:

			virtual bool format(const ValueType& val, std::string& buf) override { return callbacks.format(identifier, val, buf); }
			virtual bool interpret(const cpl::string_ref buf, ValueType& val) override { return callbacks.interpret(identifier, buf, val); }
			virtual ValueType transform(ValueType val) const noexcept override { return callbacks.transform(identifier, val); }
			virtual ValueType normalize(ValueType val) const noexcept override { return callbacks.normalize(identifier, val); }

		private:

			int identifier;
			Callbacks& callbacks;
			std::atomic<PFloat> value;

		};

		typedef cpl::ParameterGroup<PFloat, HostFloat, LowLevelParameter> ParameterSet;

		class ParameterManager
			: private ParameterSet::AutomatedProcessor
			, private LowLevelParameter::Callbacks
		{
		public:

			friend class PluginState;

			typedef cpl::Parameters::Handle IndexHandle;

			class ExternalParameterTraits
				: public cpl::VirtualFormatter<UIFloat>
				, public cpl::VirtualTransformer<UIFloat>
			{
			public:
				virtual const std::string& getName() = 0;
			};


			ParameterManager(Engine& engine, std::size_t numParameters);

			std::size_t numParams() const noexcept;
			void setParameter(IndexHandle index, HostFloat normalizedValue);
			HostFloat getParameter(IndexHandle index);
			std::string getParameterName(IndexHandle index);
			std::string getParameterText(IndexHandle index);
			cpl::ValueEntityBase& getValueFor(IndexHandle index);
			void emplaceTrait(IndexHandle index, ExternalParameterTraits& trait);
			void clearTrait(IndexHandle index);
			void pulse();

		protected:

			typedef cpl::ParameterValueWrapper<ParameterSet::ParameterView> ParameterValueWrapper;

			// Inherited via AutomatedProcessor
			void automatedTransmitChangeMessage(int parameter, ParameterSet::FrameworkType value) override;
			void automatedBeginChangeGesture(int parameter) override;
			void automatedEndChangeGesture(int parameter) override;

			// Inherited via Parameter::Callbacks
			bool format(int ID, const LowLevelParameter::ValueType & val, std::string & buf) override;
			bool interpret(int ID, const cpl::string_ref buf, LowLevelParameter::ValueType & val) override;
			LowLevelParameter::ValueType transform(int ID, LowLevelParameter::ValueType val) const noexcept override;
			LowLevelParameter::ValueType normalize(int ID, LowLevelParameter::ValueType val) const noexcept override;
			const std::string& getName(int ID) const noexcept override;

			ParameterSet& getParameterSet() noexcept { return parameterSet; }

		private:

			Engine& engine;
			const std::size_t numParameters;
			std::vector<LowLevelParameter> parameters;
			std::vector<ExternalParameterTraits*> traits;
			std::vector<ParameterValueWrapper> valueWrappers;

			ParameterSet parameterSet;

			cpl::LinearRange<LowLevelParameter::ValueType> defaultRange;
			cpl::BasicFormatter<LowLevelParameter::ValueType> defaultFormatter;
			std::string unnamed;
		};
	}
#endif
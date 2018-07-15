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

	namespace ape 
	{
		class Engine;

		static const constexpr std::size_t NumParameters = 50;

		/// <summary>
		/// Floating-point type used for parameters etc.
		/// </summary>
		typedef double SFloat;
		/// <summary>
		/// Floating point type used for audio
		/// </summary>
		typedef float AFloat;
		/// <summary>
		/// Floating point type used for parameters of the host system
		/// </summary>
		typedef float PFloat;


		class Parameter 
			: private cpl::Utility::COnlyPubliclyMovable
			, private cpl::VirtualFormatter<SFloat>
			, private cpl::VirtualTransformer<SFloat>
		{
		public:

			typedef cpl::VirtualTransformer<SFloat> Transformer;
			typedef cpl::VirtualFormatter<SFloat> Formatter;
			typedef SFloat ValueType;

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

			Parameter(int identifier, Callbacks& callbacks)
				: identifier(identifier), callbacks(callbacks), value(0)
			{
				
			}

			Parameter(Parameter&& other)
				: identifier(other.identifier), callbacks(other.callbacks), value(other.value.load(std::memory_order_acquire))
			{

			}

			ValueType getValue() const { return value.load(std::memory_order_acquire); }
			void setValue(ValueType newValue) { value.store(newValue, std::memory_order_release); }

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
			std::atomic<ValueType> value;

		};

		typedef cpl::ParameterGroup<SFloat, PFloat, Parameter> ParameterSet;
		typedef cpl::ParameterValue<ParameterSet::ParameterView> ParameterValue;

		class ParameterManager
			: private ParameterSet::AutomatedProcessor
			, private Parameter::Callbacks
		{
		public:

			typedef cpl::Parameters::Handle IndexHandle;

			ParameterManager(Engine& engine, std::size_t numParameters);

			std::size_t numParams() const noexcept;
			void setParameter(IndexHandle index, SFloat normalizedValue);
			SFloat getParameter(IndexHandle index);
			std::string getParameterName(IndexHandle index);
			std::string getParameterText(IndexHandle index);

		protected:

			// Inherited via AutomatedProcessor
			void automatedTransmitChangeMessage(int parameter, ParameterSet::FrameworkType value) override;
			void automatedBeginChangeGesture(int parameter) override;
			void automatedEndChangeGesture(int parameter) override;

			// Inherited via Parameter::Callbacks
			bool format(int ID, const Parameter::ValueType & val, std::string & buf) override;
			bool interpret(int ID, const cpl::string_ref buf, Parameter::ValueType & val) override;
			Parameter::ValueType transform(int ID, Parameter::ValueType val) const noexcept override;
			Parameter::ValueType normalize(int ID, Parameter::ValueType val) const noexcept override;
			const std::string& getName(int ID) const noexcept override;

		private:

			Engine& engine;
			const std::size_t numParameters;
			std::vector<Parameter> parameters;
			ParameterSet parameterSet;
			cpl::LinearRange<Parameter::ValueType> defaultRange;
			cpl::BasicFormatter<Parameter::ValueType> defaultFormatter;
			std::string blah = "blah";
		};
	}
#endif
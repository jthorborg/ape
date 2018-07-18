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

	file:PluginCommandQueue.h
		
		Represents the commands given by the plugin during construction

*************************************************************************************/

#ifndef APE_PLUGINCOMMANDQUEUE_H
	#define APE_PLUGINCOMMANDQUEUE_H

	#include <vector>
	#include <string>
	#include <memory>
	#include <ape/APE.h>

	namespace ape 
	{


		enum class CommandType
		{
			Parameter
		};

		class CommandBase
		{
		public:

			CommandBase(CommandType type) : command(type) {}
			virtual ~CommandBase() {}

			CommandType getCommandType() const noexcept { return command; }
			
		private:
			CommandType command;
		};

		class ParameterRecord : public CommandBase
		{
		public:
			using CommandBase::CommandBase;

			/* static ParameterRecord BoolFlag(const char* name, float* val)
			{
				ParameterRecord ret(CommandType::Parameter);
				ret.name = name;
				ret.value = val;
				ret.type = ParameterType::Boolean;
				return ret;
			}

			static ParameterRecord LegacyKnob(const char* name, float* val, int type)
			{
				ParameterRecord ret(CommandType::Parameter);
				ret.name = name;
				ret.value = val;
				ret.type = ParameterType::TypedKnob;
				ret.knobType = type;
				return ret;
			}

			static ParameterRecord ScaledParameter(const char* name, const char* unit, float* val, ScaleFunc func, float min, float max)
			{
				ParameterRecord ret(CommandType::Parameter);
				ret.name = name;
				ret.value = val;
				ret.type = ParameterType::ScaledFloat;
				ret.min = min;
				ret.max = max;
				ret.scaler = func;
				ret.unit = unit;
				return ret;
			}

			static ParameterRecord ValueList(const char* name, const char* unit, float* val, const char * values)
			{
				ParameterRecord ret(CommandType::Parameter);
				ret.name = name;
				ret.value = val;
				ret.type = ParameterType::List;
				ret.unit = unit;
				ret.values = values;
				return ret;
			} */

			static ParameterRecord NormalParameter(const char* name, const char* unit, PFloat* val, Transformer transformer, Normalizer normalizer, PFloat min, PFloat max)
			{
				ParameterRecord ret(CommandType::Parameter);
				ret.name = name;
				ret.value = val;
				ret.type = ParameterType::ScaledFloat;
				ret.min = min;
				ret.max = max;
				ret.transformer = transformer ? transformer : cpl::Math::UnityScale::linear<PFloat>;
				ret.normalizer = normalizer ? normalizer : cpl::Math::UnityScale::Inv::linear<PFloat>;
				ret.unit = unit;
				return ret;
			}

			enum ParameterType
			{
				/// <summary>
				/// Choice of two things
				/// Trigger button
				/// </summary>
				Boolean,
				/// <summary>
				/// 0 - 1 float, check knob type
				/// </summary>
				TypedKnob,
				/// <summary>
				/// A scaled value in the range of ]min, max], see ScaleFunc
				/// </summary>
				ScaledFloat,
				/// <summary>
				/// A list of values, see values
				/// </summary>
				List
			};

			ParameterType type;
			std::string name;
			std::string unit;
			std::string values;
			PFloat* value = nullptr;
			PFloat min = 0;
			PFloat max = 1;

			int knobType = 0;
			Transformer transformer = nullptr;
			Normalizer normalizer = nullptr;
		};

		class PluginCommandQueue
		{
		public:

			typedef std::vector<std::unique_ptr<CommandBase>> CommandQueue;

			template<class Command>
			typename std::enable_if<std::is_base_of<CommandBase, Command>::value>::type enqueueCommand(Command&& command)
			{
				commands.emplace_back(std::make_unique<Command>(std::move(command)));
			}

			void enqueueCommand(std::unique_ptr<CommandBase> command)
			{
				commands.emplace_back(std::move(command));
			}

			const CommandQueue& getCommands() const noexcept { return commands; }

			const CommandBase& operator[] (std::size_t index) const noexcept { return *commands[index]; }
			std::size_t size() const noexcept { return commands.size(); }

		private:
			CommandQueue commands;
		};

	}
#endif
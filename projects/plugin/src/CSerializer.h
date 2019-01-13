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

	file:CSerializer.h
		Provides static methods for serializing objects of type 'Engine' into a memoryblock.

*************************************************************************************/

#ifndef _CSERIALIZER_H
	#define _CSERIALIZER_H


	#include <cpl/MacroConstants.h>
	#include "Engine.h"
	#include "UIController.h"
	#include "CApi.h"
	#include "Common.h"
	#include "CConsole.h"
	#include "CodeEditor/SourceManager.h"
	#include <cpl/state/Serialization.h>
	#include "PluginState.h"

	namespace ape
	{
		static const cpl::Version CurrentVersion(0, 1, 1);

		/*
			It is of pretty high importance to standardize the layout of this 
			struct os-, architechture- and compiler independent (for obvious reasons)
		*/
		struct SerializedEngine
		{
			typedef uint32_t SeIntType;

			struct ControlValue
			{
				float value;
				SeIntType tag;

			};

			SeIntType
				structSize, // sizeof this struct
				version, // the version that created this struct ( typically _VERSION_INT)
				size, // the total size of this memory block
				fileNameOffset, // the offset from which the filename is stored as a char
				numValues, // the number of ControlValue values that exists in this block
				valueOffset, // the offset from which the array of ControlValues resides in this block
				editorOpened, // whether the editor was opened
				hasAProject, // whether the state has a project
				isActivated; // whether the state was activated


			const char * getFileNameConst() const
			{
				if (fileNameOffset > size)
					return nullptr;
				return reinterpret_cast<const char*>(reinterpret_cast<const char*>(this) + fileNameOffset);
			}
			const ControlValue * getValuesConst() const
			{
				if (valueOffset > size)
					return nullptr;
				return reinterpret_cast<const ControlValue*>(reinterpret_cast<const char*>(this) + valueOffset);
			}
			char * getFileName()
			{
				if (fileNameOffset > size)
					return nullptr;
				return reinterpret_cast<char*>(reinterpret_cast<char*>(this) + fileNameOffset);
			}
			ControlValue * getValues()
			{
				if (valueOffset > size)
					return nullptr;
				return reinterpret_cast<ControlValue*>(reinterpret_cast<char*>(this) + valueOffset);
			}
		};

		class CSerializer
		{
		public:

			static bool serialize(Engine& engine, juce::MemoryBlock& destination)
			{
				cpl::CCheckedSerializer serializer("Audio Programming Environment");
				auto& archive = serializer.getArchiver();
				archive.setMasterVersion(CurrentVersion);

				archive << engine.isProcessingAPlugin();

				archive["scope-data"]["state"].setMasterVersion({ SIGNALIZER_MAJOR, SIGNALIZER_MINOR, SIGNALIZER_BUILD });
				archive["scope-data"]["state"] << engine.getOscilloscopeData().getContent();
				archive["controller"] << engine.getController();
				archive["session-name"] << engine.getController().getProjectName();

				archive["params"] << engine.getParameterManager().getParameterSet();

				auto content = serializer.compile(true);

				destination.append(content.getBlock(), content.getSize());

				return true;
			}

			static bool restoreNewVersion(Engine& engine, const void* block, unsigned size)
			{
				cpl::CCheckedSerializer serializer("Audio Programming Environment");
				if (!serializer.build({ block, size }))
					return false;

				auto builder = serializer.getBuilder();

				if (serializer.getBuilder().getLocalVersion() < CurrentVersion)
					return false;

				bool isActivated;
				builder >> isActivated;

				std::string sessionName;
				auto& controller = engine.getController();

				builder["session-name"] >> sessionName;
				builder["controller"] >> controller;

				if (builder.findForKey("scope-data"))
				{
					auto& scope = builder["scope-data"];
					scope["state"] >> engine.getOscilloscopeData().getContent();
				}

				if (!isActivated)
					return true;

				// try to compile the project
				auto plugin = controller.createPlugin(false).get();

				// check if success
				if (!plugin)
				{
					controller.getConsole().printLine(CConsole::Error,
						"[Serializer] : Error compiling session file (%s)!", sessionName.c_str());
					return false;
				}

				controller.setPlugin(std::move(plugin));

				// project is now compiled, lets try to activate it
				if (!controller.performCommand(UICommand::Activate))
				{
					controller.getConsole().printLine(CConsole::Error,
						"[Serializer] : Error activating project (%s)!", sessionName.c_str());
					return false;

				}

				if (auto* list = builder.findForKey("params"))
				{
					auto& params = builder["params"];
					params >> engine.getParameterManager().getParameterSet();

				}
				else if(builder.findForKey("parameters"))
				{
					auto& list = builder["parameters"];
					auto counts = builder.findForKey("parameter-count");
					std::size_t parameters = 0;
					if (builder.findForKey("parameter-count"))
						builder["parameter-count"] >> parameters;

					auto& manager = engine.getParameterManager();

					for (std::size_t i = 0; i < parameters; ++i)
					{
						SerializedEngine::ControlValue value;
						list[i] >> value;

						manager.setParameter(i, value.value);
					}
				}

				auto& commands = controller.getUICommandState();
				commands.changeValueExternally(commands.activationState, 1.0);

				return true;

			}

			static bool restore(Engine& engine, const void * block, unsigned size)
			{
				using namespace cpl;
				const SerializedEngine * se = reinterpret_cast<const SerializedEngine *> (block);
				auto& controller = engine.getController();
				auto& console = controller.getConsole();
				// some basic checks
				if (
					!se // nullpointer check
					|| size < sizeof(SerializedEngine) // whether the actual size is smaller than the 
					|| se->size != size // whether the size actual size matches reported size
					)
				{
					if (!restoreNewVersion(engine, block, size))
					{
						console.printLine(CConsole::Error,
							"[Serializer] : Invalid memory block recieved from host (%d, %d, %d, %d)!",
							se, size, sizeof(SerializedEngine), se->size);
						return false;
					}
					return true;
				}
				else if (se->version != 9)
				{
					return restoreNewVersion(engine, block, size);
				}

				if (controller.getSourceManager().checkAutoSave())
				{
					console.printLine(CConsole::Error,
						"[Serializer] : Autosave was restored, reopen the project to perform normal serialization.");
					return false;
				}
				// then, we set it to the file from last session
				if(!controller.getSourceManager().openFile(se->getFileNameConst()))
				{
					console.printLine(CConsole::Error,
						"[Serializer] : Error opening session file (%s)!", se->getFileNameConst());
					return false;
				}
				// check if the project was running:
				if (se->isActivated)
				{
					// try to compile the project
					auto plugin = controller.createPlugin(false).get();

					// check if success
					if(!plugin)
					{
						console.printLine(CConsole::Error,
							"[Serializer] : Error compiling session file (%s)!", se->getFileNameConst());
						return false;
					}

					controller.setPlugin(std::move(plugin));

					// project is now compiled, lets try to activate it
					if (!controller.performCommand(UICommand::Activate))
					{
						console.printLine(CConsole::Error,
							"[Serializer] : Error activating project (%s)!", controller.getProjectName().c_str());
						return false;

					}
					// project is up and running! now we just need to reset parameters
					// get values
					const SerializedEngine::ControlValue * values = se->getValuesConst();

					auto& manager = engine.getParameterManager();

					for (std::size_t i = 0; i < se->numValues; ++i)
					{
						manager.setParameter(i, values[i].value);
					}

					auto& commands = controller.getUICommandState();
					commands.changeValueExternally(commands.activationState, 1.0);
				}

				return true;

			}




		};
	};
#endif
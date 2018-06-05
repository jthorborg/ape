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
	#include <cpl/CThread.h>
	#include "CodeEditor/CCodeEditor.h"
	#include <cpl/state/Serialization.h>

	namespace ape
	{
		static const cpl::Version CurrentVersion(0, 1, 0);

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

			static bool serialize(Engine * engine, juce::MemoryBlock & destination)
			{
				cpl::CCheckedSerializer serializer("Audio Programming Environment");
				auto& archive = serializer.getArchiver();
				archive.setMasterVersion(CurrentVersion);

				std::string fileName = engine->getController().externEditor->getDocumentPath();
				bool hasAProject = fileName.size() > 2 ? true : false;
				Status state = engine->getCurrentPluginState() ? engine->getCurrentPluginState()->getState() : STATUS_DISABLED;

				// we basically quantize all engine states to running or not running
				// anything in between is error states or intermediate states.
				bool isActivated(false);

				switch (state)
				{
				case Status::STATUS_ERROR:
				case Status::STATUS_DISABLED:
					isActivated = false;
					break;
				case Status::STATUS_OK:
				case Status::STATUS_READY:
					isActivated = true;
					break;
				default:
					isActivated = false;
				}

				archive["fileName"] << engine->getController().externEditor->getDocumentPath();
				//archive["scope"] << engine->getOscilloscopeData().getContent();

				archive << hasAProject;
				archive << isActivated;
				archive << engine->getController().externEditor->isOpen();

				archive["scope-data"]["state"] << engine->getOscilloscopeData().getContent();

				if (isActivated)
				{
					// TODO: Remove, turn into actual audio parameters
					auto& list = archive["parameters"];
					std::size_t count = engine->getCurrentPluginState()->getCtrlManager().getControls().size();
					std::size_t i = 0;
					archive["parameter-count"] << count;
					for (auto ctrl : engine->getCurrentPluginState()->getCtrlManager().getControls())
					{
						list[i++] << SerializedEngine::ControlValue { ctrl->bGetValue(), (SerializedEngine::SeIntType) ctrl->bGetTag() };
					}
				}

				auto content = serializer.compile(true);

				destination.append(content.getBlock(), content.getSize());

				return true;
			}

			static bool restoreNewVersion(Engine* engine, const void* block, unsigned size)
			{
				cpl::CCheckedSerializer serializer("Audio Programming Environment");
				if (!serializer.build({ block, size }))
					return false;

				auto builder = serializer.getBuilder();

				std::string filePath;

				bool hasProject, isActivated, isOpen;

				builder["fileName"] >> filePath;
				builder >> hasProject >> isActivated >> isOpen;
				//builder["scope"] >> engine->getOscilloscopeData().getContent();

				if (builder.findForKey("scope-data"))
				{
					auto& scope = builder["scope-data"];
					scope["state"] >> engine->getOscilloscopeData().getContent();
				}


				// first we open the editor to ensure it's initialized
				if (!engine->getController().externEditor->openEditor(isOpen))
				{
					engine->getController().console().printLine(CColours::red,
						"[Serializer] : Error opening editor!");
					return false;
				}
				if (engine->getController().externEditor->checkAutoSave())
				{
					engine->getController().console().printLine(CColours::red,
						"[Serializer] : Autosave was restored, reopen the project to perform normal serialization.");
					return false;
				}
				// then, we set it to the file from last session
				if (!engine->getController().externEditor->openFile(filePath))
				{
					engine->getController().console().printLine(CColours::red,
						"[Serializer] : Error opening session file (%s)!", filePath.c_str());
					return false;
				}
				
				if (!isActivated)
					return true;

				// try to compile the project
				auto plugin = engine->getController().createPlugin().get();

				// check if success
				if (!plugin)
				{
					engine->getController().console().printLine(CColours::red,
						"[Serializer] : Error compiling session file (%s)!", filePath.c_str());
					return false;
				}

				engine->exchangePlugin(std::move(plugin));

				// project is now compiled, lets try to activate it
				if (!engine->activatePlugin())
				{
					engine->getController().console().printLine(CColours::red,
						"[Serializer] : Error activating project (%s)!", engine->getController().projectName.c_str());
					return false;

				}

				if (builder.findForKey("parameters"))
				{
					auto& list = builder["parameters"];
					auto counts = builder.findForKey("parameter-count");
					std::size_t parameters = 0;
					if (builder.findForKey("parameter-count"))
						builder["parameter-count"] >> parameters;

					CPluginCtrlManager & ctrlManager = engine->getCurrentPluginState()->getCtrlManager();

					for (std::size_t i = 0; i < parameters; ++i)
					{
						SerializedEngine::ControlValue value;
						list[i] >> value;

						auto control = ctrlManager.getControl(value.tag);

						if (control)
							control->bSetValue(value.value);
						else
						{
							engine->getController().console().printLine(CColours::red,
								"[Serializer] : Error restoring values to controls: No control found for tag %d!",
								value.value);
							return false;

						}
					}
				}

				return true;

			}

			static bool restore(Engine * engine, const void * block, unsigned size)
			{
				using namespace cpl;
				const SerializedEngine * se = reinterpret_cast<const SerializedEngine *> (block);
				// some basic checks
				if (
					!se // nullpointer check
					|| size < sizeof(SerializedEngine) // whether the actual size is smaller than the 
					|| se->size != size // whether the size actual size matches reported size
					)
				{
					if (!restoreNewVersion(engine, block, size))
					{
						engine->getController().console().printLine(CColours::red,
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
				// first we open the editor to ensure it's initialized
				if(!engine->getController().externEditor->openEditor(se->editorOpened))
				{
					engine->getController().console().printLine(CColours::red,
						"[Serializer] : Error opening editor!");
					return false;
				}
				if (engine->getController().externEditor->checkAutoSave())
				{
					engine->getController().console().printLine(CColours::red,
						"[Serializer] : Autosave was restored, reopen the project to perform normal serialization.");
					return false;
				}
				// then, we set it to the file from last session
				if(!engine->getController().externEditor->openFile(se->getFileNameConst()))
				{
					engine->getController().console().printLine(CColours::red,
						"[Serializer] : Error opening session file (%s)!", se->getFileNameConst());
					return false;
				}
				// check if the project was running:
				if (se->isActivated)
				{
					// try to compile the project
					auto plugin = engine->getController().createPlugin().get();

					// check if success
					if(!plugin)
					{
						engine->getController().console().printLine(CColours::red,
							"[Serializer] : Error compiling session file (%s)!", se->getFileNameConst());
						return false;
					}

					engine->exchangePlugin(std::move(plugin));

					// project is now compiled, lets try to activate it
					if (!engine->activatePlugin())
					{
						engine->getController().console().printLine(CColours::red,
							"[Serializer] : Error activating project (%s)!", engine->getController().projectName.c_str());
						return false;

					}
					// project is up and running! now we just need to reset parameters
					// get values
					const SerializedEngine::ControlValue * values = se->getValuesConst();
					// get control manager
					CPluginCtrlManager & ctrlManager = engine->getCurrentPluginState()->getCtrlManager();
				
					for (unsigned i = 0; i < se->numValues; i++)
					{
						// get instance control from tag
						auto control = ctrlManager.getControl(values[i].tag);
						// if it exists, set the value of it
						
						if (control)
							control->bSetValue(values[i].value);
						else
						{
							engine->getController().console().printLine(CColours::red,
								"[Serializer] : Error restoring values to controls: No control found for tag %d!",
								values[i].tag);
							return false;

						}
					}
					// all controls restored - now we update display, and were set!
					ctrlManager.callListeners();
				}

				return true;

			}




		};
	};
#endif
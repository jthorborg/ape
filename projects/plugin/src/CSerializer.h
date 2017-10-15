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


	#include "MacroConstants.h"
	#include "Engine.h"
	#include "GraphicUI.h"
	#include "CApi.h"
	#include "Common.h"
	#include "CConsole.h"
	#include "CThread.h"

	namespace APE
	{
		/*
			It is of pretty high importance to standardize the layout of this 
			struct os-, architechture- and compiler independent (for obvious reasons)
		*/
		struct __alignas(APE_DEF_ALIGN) SerializedEngine
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

				std::string fileName = engine->getGraphicUI()->externEditor->getDocumentPath();
				bool hasAProject = fileName.size() > 2 ? true : false;
				Status state = engine->state;

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

				auto & controls = engine->getGraphicUI()->ctrlManager.getControls();
				auto listSize = controls.size();

				// needed size in bytes
				std::size_t neededSize =
					sizeof(SerializedEngine) +
					listSize * sizeof(SerializedEngine::ControlValue) +
					fileName.size() + 1;

				SerializedEngine * se = reinterpret_cast<SerializedEngine*>(malloc(neededSize));
				::memset(se, 0, neededSize);
				se->size = neededSize;
				se->structSize = sizeof(SerializedEngine);
				se->version = _VERSION_INT;
				se->editorOpened = engine->gui->externEditor->isOpen();
				se->isActivated = isActivated;
				se->valueOffset = sizeof(SerializedEngine);
				se->fileNameOffset = se->valueOffset + listSize * sizeof(SerializedEngine::ControlValue);
				se->hasAProject = hasAProject;
				se->numValues = listSize;
				::memcpy(se->getFileName(), fileName.c_str(), fileName.size() + 1);
				SerializedEngine::ControlValue * values = se->getValues();
				int i = 0;
				for (auto ctrl : controls)
				{
					values[i].tag = ctrl->bGetTag();
					values[i].value = ctrl->bGetValue();

					i++;
				}
				destination.ensureSize(neededSize);
				destination.copyFrom(se, 0, neededSize);
				::free(se);
				return true;
			}

			static bool restore(Engine * engine, const void * block, unsigned size)
			{

				const SerializedEngine * se = reinterpret_cast<const SerializedEngine *> (block);
				// some basic checks
				if (
					!se // nullpointer check
					|| size < sizeof(SerializedEngine) // whether the actual size is smaller than the 
					|| se->size != size // whether the size actual size matches reported size
					)
				{
					engine->gui->console->printLine(CColours::red,
						"[Serializer] : Invalid memory block recieved from host (%d, %d, %d, %d)!", 
						se, size, sizeof(SerializedEngine), se->size);
					return false;
				}
				else if (se->version != _VERSION_INT)
				{
					engine->gui->console->printLine(CColours::red,
						"[Serializer] : Warning: Different versions detected!");

					auto answer = Misc::MsgBox
						(
						"Warning: You're trying to restore an instance from a different version ("
						_VERSION_INT_STRING ") of this plugin, continue?", 
						_PROGRAM_NAME " warning",
						Misc::MsgStyle::sYesNoCancel | Misc::MsgIcon::iWarning
						);
					if (answer != Misc::MsgButton::bYes)
						return false;
				}
				// first we open the editor to ensure it's initialized
				if(!engine->gui->externEditor->openEditor(se->editorOpened))
				{
					engine->gui->console->printLine(CColours::red,
						"[Serializer] : Error opening editor!");
					return false;
				}
				if (engine->gui->externEditor->checkAutoSave())
				{
					engine->gui->console->printLine(CColours::red,
						"[Serializer] : Autosave was restored, reopen the project to perform normal serialization.");
					return false;
				}
				// then, we set it to the file from last session
				if(!engine->gui->externEditor->openFile(se->getFileNameConst()))
				{
					engine->gui->console->printLine(CColours::red,
						"[Serializer] : Error opening session file (%s)!", se->getFileNameConst());
					return false;
				}
				// check if the project was running:
				if (se->isActivated)
				{
					// try to compile the project
					CThread compileThread(GraphicUI::startCompilation);
					compileThread.run(engine);
					// wait for it to finish
					compileThread.join();

					// check if success
					if (!engine->gui->bIsCompiled)
					{
						engine->gui->console->printLine(CColours::red,
							"[Serializer] : Error compiling session file (%s)!", se->getFileNameConst());
						return false;

					}
					// project is now compiled, lets try to activate it
					if (!engine->activatePlugin())
					{
						engine->gui->console->printLine(CColours::red,
							"[Serializer] : Error activating project (%s)!", engine->gui->projectName.c_str());
						return false;

					}
					// project is up and running! now we just need to reset parameters
					// get values
					const SerializedEngine::ControlValue * values = se->getValuesConst();
					// get control manager
					CPluginCtrlManager & ctrlManager = engine->gui->ctrlManager;
				
					for (unsigned i = 0; i < se->numValues; i++)
					{
						// get instance control from tag
						auto control = ctrlManager.getControl(values[i].tag);
						// if it exists, set the value of it
						
						if (control)
							control->bSetValue(values[i].value);
						else
						{
							engine->gui->console->printLine(CColours::red,
								"[Serializer] : Error restoring values to controls: No control found for tag %d!",
								values[i].tag);
							return false;

						}
					}
					// all controls restored - now we update display, and were set!
					engine->gui->ctrlManager.callListeners();
				}

				return true;

			}




		};
	};
#endif
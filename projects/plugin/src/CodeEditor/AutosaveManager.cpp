/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
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

	file:AutosaveManager.cpp
		
		Implementation of AutosaveManager.h

*************************************************************************************/


#include "AutosaveManager.h"
#include "../Settings.h"
#include "../CConsole.h"
#include "SourceManager.h"
#include <cpl/state/Serialization.h>
#include "../CSerializer.h"
#include <ctime>
#include <vector>
#include <cpl/stdext.h>

namespace ape
{
	const std::string nameReference = "ape-autosave";

	std::mutex AutosaveManager::directoryLock;

	AutosaveManager::AutosaveManager(int ID, Settings& settings, SourceManager& manager, UIController& controller)
		: instanceID(ID)
		, settings(settings)
		, manager(manager)
		, controller(controller)
		, checked(false)
		, wasRestored(false)
	{
		startTimer(1000 * settings.lookUpValue(60, "application", "autosave_interval"));
	}

	AutosaveManager::~AutosaveManager()
	{
		stopTimer();
		if (autosaveFile.isOpened())
			autosaveFile.remove();
	}

	void AutosaveManager::timerCallback()
	{
		if (manager.isDirty())
		{
			if(performAutosave())
				controller.setStatusText("Autosaved...", CColours::lightgoldenrodyellow, 2000);
		}
	}

	bool AutosaveManager::performAutosave()
	{
		try
		{
			auto state = std::make_unique<cpl::CCheckedSerializer>(nameReference);
			auto& ar = state->getArchiver();

			ar.setMasterVersion(CurrentVersion);
			ar["autosave"] << manager;
			ar["path"] << manager.getDocumentPath().string();

			autosaveState = std::async(
				[this](auto ptr)
				{
					return asyncSave(std::move(ptr));
				},
				std::move(state)
			);
		}
		catch (const std::exception& e)
		{
			controller.console().printLine(juce::Colours::red, "[Autosave] : Error: %s", e.what());
			return false;
		}

		return true;
	}

	bool AutosaveManager::asyncSave(std::unique_ptr<cpl::CCheckedSerializer> state)
	{
		auto& ar = state->getArchiver();

		ar["time-stamp"] << juce::Time::currentTimeMillis();

		if (!autosaveFile.isOpened())
		{
			std::string path = cpl::Misc::DirectoryPath() + "/logs/autosave" + std::to_string(instanceID) + ".ape";
			if (!autosaveFile.open(path))
				return false;

			auto wrapper = state->compile();

			if (!autosaveFile.write(wrapper.getBlock(), wrapper.getSize()))
				return false;

			autosaveFile.flush();

		}

		return true;
	}

	bool AutosaveManager::checkAutosave()
	{
		if (checked)
			return wasRestored;

		try
		{
			std::lock_guard<std::mutex> lock(directoryLock);

			juce::DirectoryIterator dir(juce::File(cpl::Misc::DirectoryPath() + "/logs/"), false, "autosave*.ape");

			// Iterate over all autosave files until user chooses to open one, or we deleted/moved all existing ones.
			while (dir.next())
			{
				juce::File f(dir.getFile());

				// check if file is opened in exclusive mode by another instance of this program
				// ie: whether it's an autosave file currently being used, or if it's an old file
				// that we can safely mess with / restore / delete
				if (cpl::CExclusiveFile::isFileExclusive(f.getFullPathName().toRawUTF8()))
					continue;

				juce::ScopedPointer<juce::FileInputStream> stream = f.createInputStream();

				// this really should fail if an autosave file in another instance is opened in none-shared mode
				// ie. this is a feature: we wont offer to open autosaves created by other active instances
				if (!stream.get())
					continue;

				std::vector<std::byte> bytes(stream->getTotalLength());

				if (auto bytesRead = stream->read(bytes.data(), static_cast<int>(bytes.size())); bytesRead != bytes.size())
				{
					controller.console().printLine(CColours::red,
						"[Autosave] : Error reading file contents of %s (read %d, expected %d bytes)",
						f.getFileName().toRawUTF8(),
						static_cast<int>(bytesRead),
						static_cast<int>(bytes.size())
					);

					continue;
				}

				stream = nullptr;

				if (wasRestored = potentiallyRestoreAutosave(bytes, f))
					break;
			}

			checked = true;
			return wasRestored;

		}
		catch (const std::exception& e)
		{
			controller.console().printLine(CColours::red, "[Autosave] : Exception while checking autosaves: %s", e.what());

			return false;
		}
		
	}

	bool AutosaveManager::potentiallyRestoreAutosave(const std::vector<std::byte>& src, juce::File & file)
	{
		using namespace cpl::Misc;
		
		std::stringstream fmt;
		cpl::CCheckedSerializer state(nameReference);

		if (!state.build({ src.data(), src.size() }))
			return false;

		auto& builder = state.getBuilder();

		std::string fullFileName;
		builder["path"] >> fullFileName;

		juce::int64 ms;
		builder["time-stamp"] >> ms;

		juce::Time time(ms);

		fs::path filePath = fullFileName;


		fmt << "Recoverable file found: " << fullFileName << std::endl;
		fmt << "File was last autosaved at " << time.getHours() << ":" << time.getMinutes() << ":"
			<< time.getSeconds() << ", " << time.getDayOfMonth() << "/" << time.getMonth() + 1 << "." << std::endl;

		if (fs::exists(filePath))
		{
			bool newer = juce::File(filePath.string()).getLastModificationTime() > time;
			fmt << "File saved on disk is " << (newer ? "newer" : "older") << "." << std::endl;
		}

		fmt << "Do you want to open this file (yes), delete it (no) or move it to /junk/ folder (cancel)?";

		auto answer = MsgBox(fmt.str(), "ape - Autorecover", MsgStyle::sYesNoCancel | MsgIcon::iQuestion, manager.getParentWindow(), true);

		std::string fileName = cpl::fs::path(fullFileName).filename().string();

		bool ret = false;

		switch (answer)
		{
		case MsgButton::bCancel:
		{
			// move file to a junk folder
			auto dirPath = DirectoryPath() + "/junk/";
			auto dir = juce::File(dirPath);
			if (!dir.exists())
				dir.createDirectory();

			file.moveFileTo(juce::File(DirectoryPath() + "/junk/" + std::to_string(ms) + fileName));
			break;
		}
		case MsgButton::bYes:
		{
			builder["autosave"] >> manager;
			ret = true;
		}
		// fall through is intentional here
		case MsgButton::bNo:
			file.deleteFile();
			break;
		}

		return ret;
	}

}

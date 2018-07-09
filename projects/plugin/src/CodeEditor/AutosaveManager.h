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

	file:AutosaveManager.h
		
		Manages autosaving the current contents regularly

*************************************************************************************/

#ifndef APE_AUTOSAVEMANAGER_H
	#define APE_AUTOSAVEMANAGER_H

	#include <cpl/Common.h>
	#include <mutex>
	#include <future>
	#include <memory>
	#include <cpl/CExclusiveFile.h>
	#include <vector>

	namespace cpl
	{
		class CCheckedSerializer;
	}

	namespace juce
	{
		class File;
	}

	namespace ape
	{
		class CConsole;
		class UIController;
		class SourceManager;
		class Settings;

		/// <summary>
		/// 
		/// </summary>
		class AutosaveManager : juce::Timer
		{
		public:
			
			AutosaveManager(int ID, Settings& settings, SourceManager& manager, UIController& controller);
			~AutosaveManager();

			bool checkAutosave();

		protected:

			void timerCallback() override;

		private:

			bool potentiallyRestoreAutosave(const std::vector<std::byte>& src, juce::File& file);
			bool performAutosave();
			bool asyncSave(std::unique_ptr<cpl::CCheckedSerializer> state);

			static std::mutex directoryLock;
			int instanceID;
			SourceManager& manager;
			UIController& controller;
			Settings& settings;
			std::future<bool> autosaveState;
			cpl::CExclusiveFile autosaveFile;
			bool checked, wasRestored;
		};

	}
#endif
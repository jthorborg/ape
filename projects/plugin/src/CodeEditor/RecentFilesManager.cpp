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

	file:RecentFilesManager.cpp
		
		Implementation of RecentFilesManager.h

*************************************************************************************/


#include "RecentFilesManager.h"
#include <cpl/Misc.h>

namespace ape
{
    juce::RecentlyOpenedFilesList& RecentFilesManager::get()
    {
        using namespace cpl::fs;

        class RetainFiles : public juce::RecentlyOpenedFilesList
        {
        public:

            RetainFiles()
            {
                auto result = cpl::Misc::ReadFile(cpl::Misc::DirFSPath() / "recent files.txt");

                if (result.first)
                {
                    restoreFromString(result.second);
                }
            }

            ~RetainFiles()
            {
                const auto currents = toString();
                cpl::Misc::WriteFile(cpl::Misc::DirFSPath() / "recent files.txt", currents.toStdString());
            }
        };

        static RetainFiles files;

        return files;
    }
}

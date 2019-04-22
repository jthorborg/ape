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
 
	 file:SourceFile.cpp
	 
		Implementation of SourceFile.h

 *************************************************************************************/


#include "SourceFile.h"
#include <FileSystemWatcher/FileSystemWatcher.h>
#include <set>
#include <cpl/Exceptions.h>

namespace ape
{
	class SourceFile::ListenerManager : public FileSystemWatcher::Listener
	{
	public:

		ListenerManager(SourceFile& parent)
			: parent(parent)
		{
			auto folder = parent.path.parent_path();

			CPL_RUNTIME_ASSERTION(fs::exists(folder));
			watcher.addFolder(juce::File(folder.string()));
			watcher.addListener(this);
		}

		std::set<SourceFile::Listener*> listeners;

		virtual void folderChanged(const juce::File)
		{
			parent.onFileChanged();
		}

	private:

		SourceFile& parent;
		FileSystemWatcher watcher;
	};

	SourceFile::SourceFile(fs::path pathToUse)
		: path(std::move(pathToUse))
	{
		isFile = fs::exists(path);
	}

	SourceFile::SourceFile(fs::path pathToUse, bool isActualFile)
		: path(std::move(pathToUse)), isFile(isActualFile)
	{

	}

	SourceFile::SourceFile(SourceFile && other)
		: path(std::move(other.path)), isFile(other.isFile), listenerManager(std::move(other.listenerManager))
	{
		
	}

	SourceFile& SourceFile::operator = (SourceFile&& other)
	{
		path = std::move(other.path);
		isFile = other.isFile;
		listenerManager = std::move(other.listenerManager);

		return *this;
	}

	SourceFile::~SourceFile()
	{

	}

	void SourceFile::onFileChanged()
	{
		for (auto* l : getListenerManager().listeners)
			l->fileChanged(*this);
	}

	void SourceFile::addListener(Listener & list)
	{
		getListenerManager().listeners.insert(&list);
	}

	void SourceFile::removeListener(Listener & list)
	{
		getListenerManager().listeners.erase(&list);
	}

	SourceFile::ListenerManager& SourceFile::getListenerManager()
	{
		if (!listenerManager)
			listenerManager = std::make_unique<ListenerManager>(*this);

		return *listenerManager;
	}
}
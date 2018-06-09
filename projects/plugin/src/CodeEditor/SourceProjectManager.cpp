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
 
	 file:SourceProjectManager.cpp
	 
		Implementation of the source project manager
 
 *************************************************************************************/


#include "SourceProjectManager.h"
#include <cpl/misc.h>
#include "../Engine.h"
#include "../UIController.h"
#include "../CConsole.h"
#include <ctime>
#include <cstdio>
#include <sstream>
#include <experimental/filesystem>
#include <memory>

namespace ape
{
	namespace fs = std::experimental::filesystem;

	std::unique_ptr<CCodeEditor> MakeCodeEditor(UIController& ui, const Settings& s, int instanceID)
	{
		return std::make_unique<SourceProjectManager>(ui, s, instanceID);
	}

	SourceProjectManager::SourceProjectManager(UIController& ui, const Settings& s, int instanceID)
		: CCodeEditor(ui, s, instanceID)
		, editorWindow(nullptr)
		, isInitialized(false)
		, isSingleFile(true)
		, fullPath("Untitled")
		, appName(cpl::programInfo.programAbbr + " Editor")
		, isActualFile(false)
		, wasRestored(false)
		, autoSaveChecked(false)
	{
		doc.setSavePoint();
	}

	SourceProjectManager::~SourceProjectManager()
	{
		saveIfUnsure();
		// if we successfully close (like now), we delete autosave state
		autoSaveFile.remove();
	}

	bool SourceProjectManager::initEditor()
	{
		if (!isInitialized)
		{
			editorWindow = std::make_unique<CodeEditorWindow>(doc);
			if (editorWindow)
			{
				editorWindow->getLineTracer().addTraceListener(this);
				editorWindow->setSize(800, 900);
				loadHotkeys();
				editorWindow->setAppCM(&appCM);

				/*
					open a default file from settings
				*/
				std::string file;
				try
				{
					settings.root()["languages"].lookupValue("default_file", file);
				}
				catch (const std::exception & e)
				{
					controller.console().printLine(CColours::red, "Error reading default file from config... %s", e.what());
				}
				
				if (file.length())
					openFile((cpl::Misc::DirectoryPath() + file).c_str());

				setTitle();
				return true;
			}
		}
		else
			return true;
		return false;
	}

	void SourceProjectManager::openAFile()
	{
		// array of types. these are read from the config and defines what sources
		// the program can open
		std::vector<std::string> filetypes;

		try
		{
			// look up languages part
			const auto& langs = settings.root()["languages"];
			// check if theres anything there (and if its a group)
			if (!langs.isGroup())
			{
				controller.console().printLine(CColours::red,
					"[Editor] Warning: No languages specified in config. Can't open files. ");
				return;
			}
			else
			{
				// get number of languages defined.
				int elements = langs.getLength();
				// iterate over languages
				for (int x = 0; x < elements; ++x)
				{
					if (langs[x].isGroup()) 
					{
						// look up list of extensions
						try
						{
							const auto& exts = langs[x]["extensions"];
							if (!exts.isList())
							{
								// this is not really ideal control transfer, 
								// but every lookup on libconfig::Setting might throw,
								// so we use the same method here.. and only here.
								throw std::runtime_error("Not a list");
							}
							else
							{
								// get number of extensions
								int numExts = exts.getLength();
								// iterate over these
								for (int y = 0; y < numExts; ++y)
								{
									const char * type = exts[y].c_str();
									filetypes.push_back(type);
								}
							}
						}
						catch (const std::exception & e)
						{
							controller.console().printLine(CColours::red,
								"[Editor] Warning: language %s has no defined extensions in config. "
								"Program will not be able to open any files for that language. (%s)",
								langs[x].getName(), e.what());
						}
					}
				}

			}

		}
		catch (const std::exception & e)
		{
			controller.console().printLine(CColours::red,
				"[Editor] Error parsing file type extensions in config (%s).", e.what());
			return;
		}

		std::string validTypes;
		for (auto & str : filetypes)
			validTypes += "*." + str + ";";

		juce::File initialPath(cpl::Misc::DirectoryPath() + "/examples/");
		juce::FileChooser fileSelector(cpl::programInfo.programAbbr + " - Select a source file...", initialPath, validTypes);

		if (fileSelector.browseForFileToOpen())
		{
			openFile(fileSelector.getResult().getFullPathName().toStdString());
		}
		else
		{
			std::stringstream fmt;
			fmt << "Error opening file dialog (" << 0 << ")!";
			cpl::Misc::MsgBox(fmt.str(), cpl::programInfo.programAbbr + " Error!",
				cpl::Misc::MsgStyle::sOk | cpl::Misc::MsgIcon::iWarning, getParentWindow());
		}

	}

	void SourceProjectManager::setTitle()
	{
		std::string title;
		title += appName + " (" + std::to_string(instanceID & 0xFF) + ")";
		if (isDirty())
			title += " * ";
		else
			title += " - ";
		title += fullPath;
		if (editorWindow)
			editorWindow->setName(title);
	}

	bool SourceProjectManager::openEditor(bool initialVisibility)
	{
		if (!isInitialized) 
		{
			if (!initEditor())
				return false;
			isInitialized = true;
			// on first open, we check autosave
			checkAutoSave();
		}

		editorWindow->setVisible(initialVisibility);
		editorWindow->setMinimised(false);
		// extremely buggy:
		//editorWindow->toFront(true);
		return true;
	}

	void SourceProjectManager::onBreakpointsChanged(const std::set<int>& breakpoints)
	{
		controller.onBreakpointsChanged(breakpoints);
	}


	std::string SourceProjectManager::getDocumentPath()
	{
		return fullPath;
	}

	bool SourceProjectManager::closeEditor()
	{
		if (isInitialized)
		{
			editorWindow->setVisible(false);
			return true;
		}
		else
			return false;
	}

	void SourceProjectManager::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo & result)
	{
		auto & cDesc = CommandTable[Menus::File][commandID];
		juce::ApplicationCommandInfo aci(cDesc.command);
		if(userHotKeys[commandID].size())
		{
			aci.defaultKeypresses.add(juce::KeyPress::createFromDescription(userHotKeys[commandID]));
		}
		else
			aci.addDefaultKeypress(cDesc.key, cDesc.modifier);
		aci.shortName = cDesc.name;
		aci.flags = 0;
		aci.setActive(true);
		result = aci;
	}

	void SourceProjectManager::getAllCommands(juce::Array<juce::CommandID> & commands)
	{
		for (int c = Command::FileNew; c < Command::End; ++c)
		{
			commands.add(c);
		}
	}

	void SourceProjectManager::newDocument()
	{
		fullPath = "Untitled";
		doc.clearUndoHistory();
		doc.replaceAllContent("");
		doc.setSavePoint();
		setTitle();
		isActualFile = false;
	}

	bool SourceProjectManager::perform(const InvocationInfo & info)
	{
		
		switch (info.commandID)
		{
		case Command::FileNew:
			if (saveIfUnsure() != cpl::Misc::MsgButton::bCancel) {
				newDocument();
			}
			break;
		case Command::FileOpen:
			if (saveIfUnsure() != cpl::Misc::MsgButton::bCancel) {
				openAFile();
			}
			break;
		case Command::FileSave:
			saveCurrentFile();
			break;
		case Command::FileSaveAs:
			saveAs();
			break;
		case Command::FileExit:
			closeEditor();
			break;
		}
		return true;
	}

	std::unique_ptr<ProjectEx> SourceProjectManager::getProject()
	{
		/*
		Implement a proper interface instead of passing shitty c strings around.
		Consider the implementation of this.
		*/
		std::unique_ptr<ProjectEx> project;
		// for now we only support single files. 
		// we set the appropiate values in the project struct and fill it.
		if (isSingleFile) 
		{
			project = std::make_unique<ProjectEx>();
			project->files = nullptr;
			project->nFiles = 1;
			project->uniqueID = (unsigned)-1;
			project->isSingleString = true;
			std::string text;
			// Copy strings here. We have to do it this tedious way to stay c-compatible.
			// only FreeProjectStruct is supposed to free this stuff.
			// copy document text
			;
			auto len = text.length();

			auto assignCStr = [] (const std::string & orig, auto & location)
			{
				auto length = std::size(orig);
				if (!length)
					return false;
				auto pointer = new char[length + 1];
				std::copy(std::begin(orig), std::end(orig), pointer);
				pointer[length] = '\0';
				location = pointer;
				return true;
			};
			auto fileLocations = new char *[1];
			project->files = fileLocations;
			if (getDocumentText(text) && text.length() != 0 && 
				assignCStr(text, project->sourceString) &&
				assignCStr(getProjectName(), project->projectName) &&
				assignCStr(cpl::Misc::DirectoryPath(), project->rootPath) &&
				assignCStr(getExtension(), project->languageID) &&
				assignCStr(fullPath, fileLocations[0]))
			{
				project->nFiles = 1;

				if (editorWindow)
				{
					const auto& tracedLines = editorWindow->getLineTracer().getTracedLines();
					auto copiedLines = new int[tracedLines.size()];
					std::size_t counter = 0;
					for (auto line : tracedLines)
						copiedLines[counter++] = line;

					project->traceLines = copiedLines;
					project->numTraceLines = counter;
				}

				project->state = CodeState::None;
				return project;
			}
		}
		// some error occured.
		return nullptr;
	}

	void SourceProjectManager::setErrorLine(int line)
	{


	}

	bool SourceProjectManager::getDocumentText(std::string & buffer)
	{
		buffer.append( doc.getAllContent().toRawUTF8());
		return true;
	}

	std::string SourceProjectManager::getDocumentName() 
	{
		return fs::path(fullPath).filename().string();
	}

	std::string SourceProjectManager::getDirectory() 
	{
		// TODO: fullpath -> std::path
		return fs::path(fullPath).parent_path().string();
	}

	bool SourceProjectManager::loadHotkeys()
	{
		/*
		 try to read the hotkeys from editor {}
		 */
		try
		{
			std::string temp;
			const auto& root = settings.root();
			if(root["editor"].lookupValue("hkey_save", temp))
				userHotKeys[Command::FileSave] = temp;
			if(root["editor"].lookupValue("hkey_new", temp))
				userHotKeys[Command::FileNew] = temp;
			if(root["editor"].lookupValue("hkey_open", temp))
				userHotKeys[Command::FileOpen] = temp;
		}
		catch (const std::exception & e)
		{
			controller.console().printLine(CColours::red,
													   "[Editor] : Error reading editor hotkeys from config... %s", e.what());
			return false;
		}
		
		appCM.registerAllCommandsForTarget(this);
		appCM.setFirstCommandTarget(this);
		
		return true;
	}

	std::string SourceProjectManager::getExtension()
	{
		for (signed int i = static_cast<signed int>(fullPath.length());
			i >= 0;
			--i)
		{
			if (fullPath[i] == '.') // if fullPath is a path and not an unsaved file, shave the path off
				// by looping backwards and finding the first dot.
				return fullPath.substr(i + 1);
		}
		return "";
	}

	std::string SourceProjectManager::getProjectName()
	{
		if (isInitialized) {
			if (isSingleFile) {
				// fix this obviously when we start using project files.
				std::string ret = getDocumentName();
				signed int end = static_cast<signed int>(ret.size());
				// scan backwards till we get the filename without extension
				while (end > 0 && ret[--end] != '.');
				if (end)
					return std::string(ret.begin(), ret.begin() + end);
				else
					return ret;
			}
			// ...
		}
		return "";
	}

	bool SourceProjectManager::isDirty()
	{
		return doc.hasChangedSinceSavePoint();
	}

	int SourceProjectManager::saveIfUnsure()
	{
		using namespace cpl::Misc;
		if (isDirty()) {
			std::string msg("Save changes to \"");
			msg += fullPath;
			msg += "\"?";
			int decision = MsgBox(msg, cpl::programInfo.programAbbr, MsgStyle::sYesNoCancel | MsgIcon::iQuestion, getParentWindow(), true);
			if (decision == MsgButton::bYes) {
				if (isActualFile)
					saveCurrentFile();
				else
					saveAs();
			}
			return decision;
		}
		return MsgButton::bYes;
	}

	void SourceProjectManager::saveCurrentFile() {
		if (isActualFile)
			doSaveFile(fullPath);
		else
			saveAs();
	}

	bool SourceProjectManager::openFile(const std::string & fileName)
	{

		fullPath = fileName;
		juce::File f(fullPath);
		juce::FileInputStream s(f);
		if (s.openedOk()) {
			setTitle();
			doc.replaceAllContent("");
			doc.loadFromStream(s);
		}
		else {
			std::string msg("Could not open file \"");
			msg += fullPath;
			msg += "\".";
			cpl::Misc::MsgBox(msg, appName, cpl::Misc::MsgStyle::sOk, getParentWindow(), true);
			return false;
		}
		setTitle();
		doc.setSavePoint();
		isActualFile = true;
		return true;
	}

	void SourceProjectManager::saveAs() {
		 
		juce::File suggestedPath(isActualFile ? fullPath : cpl::Misc::DirectoryPath());

		juce::FileChooser fileSelector(cpl::programInfo.programAbbr + " :: Select where to save your file...", suggestedPath);
		if (fileSelector.browseForFileToSave(true))
		{
			fullPath = fileSelector.getResult().getFullPathName().toStdString();

			setTitle();
			doSaveFile(fullPath);
		}
		else
		{
			std::stringstream fmt;
			fmt << "Error opening save file dialog (" << false << ")!";
			cpl::Misc::MsgBox(fmt.str(), cpl::programInfo.programAbbr + " Error!",
				cpl::Misc::MsgStyle::sOk | cpl::Misc::MsgIcon::iWarning, getParentWindow());
		}
	}

	void SourceProjectManager::doSaveFile(const std::string & fileName)
	{
		fullPath = fileName;
		FILE *fp = nullptr;
		#ifdef CPL_MSVC
			fopen_s(&fp, fullPath.c_str(), "wb");
		#else
			fp = fopen(fullPath.c_str(), "wb");
		#endif
		if (fp)
		{
			std::string data;
			getDocumentText(data);
			auto ret = fwrite(data.data(), data.size(), 1, fp);
			if (ret != 1)
			{
				using namespace cpl::Misc;
				std::stringstream fmt;
				fmt << "Error saving to file \"" << fullPath << "\"." 
					<< std::endl << "Wrote " << ret * data.size() << " bytes, expected " << data.size() << " bytes.";
				MsgBox(fmt.str(), cpl::programInfo.programAbbr + " error!", MsgStyle::sOk | MsgIcon::iStop, getParentWindow());
			}
			else
			{
				doc.setSavePoint();
			}
			fclose(fp);
		}
		else 
		{
			using namespace cpl::Misc;
			std::stringstream fmt;
			fmt << "Could not save file \"" << fullPath << "\".";
			MsgBox(fmt.str(), cpl::programInfo.programAbbr + " error!", MsgStyle::sOk | MsgIcon::iStop, getParentWindow());
		}
	}

	struct AutoSaveInfo
	{
		int wasDirty;
		time_t timeStamp;
		std::size_t fileNameOffset;
		std::size_t textDataOffset;

		char * getFileName()
		{
			return reinterpret_cast<char*>(this) + fileNameOffset;
		}
		char * getTextData()
		{
			return reinterpret_cast<char*>(this) + textDataOffset;
		}
	};

	void SourceProjectManager::autoSave()
	{
		// only create an autosave file if current contents are unsaved
		if (isDirty())
		{
			// close the currently open file
			if(!autoSaveFile.isOpened())
			{
				std::string path = cpl::Misc::DirectoryPath() + "/logs/autosave"
				+ std::to_string(instanceID) + ".ape";
				autoSaveFile.open(path);
			}
			else
			{
				autoSaveFile.reset();
			}
			
			auto fileName = fullPath;
			time_t timestamp;
			std::time(&timestamp);
			std::string buf;
			getDocumentText(buf);

			auto size_needed = sizeof (AutoSaveInfo) + fileName.size() + 1 + buf.size() + 1;

			AutoSaveInfo * info = static_cast<AutoSaveInfo*>(std::malloc(size_needed));
			info->wasDirty = isDirty();
			info->timeStamp = timestamp;
			info->fileNameOffset = sizeof (AutoSaveInfo);
			std::memcpy(info->getFileName(), fileName.c_str(), fileName.size() + 1);
			info->textDataOffset = info->fileNameOffset + fileName.size() + 1;
			std::memcpy(info->getTextData(), buf.c_str(), buf.size() + 1);

			if (autoSaveFile.isOpened())
			{
				autoSaveFile.write(info, size_needed);
				autoSaveFile.flush();
				// we flush the file and keep it open in memory, so other instances
				// of this plugin dont try to open our autosave.
			}
			else
			{
				controller.console().printLine(CColours::red,
													"[Editor] : error opening file %s for autosave.", autoSaveFile.getName().c_str());
			}
			std::free(info);
			controller.setStatusText("Autosaved...", CColours::lightgoldenrodyellow, 2000);
		}
	}

	void * SourceProjectManager::getParentWindow()
	{
		if (isInitialized && editorWindow && editorWindow->isVisible())
			return editorWindow->getWindowHandle();
		return controller.getSystemWindow();
	}

	bool SourceProjectManager::restoreAutoSave(AutoSaveInfo * info, juce::File & parentFile)
	{
		using namespace cpl::Misc;
		bool ret = false; // whether we actually restored a state
		if (!info->wasDirty)
			return false;
		std::stringstream fmt;

		time_t timeObj = info->timeStamp;
		tm * ctime;
		#ifdef CPL_MSVC
			tm pTime;
			gmtime_s(&pTime, &timeObj);
			ctime = &pTime;
		#else
			// consider using a safer alternative here.
			ctime = gmtime(&timeObj);			
		#endif

		std::string fullFileName = info->getFileName();

		fmt << "Recoverable file found: " << fullFileName << std::endl;
		fmt << "File was last autosaved at " << ctime->tm_hour << ":" << ctime->tm_min << ":"
			<< ctime->tm_sec << ", " << ctime->tm_mday << "/" << ctime->tm_mon + 1 << "." << std::endl;
		fmt << "Do you want to open this file (yes), delete it (no) or move it to /junk/ folder (cancel)?";

		std::string fileName = fs::path(fullFileName).filename().string();


		auto answer = MsgBox(fmt.str(), "ape - Autorecover", MsgStyle::sYesNoCancel | MsgIcon::iQuestion, getParentWindow(), true);

		switch (answer)
		{
		case MsgButton::bCancel:
			parentFile.moveFileTo(juce::File(DirectoryPath() + "/junk/" + std::to_string(info->timeStamp) + fileName));
			// move file to a junk folder
			break;
		case MsgButton::bYes:
			{
				std::string newName = "(Recovered) " + fileName;

				setContents(info->getTextData());
				fullPath = newName;
				setTitle();
				isActualFile = false;
				ret = true;
			}
			// fall through is intentional here
		case MsgButton::bNo:
			parentFile.deleteFile();
			break;
		}
		return ret;
	}

	void SourceProjectManager::setContents(const juce::String & newContent)
	{
		doc.replaceAllContent(newContent);
	}

	bool SourceProjectManager::checkAutoSave()
	{
		// prevent multiple instances of this plugin to access the same files at once
		static cpl::CMutex::Lockable autoSaveMutex;
		cpl::CMutex lock(autoSaveMutex);

		if (autoSaveChecked)
			return wasRestored;


		juce::DirectoryIterator dir(juce::File(cpl::Misc::DirectoryPath() + "/logs/"), false, "autosave*.ape");
		
		bool restored(false);
		/*
			Iterate over all autosave files until user chooses to open one, or we deleted/moved all existing ones.
		*/
		while (dir.next())
		{
			juce::File f(dir.getFile());
			
			// check if file is opened in exclusive mode by another instance of this program
			// ie: whether it's an autosave file currently being used, or if it's an old file
			// that we can safely mess with / restore / delete
			if(cpl::CExclusiveFile::isFileExclusive(f.getFullPathName().toRawUTF8()))
				continue;

			
			juce::ScopedPointer<juce::FileInputStream> stream = f.createInputStream();
	
			// this really should fail if an autosave file in another instance is opened in none-shared mode
			// ie. this is a feature: we wont offer to open autosaves created by other active instances
			if (!stream.get())
				continue;

			auto size = stream->getTotalLength();

			// valid autosave file?
			if (!size || size < sizeof(AutoSaveInfo) || size == -1)
			{
				// nope -- close opened stream
				stream = nullptr; // alternative: delete stream.release();

				// report anomaly
				controller.console().printLine(CColours::black,
					"[Editor] : Invalid autosave file found - deleting... (%s)",
					f.getFileName().toRawUTF8());

				// delete the file
				std::remove(f.getFullPathName().toRawUTF8());
				// NEXT
				continue;
			}

			char * src = static_cast<char*>(std::malloc(static_cast<std::size_t>(size) + 1));

			if (src)
			{
				auto bytesRead = stream->read(src, static_cast<int>(size));
				if (bytesRead == size)
					src[size] = '\0'; // nullterminate the extra byte we allocated
				else
				{
					controller.console().printLine(CColours::red,
						"[Editor] : Error reading file contents of %s (read %d, expected %d bytes)",
						f.getFileName().toRawUTF8(), bytesRead, size);
					std::free(src);
					src = nullptr;
					// error reading this file -- continue to next file in directory
					continue;
				}
			}

			// close input stream
			stream = nullptr;

			/*
				at this point, we closed any handles associated with the file we read from.
				if src is not a nullptr, any contents were successfully copied into the buffer.
			*/
			if (src)
			{
				restored = restoreAutoSave(reinterpret_cast<AutoSaveInfo *> (src), f);
				std::free(src);
				src = nullptr;
			}


			if (restored)
				break;
		}


		wasRestored = restored;
		autoSaveChecked = true;
		return restored;
	}
}
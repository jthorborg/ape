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
#include "CodeEditorWindow.h"

namespace ape
{
	namespace fs = std::experimental::filesystem;

	std::unique_ptr<SourceManager> MakeSourceManager(UIController& ui, const Settings& s, int instanceID)
	{
		return std::make_unique<SourceProjectManager>(ui, s, instanceID);
	}

	SourceProjectManager::SourceProjectManager(UIController& ui, const Settings& s, int instanceID)
		: SourceManager(ui, s, instanceID)
		, editorWindow(nullptr)
		, isSingleFile(true)
		, fullPath("Untitled")
		, appName(cpl::programInfo.programAbbr + " Editor")
		, isActualFile(false)
		, editorWindowState([this] { return createWindow(); })
	{

		try
		{
			std::string file;

			settings.root()["languages"].lookupValue("default_file", file);
			if (file.length())
				openFile((cpl::Misc::DirectoryPath() + file).c_str());

			setTitle();

		}
		catch (const std::exception & e)
		{
			controller.console().printLine(CColours::red, "Error reading default file from config... %s", e.what());
		}

	}

	SourceProjectManager::~SourceProjectManager()
	{
		saveIfUnsure();
	}

	void SourceProjectManager::serialize(cpl::CSerializer::Archiver & ar, cpl::Version version)
	{
		ar["code-editor"]["state"] = editorWindowState.getState();
		ar["code-editor"] << (editorWindowState.hasCached() && editorWindowState.getCached()->isVisible()); // is editor open?

		ar["source-path"] << fullPath;

		std::string contents;
		getDocumentText(contents);
		ar["source"] << contents;

		ar["traces"]["size"] << breakpoints.size();

		for (auto trace : breakpoints)
			ar["traces"]["content"] << trace;
	}

	void SourceProjectManager::deserialize(cpl::CSerializer::Builder & builder, cpl::Version version)
	{
		editorWindowState.setState(builder["code-editor"]["state"], builder["code-editor"]["state"].getLocalVersion());
		bool editorOpen = false;
		builder["code-editor"] >> editorOpen;

		builder["source-path"] >> fullPath;
		std::string contents;
		builder["source"] >> contents;
		doc.replaceAllContent(contents);

		std::size_t numTraces;
		builder["traces"]["size"] >> numTraces;

		for (std::size_t i = 0; i < numTraces; ++i)
		{
			int trace;
			builder["traces"]["content"] >> trace;
			breakpoints.emplace(trace);
		}

		if (editorOpen)
			setEditorVisibility(true);
	}

	std::unique_ptr<CodeEditorWindow> SourceProjectManager::createWindow()
	{
		auto window = std::make_unique<CodeEditorWindow>(settings, doc);
		window->setBreakpoints(breakpoints);
		window->addBreakpointListener(this);
		loadHotkeys();
		window->setAppCM(&appCM);
		return window;
	}

	bool SourceProjectManager::setEditorVisibility(bool visible)
	{
		if (!editorWindow)
		{
			editorWindow = editorWindowState.getCached();
			setTitle();
		}

		editorWindow->setVisible(visible);

		if (visible)
			editorWindow->setMinimised(false);

		return true;
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


	void SourceProjectManager::onBreakpointsChanged(const std::set<int>& newBreakpoints)
	{
		breakpoints = newBreakpoints;
		controller.onBreakpointsChanged(breakpoints);
	}

	std::string SourceProjectManager::getDocumentPath()
	{
		return fullPath;
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
			setEditorVisibility(false);
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
			fileLocations[0] = nullptr;
			project->files = fileLocations;
			if (getDocumentText(text) && text.length() != 0 && 
				assignCStr(text, project->sourceString) &&
				assignCStr(getProjectName(), project->projectName) &&
				assignCStr(cpl::Misc::DirectoryPath(), project->rootPath) &&
				assignCStr(getExtension(), project->languageID) &&
				assignCStr(fullPath, fileLocations[0]))
			{
				auto copiedLines = new int[breakpoints.size()];
				std::size_t counter = 0;
				for (auto line : breakpoints)
					copiedLines[counter++] = line;

				project->traceLines = copiedLines;
				project->numTraceLines = counter;

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
			controller.console().printLine(CColours::red, "[Editor] : Error reading editor hotkeys from config... %s", e.what());
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
		const auto& name = getDocumentName();
		return name.substr(0, name.find_last_of('.'));
	}

	bool SourceProjectManager::isDirty()
	{
		return doc.hasChangedSinceSavePoint();
	}

	int SourceProjectManager::saveIfUnsure()
	{
		using namespace cpl::Misc;
		if (isDirty()) 
		{
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

		if (s.openedOk()) 
		{
			doc.replaceAllContent("");
			doc.loadFromStream(s);
		}
		else 
		{
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

	void * SourceProjectManager::getParentWindow()
	{
		if (editorWindow && editorWindow->isVisible())
			return editorWindow->getWindowHandle();
		return controller.getSystemWindow();
	}

	void SourceProjectManager::setContents(const juce::String & newContent)
	{
		doc.replaceAllContent(newContent);
	}
}
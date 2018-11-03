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
#include <sstream>
#include <experimental/filesystem>
#include <memory>
#include "CodeEditorWindow.h"
#include <fstream>

namespace ape
{
	namespace fs = std::experimental::filesystem;

	std::unique_ptr<SourceManager> MakeSourceManager(UIController& ui, const Settings& s, int instanceID)
	{
		return std::make_unique<SourceProjectManager>(ui, s, instanceID);
	}

	SourceProjectManager::SourceProjectManager(UIController& ui, const Settings& s, int instanceID)
		: SourceManager(ui, s, instanceID)
		, isSingleFile(true)
		, fullPath("Untitled")
		, isActualFile(false)
		, lastDirtyState(false)
		, textEditorDSO([this] { return createWindow(); })
	{
		doc = std::make_shared<juce::CodeDocument>();
		doc->addListener(this);

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
			controller.console().printLine(CConsole::Error, "Error reading default file from config... %s", e.what());
		}

	}

	SourceProjectManager::~SourceProjectManager()
	{
		saveIfUnsure();
	}

	void SourceProjectManager::serialize(cpl::CSerializer::Archiver & ar, cpl::Version version)
	{
		ar["code-editor"]["state"] = textEditorDSO.getState();
		ar["source-path"] << fullPath.string();

		std::string contents;
		getDocumentText(contents);
		ar["source"] << contents;

		ar["traces"]["size"] << breakpoints.size();

		for (auto trace : breakpoints)
			ar["traces"]["content"] << trace;
	}

	void SourceProjectManager::deserialize(cpl::CSerializer::Builder & builder, cpl::Version version)
	{
		textEditorDSO.setState(builder["code-editor"]["state"], builder["code-editor"]["state"].getLocalVersion());

		std::string path;

		builder["source-path"] >> path;
		fullPath = path;
		// TODO: Check contents vs. fullPath

		std::string contents;
		builder["source"] >> contents;
		doc->replaceAllContent(contents);

		std::size_t numTraces;
		builder["traces"]["size"] >> numTraces;

		for (std::size_t i = 0; i < numTraces; ++i)
		{
			int trace;
			builder["traces"]["content"] >> trace;
			breakpoints.emplace(trace);
		}

		validateInvariants();
	}

	void SourceProjectManager::addListener(CodeDocumentListener & listener)
	{
		listeners.insert(&listener);
	}

	void SourceProjectManager::removeListener(CodeDocumentListener & listener)
	{
		listeners.erase(&listener);
	}

	void SourceProjectManager::validateInvariants()
	{
		using namespace cpl::Misc;

		bool currentlyExists = fs::exists(fullPath);

		if (currentlyExists && isActualFile)
		{
			juce::File f(fullPath.string());
			juce::FileInputStream s(f);
			auto contents = s.readEntireStreamAsString();

			if (doc->getAllContent() == contents)
			{
				doc->setSavePoint();
				checkDirtynessState();
			}
		}
	}

	void SourceProjectManager::checkDirtynessState()
	{
		bool status = doc->hasChangedSinceSavePoint();
		bool trigger = !lastDirtyState || lastDirtyState.value() != status;
		
		lastDirtyState = status;

		if (trigger)
		{
			for (auto listener : listeners)
				listener->documentDirtynessChanged(status);
		}
	}

	std::unique_ptr<juce::Component> SourceProjectManager::createCodeEditorComponent()
	{
		return std::unique_ptr<juce::Component> { textEditorDSO.getUnique().acquire() };
	}

	std::unique_ptr<CodeEditorComponent> SourceProjectManager::createWindow()
	{
		auto textEditor = std::make_unique<CodeEditorComponent>(settings, doc, static_cast<CodeDocumentSource&>(*this));
		textEditor->getLineTracer().setBreakpoints(breakpoints);
		textEditor->getLineTracer().addBreakpointListener(this);

		// TODO: Should this be here?
		bool currentlyExists = fs::exists(fullPath);

		if (currentlyExists && isActualFile)
		{
			juce::File f(fullPath.string());
			juce::FileInputStream s(f);
			auto contents = s.readEntireStreamAsString();

			if (doc->getAllContent() != contents)
			{
				using namespace cpl::Misc;

				std::string message = "File on disk: \"" + fullPath.string() + "\"\nis different from loaded document, do you want to reload the disk version?";

				int choice = MsgBox(message, cpl::programInfo.name, MsgStyle::sYesNo | MsgIcon::iQuestion, getParentWindow(), true);

				if (choice == MsgButton::bYes)
				{
					openFile(fullPath);
				}
			}
		}

		textEditor->documentDirtynessChanged(doc->hasChangedSinceSavePoint());
		textEditor->documentChangedName(getDocumentName());

		return std::move(textEditor);
	}

	std::unique_ptr<DockWindow> SourceProjectManager::createSuitableCodeEditorWindow()
	{
		auto window = std::make_unique<CodeEditorWindow>(settings);

		loadHotkeys();
		window->setAppCM(&appCM);

		return std::move(window);
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
				controller.console().printLine(CConsole::Warning,
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
							controller.console().printLine(CConsole::Warning,
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
			controller.console().printLine(CConsole::Error,
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
	}

	void SourceProjectManager::setTitle()
	{
		for (auto listener : listeners)
			listener->documentChangedName(getDocumentName());

		return;
	}


	void SourceProjectManager::onBreakpointsChanged(const std::set<int>& newBreakpoints)
	{
		breakpoints = newBreakpoints;
		controller.onBreakpointsChanged(breakpoints);
	}

	fs::path SourceProjectManager::getDocumentPath()
	{
		return fullPath;
	}

	void SourceProjectManager::newDocument()
	{
		fullPath = "Untitled";
		doc->clearUndoHistory();
		doc->replaceAllContent("");
		doc->setSavePoint();
		checkDirtynessState();
		setTitle();
		isActualFile = false;
	}


	std::unique_ptr<ProjectEx> SourceProjectManager::createProject()
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
				assignCStr(fullPath.string(), fileLocations[0]))
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
		buffer.append(doc->getAllContent().toRawUTF8());
		return true;
	}

	std::string SourceProjectManager::getDocumentName() 
	{
		return fullPath.filename().string();
	}

	std::string SourceProjectManager::getDirectory() 
	{
		// TODO: fullpath -> std::path
		return fullPath.parent_path().string();
	}

	std::string SourceProjectManager::getExtension()
	{
		if(fullPath.has_extension())
			return fullPath.extension().string().substr(1);

		return "";
	}

	std::string SourceProjectManager::getProjectName()
	{
		return fullPath.stem().string();
	}

	bool SourceProjectManager::isDirty()
	{
		return doc->hasChangedSinceSavePoint();
	}

	int SourceProjectManager::saveIfUnsure()
	{
		using namespace cpl::Misc;
		if (isDirty()) 
		{
			std::string msg("Save changes to \"");
			msg += fullPath.string();
			msg += "\"?";
			int decision = MsgBox(msg, cpl::programInfo.name, MsgStyle::sYesNoCancel | MsgIcon::iQuestion, getParentWindow(), true);
			if (decision == MsgButton::bYes) 
			{
				if (isActualFile)
					saveCurrentFile();
				else
					saveAs();
			}
			return decision;
		}
		return MsgButton::bYes;
	}

	void SourceProjectManager::saveCurrentFile() 
	{
		if (isActualFile)
			doSaveFile(fullPath);
		else
			saveAs();
	}

	bool SourceProjectManager::openFile(const fs::path& fileName)
	{
		juce::File f(fileName.string());
		juce::FileInputStream s(f);

		if (!s.openedOk()) 
		{
			std::string msg("Could not open file \"");
			msg += fileName.string();
			msg += "\".";
			cpl::Misc::MsgBox(msg, cpl::programInfo.name, cpl::Misc::MsgStyle::sOk, getParentWindow(), true);
			return false;
		}

		doc->replaceAllContent("");
		doc->loadFromStream(s);
		doc->setSavePoint();
		checkDirtynessState();

		isActualFile = true;
		fullPath = fileName;

		setTitle();

		return true;
	}

	void SourceProjectManager::saveAs() 
	{
		juce::File suggestedPath(isActualFile ? fullPath.string() : cpl::Misc::DirectoryPath());

		juce::FileChooser fileSelector(cpl::programInfo.programAbbr + " :: Select where to save your file...", suggestedPath);
		if (fileSelector.browseForFileToSave(true))
		{
			fullPath = fileSelector.getResult().getFullPathName().toStdString();

			setTitle();
			doSaveFile(fullPath);
		}
	}

	void SourceProjectManager::doSaveFile(const fs::path& fileName)
	{
		using namespace cpl::Misc;

		fullPath = fileName;

		std::ofstream file(fileName.string().c_str());

		if (file)
		{
			std::string data;
			getDocumentText(data);

			if (!file.write(data.data(), data.size()))
			{
				std::stringstream fmt;
				fmt << "Error saving to file \"" << fullPath << "\".";
				MsgBox(fmt.str(), cpl::programInfo.name, MsgStyle::sOk | MsgIcon::iStop, getParentWindow());
			}
			else
			{
				doc->setSavePoint();
				checkDirtynessState();
				setTitle();
			}
		}
		else 
		{
			std::stringstream fmt;
			fmt << "Could not open file \"" << fullPath << "\" for saving.";
			MsgBox(fmt.str(), cpl::programInfo.name, MsgStyle::sOk | MsgIcon::iStop, getParentWindow());
		}
	}

	void * SourceProjectManager::getParentWindow()
	{
		if (textEditorDSO.hasCached())
			return textEditorDSO.getCached()->getWindowHandle();

		return controller.getSystemWindow();
	}

	void SourceProjectManager::codeDocumentTextInserted(const juce::String & newText, int insertIndex)
	{
		checkDirtynessState();
	}

	void SourceProjectManager::codeDocumentTextDeleted(int startIndex, int endIndex)
	{
		checkDirtynessState();
	}

	void SourceProjectManager::setContents(const juce::String& newContent)
	{
		doc->replaceAllContent(newContent);
	}
}
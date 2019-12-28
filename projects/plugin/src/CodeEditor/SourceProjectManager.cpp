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
#include <cpl/Misc.h>
#include "../Engine.h"
#include "../UIController.h"
#include "../CConsole.h"
#include <ctime>
#include <sstream>
#include <cpl/filesystem.h>
#include <memory>
#include <fstream>
#include <cpl/Process.h>
#include "RecentFilesManager.h"

namespace ape
{
    namespace fs = cpl::fs;

	std::unique_ptr<SourceManager> MakeSourceManager(UIController& ui, const Settings& s, int instanceID)
	{
		return std::make_unique<SourceProjectManager>(ui, s, instanceID);
	}

	SourceProjectManager::SourceProjectManager(UIController& ui, const Settings& s, int instanceID)
		: SourceManager(ui, s, instanceID)
		, shouldCheckContentsAgainstDisk(true)
		, enableScopePoints(false)
		, lastDirtyState(false)
		, textEditorDSO([this] { return createWindow(); })
		, sourceFile("untitled")
	{
		doc = std::make_shared<juce::CodeDocument>();
		doc->addListener(this);
		doc->setSavePoint();

		try
		{
			std::string stringTemplatePath, stringHomeDirectory;

			if(settings.exists("editor", "external_edit_command_line"))
				editCommandLine = settings.lookUpValue("", "editor", "external_edit_command_line");

			stringHomeDirectory = settings.lookUpValue((cpl::Misc::DirectoryPath() + "/examples/").c_str(), "languages", "home");
			defaultLanguageExtension = settings.lookUpValue("hpp", "languages", "default");
			stringTemplatePath = settings.lookUpValue("", "languages", "default_file");
			enableScopePoints = settings.lookUpValue(false, "editor", "enable_scopepoints");
			shouldCheckContentsAgainstDisk = settings.lookUpValue(true, "editor", "check_restored_against_disk");

			homeDirectory = stringHomeDirectory;

			if (!fs::exists(homeDirectory) || !fs::is_directory(homeDirectory))
				homeDirectory = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName().toStdString();

			templateFile = stringTemplatePath;

			if (!fs::exists(templateFile))
			{
				templateFile = homeDirectory / templateFile;

				if (!fs::exists(templateFile))
					templateFile = fs::path();
			}

			openTemplate();

			setTitle();
		}
		catch (const std::exception & e)
		{
			controller.getConsole().printLine(CConsole::Error, "Error setting up editor defaults: %s", e.what());
		}

		cacheValidFileTypes(s);

		loadHotkeys();
	}

	SourceProjectManager::~SourceProjectManager()
	{
		saveIfUnsure();
		sourceFile = SourceFile::asNonExisting("untitled");
	}

	void SourceProjectManager::serialize(cpl::CSerializer::Archiver & ar, cpl::Version version)
	{
		ar["code-editor"]["state"] = textEditorDSO.getState();
		ar["source-file"] << sourceFile;

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

		builder["source-file"] >> sourceFile;

		std::string contents;
		builder["source"] >> contents;

		doc->replaceAllContent(contents);
		doc->clearUndoHistory();
		doc->setSavePoint();

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
		if (sourceFile.isActualFile())
		{
			juce::FileInputStream s(sourceFile.getJuceFile());
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
		bool trigger = !lastDirtyState || *lastDirtyState != status;
		
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
		auto textEditor = std::make_unique<CodeEditorComponent>(settings, doc, *this);
		textEditor->getLineTracer().setBreakpoints(breakpoints);
		textEditor->getLineTracer().addBreakpointListener(this);

		if (shouldCheckContentsAgainstDisk && sourceFile.isActualFile())
		{
			if (juce::FileInputStream s(sourceFile.getJuceFile()); s.openedOk())
			{
				auto contents = s.readEntireStreamAsString();
				auto current = doc->getAllContent();
				if (current != contents)
				{
					using namespace cpl::Misc;

					std::string message = "File on disk: \"" + sourceFile.getPath().string() + "\"\nis different from loaded document, do you want to reload the disk version?";

					int choice = MsgBox(message, cpl::programInfo.name, MsgStyle::sYesNo | MsgIcon::iQuestion, getParentWindow(), true);

					if (choice == MsgButton::bYes)
					{
						openFile(sourceFile.getPath());
					}
				}
			}

		}

		textEditor->documentDirtynessChanged(doc->hasChangedSinceSavePoint());
		textEditor->documentChangedName(sourceFile.getName());

		return std::move(textEditor);
	}

	void SourceProjectManager::openAFile()
	{
		juce::File initialPath(homeDirectory.string());
		juce::FileChooser fileSelector(cpl::programInfo.programAbbr + " - Select a source file...", initialPath, validFileTypePattern);

		if (fileSelector.browseForFileToOpen())
		{
			openFile(fileSelector.getResult().getFullPathName().toStdString());
		}
	}

	void SourceProjectManager::setTitle()
	{
		for (auto listener : listeners)
			listener->documentChangedName(sourceFile.getPath().filename().string());
	}


	void SourceProjectManager::onBreakpointsChanged(const std::set<int>& newBreakpoints)
	{
		breakpoints = newBreakpoints;
		controller.onBreakpointsChanged(breakpoints);
	}

	void SourceProjectManager::newDocument()
	{
		const auto newPath = fs::path("untitled").replace_extension(defaultLanguageExtension);

		doc->clearUndoHistory();
		doc->replaceAllContent("");
		doc->setSavePoint();

		sourceFile = SourceFile::asNonExisting(newPath);

		checkDirtynessState();
		setTitle();
	}


	std::unique_ptr<ProjectEx> SourceProjectManager::createProject()
	{
		/*
		Implement a proper interface instead of passing shitty c strings around.
		Consider the implementation of this.
		*/
		std::unique_ptr<ProjectEx> project;

		project = std::make_unique<ProjectEx>();
		project->files = nullptr;
		project->nFiles = 1;
		project->uniqueID = (unsigned)-1;
		project->isSingleString = true;
		std::string text;
		// Copy strings here. We have to do it this tedious way to stay c-compatible.
		// only FreeProjectStruct is supposed to free this stuff.
		// copy document text

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

		auto projectRoot = sourceFile.isActualFile() ? sourceFile.getDirectory() : homeDirectory;

		assignCStr(projectRoot.string(), project->workingDirectory);
		assignCStr(sourceFile.getPath().string(), fileLocations[0]);

		if (getDocumentText(text) && text.length() != 0 && 
			assignCStr(text, project->sourceString) &&
			assignCStr(sourceFile.getPath().stem().string(), project->projectName) &&
			assignCStr(cpl::Misc::DirectoryPath(), project->rootPath) &&
			assignCStr(getCurrentLanguageID(), project->languageID))
		{
			if (enableScopePoints)
			{
				auto copiedLines = new int[breakpoints.size()];
				std::size_t counter = 0;
				for (auto line : breakpoints)
					copiedLines[counter++] = line;

				project->traceLines = copiedLines;
				project->numTraceLines = counter;

				project->state = CodeState::None;
			}

			return project;
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

	std::string SourceProjectManager::getCurrentLanguageID()
	{
		auto ext = sourceFile.getExtension();

		return ext.length() ? ext : defaultLanguageExtension;
	}

	bool SourceProjectManager::isDirty()
	{
		return doc->hasChangedSinceSavePoint();
	}

	const SourceFile& SourceProjectManager::getSourceFile()
	{
		return sourceFile;
	}

    juce::ApplicationCommandManager & SourceProjectManager::getCommandManager()
    {
        return appCM;
    }

	void SourceProjectManager::fileChanged(const SourceFile& file)
	{
		auto stream = juce::FileInputStream(file.getJuceFile());

		if (!stream.openedOk())
		{
			controller.getConsole().printLine(CConsole::Error, "Failed to read %s for hot-reloading changed document...", file.getPath().string().c_str());
			return;
		}

		doc->loadFromStream(stream);

		controller.recompile();
	}

	cpl::Misc::MsgButton SourceProjectManager::saveIfUnsure()
	{
		using namespace cpl::Misc;
		if (isDirty()) 
		{
			std::string msg("Save changes to \"");
			msg += sourceFile.getPath().string();
			msg += "\"?";
			const auto decision = (cpl::Misc::MsgButton)MsgBox(msg, cpl::programInfo.name, MsgStyle::sYesNoCancel | MsgIcon::iQuestion, getParentWindow(), true);
			if (decision == MsgButton::bYes) 
			{
				if (sourceFile.isActualFile())
					saveCurrentFile();
				else
					saveAs();
			}
			return decision;
		}
		return MsgButton::bYes;
	}

	bool SourceProjectManager::saveCurrentFile() 
	{
		return sourceFile.isActualFile() ? doSaveFile(sourceFile.getPath()) : saveAs();
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

        RecentFilesManager::get().addFile(f);
		doc->loadFromStream(s);
		doc->setSavePoint();
		checkDirtynessState();

		sourceFile = { fileName };

		setTitle();

		return true;
	}

	bool SourceProjectManager::saveAs() 
	{
		juce::File suggestedPath;
		
		if (sourceFile.isActualFile())
		{
			suggestedPath = sourceFile.getPath().string();
		}
		else
		{
			suggestedPath = (homeDirectory / sourceFile.getPath()).string();
		}
		
		juce::FileChooser fileSelector(cpl::programInfo.programAbbr + " :: Select where to save your file...", suggestedPath, validFileTypePattern);
		if (fileSelector.browseForFileToSave(true))
		{
			fs::path newPath = fileSelector.getResult().getFullPathName().toStdString();

			if (!newPath.has_extension())
				newPath.replace_extension(defaultLanguageExtension);

			return doSaveFile(newPath);
		}

		return false;
	}

	bool SourceProjectManager::doSaveFile(const fs::path& fileName)
	{
		using namespace cpl::Misc;

		std::ofstream file(fileName.string().c_str(), std::ios::binary);

		if (file)
		{
			std::string data;
			getDocumentText(data);

			if (!file.write(data.data(), data.size()))
			{
				std::stringstream fmt;
				fmt << "Error saving to file \"" << fileName.string() << "\".";
				MsgBox(fmt.str(), cpl::programInfo.name, MsgStyle::sOk | MsgIcon::iStop, getParentWindow());
			}
			else
			{
				sourceFile = { fileName };
				doc->setSavePoint();
				checkDirtynessState();
				setTitle();
				return true;
			}
		}
		else 
		{
			std::stringstream fmt;
			fmt << "Could not open file \"" << fileName.string() << "\" for saving.";
			MsgBox(fmt.str(), cpl::programInfo.name, MsgStyle::sOk | MsgIcon::iStop, getParentWindow());
		}

		return false;
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

	void SourceProjectManager::cacheValidFileTypes(const Settings& s)
	{
		validFileTypes.clear();

		try
		{
			// look up languages part
			const auto& langs = s.root()["languages"];
			// check if theres anything there (and if its a group)
			if (!langs.isGroup())
			{
				controller.getConsole().printLine(CConsole::Warning,
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
					if (!langs[x].isGroup())
                        continue;
					
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
								validFileTypes.emplace_back(exts[y].c_str());
							}
						}
					}
					catch (const std::exception & e)
					{
						controller.getConsole().printLine(CConsole::Warning,
							"[Editor] Warning: language %s has no defined extensions in config. "
							"Program will not be able to open any files for that language. (%s)",
							langs[x].getName(), e.what());
					}
				}

			}

		}
		catch (const std::exception & e)
		{
			controller.getConsole().printLine(CConsole::Error,
				"[Editor] Error parsing file type extensions in config (%s).", e.what());
			return;
		}

		validFileTypePattern = defaultLanguageExtension.size() ? ("*." + defaultLanguageExtension + ";") : "";

		for (auto& str : validFileTypes)
			if (str != defaultLanguageExtension)
				validFileTypePattern += "*." + str + ";";

	}

	bool SourceProjectManager::openTemplate()
	{
		if (!fs::exists(templateFile))
			return false;

		if (!openFile(templateFile))
			return false;

		sourceFile = sourceFile.asNonExisting();

		return true;
	}

	void SourceProjectManager::openHomeDirectory()
	{
		juce::File{ homeDirectory.string() }.revealToUser();
	}

	void SourceProjectManager::editExternally()
	{
		saveCurrentFile();

		sourceFile.addListener(*this);

		if (editCommandLine)
		{
			try
			{
				std::string cmdLine = *editCommandLine;
				
#ifdef CPL_MAC
				if(cmdLine.empty())
					cmdLine = "open";
#endif
				
				cpl::Process::Builder command(cmdLine);
				cpl::Args args;

				args.arg(sourceFile.getPath().string(), cpl::Args::Flags::Escaped);

				command.shell(args, 0, cpl::Process::ScopeExitOperation::Detach);
			}
			catch (const std::system_error& e)
			{
				controller.getConsole().printLine(CConsole::Error, "System exception launching \"%s\" for editing \"%s\" externally: %s", (*editCommandLine).c_str(), sourceFile.getPath().string().c_str(), e.what());
			}
			catch (const std::runtime_error& e)
			{
				controller.getConsole().printLine(CConsole::Error, "Exception launching \"%s\" for editing \"%s\"  externally: %s", (*editCommandLine).c_str(), sourceFile.getPath().string().c_str(), e.what());
			}
		}
	}

}

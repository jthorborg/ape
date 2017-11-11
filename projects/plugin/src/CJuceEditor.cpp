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
 
	 file:CJuceEditor.cpp
	 
		Implementation of the juce editor
 
 *************************************************************************************/


#include "CJuceEditor.h"
#include <cpl/misc.h>
#include "Engine.h"
#include "GraphicUI.h"
#include "CConsole.h"
#include "ProjectEx.h"
#include <ctime>
#include <cstdio>
#include <sstream>

namespace APE
{
	std::unique_ptr<CCodeEditor> MakeCodeEditor(Engine * e)
	{
		return std::make_unique<CJuceEditor>(e);
	}

	/*********************************************************************************************

		Tokeniser implementation

	*********************************************************************************************/
	int CTokeniser::readNextToken(juce::CodeDocument::Iterator& source)
	{
		return juce::CppTokeniserFunctions::readNextToken(source);
	}
	/*******************************************************************************/
	juce::CodeEditorComponent::ColourScheme CTokeniser::getDefaultColourScheme()
	{
		struct Type
		{
			const char* name;
			CColour colour;
		};

		const Type types[] =
		{
			{ "Error", CColours::darkred },
			{ "Comment", CColours::green },
			{ "Keyword", CColours::blue },
			{ "Operator", CColours::darkred },
			{ "Identifier", CColours::black },
			{ "Integer", CColours::black },
			{ "Float", CColours::black },
			{ "String", CColours::grey },
			{ "Bracket", CColours::darkred },
			{ "Punctuation", CColours::darkred },
			{ "Preprocessor Text", CColours::darkolivegreen }
		};

		juce::CodeEditorComponent::ColourScheme cs;

		for (unsigned int i = 0; i < sizeof (types) / sizeof (types[0]); ++i)  // (NB: numElementsInArray doesn't work here in GCC4.2)
			cs.set(types[i].name, CColour(types[i].colour));

		return cs;
	}
	/*********************************************************************************************
	 
		Whether the token is a reserved keyword
	 
	 *********************************************************************************************/
	bool CTokeniser::isReservedKeyword(const juce::String& token) noexcept
	{
		return juce::CppTokeniserFunctions::isReservedKeyword(token.getCharPointer(), token.length());
	}

	/*********************************************************************************************

		Menu implementations

	*********************************************************************************************/
	#ifdef __MAC__
		#define CTRLCOMMANDKEY juce::ModifierKeys::Flags::commandModifier
	#else
		#define CTRLCOMMANDKEY juce::ModifierKeys::Flags::ctrlModifier
	#endif

	const MenuEntry CommandTable[][6] =
	{
		// File
		{
			{"",0,0,Command::InvalidCommand}, // dummy element - commands are 1-based index cause of juce
			{ "New File",	'n', CTRLCOMMANDKEY, Command::FileNew },
			{ "Open...",	'o', CTRLCOMMANDKEY, Command::FileOpen },
			{ "Save",		's', CTRLCOMMANDKEY,Command::FileSave },
			{ "Save As...", 0, 0, Command::FileSaveAs },
			{ "Exit", 0, 0, Command::FileExit}
		},
		// Edit

	};
	/*********************************************************************************************

		Window implementations - constructor

	*********************************************************************************************/
	CWindow::CWindow(juce::CodeDocument & cd)
		: DocumentWindow(cpl::programInfo.name + " editor",
		CColours::white,
		DocumentWindow::TitleBarButtons::allButtons),
		cec(nullptr)
	{

		cec = new juce::CodeEditorComponent(cd, &tokeniser);
		cec->setVisible(true);
		setMenuBar(this);
		setResizable(true, true);
		setBounds(100, 100, 400, 400);
		setUsingNativeTitleBar(true);
		setContentNonOwned(cec,false);
	}
	/*********************************************************************************************
		
		Destructor of CWindow
	 
	 *********************************************************************************************/
	CWindow::~CWindow()
	{
		setMenuBar(nullptr);
		// dont delete menu - it is owned by DocumentWindow
		if (cec)
			delete cec;
	}
	/*********************************************************************************************
	 
		Returns a popupmenu with entries from our applicationCommandManager
	 
	 *********************************************************************************************/
	juce::PopupMenu CWindow::getMenuForIndex(int topLevelMenuIndex, const juce::String & menuName)
	{
		juce::PopupMenu ret;

		switch (topLevelMenuIndex)
		{
			case Menus::File:
			{
				for (int i = Command::Start; i < Command::End; ++i)
					ret.addCommandItem(appCM, i);
				break;
			}
			case Menus::Edit:
			{

				break;
			}
		}
		return ret;
	}
	/*********************************************************************************************
	 
		This is unused: use CJuceEditor::perform instead (called from applicationCommandManager)
	 
	 *********************************************************************************************/
	void CWindow::menuItemSelected(int menuItemID, int topLevelMenuIndex)
	{
	}
	/*********************************************************************************************
	 
		Resizes our contents of the documentwindow
	 
	 *********************************************************************************************/
	void CWindow::resized()
	{
		/*
			temporary code: when the switch is done to setContentOwned() (see CWindow::CWindow),
			resize() shouldn't be overloaded anymore - ResizableWindow automagically resizes child
			components.
		*/
		// call our resizing firstly
		DocumentWindow::resized();
		// resize the code editor
		/*CRect bounds = getBounds();
		bounds.setX(0);
		bounds.setY(0);
		// offset the menu size
		// the codeeditorcomponent clips the first line no matter what (for some reason)
		bounds.setTop((getMenuBarComponent() ? getMenuBarComponent()->getHeight() : 0) + getTitleBarHeight() );
		// for some reason, the parent is 2 pixels narrower than what it states. account for this
		bounds.setHeight(bounds.getHeight() - 1);
		// resize code editor
		cec->setBounds(bounds);*/
	}
	/*********************************************************************************************
	
		Returns an array of the menu bar names
	 
	 *********************************************************************************************/
	juce::StringArray CWindow::getMenuBarNames()
	{
		juce::StringArray ret;
		ret.add("File");
		//ret.add("Edit");
		return ret;
	}
	/*********************************************************************************************
	 
		Hides our window when the user clicks 'x'
	 
	 *********************************************************************************************/
	void CWindow::closeButtonPressed()
	{
		setVisible(false);
	}
	/*********************************************************************************************
	 
		Attaches an application command manager to our window, and adds a keylistener
	 
	 *********************************************************************************************/
	void CWindow::setAppCM(juce::ApplicationCommandManager * acm)
	{
		// set the application command manager that is associated with this window
		appCM = acm;
		if (appCM)
			addKeyListener(appCM->getKeyMappings());
	}
	/*********************************************************************************************

		Implementation for the juceeditor constructor

	*********************************************************************************************/
	CJuceEditor::CJuceEditor(Engine * e)
		: CCodeEditor(e), window(nullptr), isInitialized(false), isSingleFile(true), fullPath("Untitled"),
		appName(cpl::programInfo.programAbbr + " Editor"), isActualFile(false), wasRestored(false), autoSaveChecked(false)
	{
		doc.setSavePoint();
	}
	/*********************************************************************************************
	 
		Destructor. Checks if user wants to save file
	 
	 *********************************************************************************************/
	CJuceEditor::~CJuceEditor()
	{
		saveIfUnsure();
		if (isInitialized && window)
			delete window;
		// if we successfully close (like now), we delete autosave state
		autoSaveFile.remove();
	}
	/*********************************************************************************************
	 
		Initializes our editor.
		Opens the default file.
	 
	 *********************************************************************************************/
	bool CJuceEditor::initEditor()
	{
		if (!isInitialized)
		{
			window = new CWindow(doc);
			if (window)
			{
				window->setSize(800, 900);
				loadHotkeys();
				window->setAppCM(&appCM);

				/*
					open a default file from settings
				*/
				std::string file;
				try
				{
					auto & root = engine->getRootSettings();
					root["languages"].lookupValue("default_file", file);
				}
				catch (const std::exception & e)
				{
					engine->getGraphicUI()->console->printLine(CColours::red, "Error reading default file from config... %s", e.what());
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
	/*********************************************************************************************
	 
		Prompts the user to open a file
	 
	 *********************************************************************************************/
	void CJuceEditor::openAFile()
	{
		// array of types. these are read from the config and defines what sources
		// the program can open
		std::vector<std::string> filetypes;

		try
		{
			// get root settings
			libconfig::Setting & stts = engine->getRootSettings();
			// look up languages part
			auto & langs = stts["languages"];
			// check if theres anything there (and if its a group)
			if (!langs.isGroup())
			{
				engine->getGraphicUI()->console->printLine(CColours::red,
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
					// get name of current language
					std::string name = langs[x].getName();
					if (langs[x].isGroup()) {
						// look up list of extensions
						try
						{
							libconfig::Setting & exts = langs[x]["extensions"];
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
							engine->getGraphicUI()->console->printLine(CColours::red,
								"[Editor] Warning: language %s has no defined extensions in config. "
								"Program will not be able to open any files for that language. (%s)",
								name.c_str(), e.what());
						}
					}
				}

			}

		}
		catch (const std::exception & e)
		{
			engine->getGraphicUI()->console->printLine(CColours::red,
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
	/*********************************************************************************************
	 
		Sets the title of our window
	 
	 *********************************************************************************************/
	void CJuceEditor::setTitle()
	{
		std::string title;
		title += appName + " (" + std::to_string(engine->instanceCounter()) + ")";
		if (isDirty())
			title += " * ";
		else
			title += " - ";
		title += fullPath;
		if (window)
			window->setName(title);
	}
	/*********************************************************************************************
	 
		Ensures our editor is opened, and shows it if initialVisibility is true
	 
	 *********************************************************************************************/
	bool CJuceEditor::openEditor(bool initialVisibility)
	{
		if (!isInitialized) {

			if (!initEditor())
				return false;
			isInitialized = true;
			// on first open, we check autosave
			checkAutoSave();
		}
		window->setVisible(initialVisibility);
		window->setMinimised(false);
		// extremely buggy:
		//window->toFront(true);
		return true;
	}
	/*********************************************************************************************
	 
		Returns the path of the current document
	 
	 *********************************************************************************************/
	std::string CJuceEditor::getDocumentPath()
	{
		return fullPath;
	}
	/*********************************************************************************************
	 
		Hides our editor
	 
	 *********************************************************************************************/
	bool CJuceEditor::closeEditor()
	{
		if (isInitialized)
		{
			window->setVisible(false);
			return true;
		}
		else
			return false;
	}
	/*********************************************************************************************
	 
		Returns info about commands
	 
	 *********************************************************************************************/
	void CJuceEditor::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo & result)
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
	/*********************************************************************************************
	 
		Returns all the commands our instance support
	 
	 *********************************************************************************************/
	void CJuceEditor::getAllCommands(juce::Array<juce::CommandID> & commands)
	{
		for (int c = Command::FileNew; c < Command::End; c++ /* lol c++ */)
		{
			commands.add(c);
		}
	}
	/*********************************************************************************************
	 
		Sets an empty document. Do not call directly!
	 
	 *********************************************************************************************/
	void CJuceEditor::newDocument()
	{
		fullPath = "Untitled";
		doc.clearUndoHistory();
		doc.replaceAllContent("");
		doc.setSavePoint();
		setTitle();
		isActualFile = false;
	}
	/*********************************************************************************************
	 
		Eventhandler
	 
	 *********************************************************************************************/
	bool CJuceEditor::perform(const InvocationInfo & info)
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
	/*********************************************************************************************

		Returns a project struct that describes the current project. Must be deallocated with
		APE::FreeProjectStruct() when done with it.

	*********************************************************************************************/
	ProjectEx * CJuceEditor::getProject()
	{
		/*
		Implement a proper interface instead of passing shitty c strings around.
		Consider the implementation of this.
		*/
		ProjectEx * project = nullptr;
		// for now we only support single files. 
		// we set the appropiate values in the project struct and fill it.
		if (isSingleFile) 
		{
			project = APE::CreateProjectStruct();
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
				project->state = CodeState::None;
				return project;
			}
		}
		// some error occured.
		APE::FreeProjectStruct(project);
		return nullptr;
	}
	/*********************************************************************************************
	 
		Marks an line as an error
	 
	 *********************************************************************************************/
	void CJuceEditor::setErrorLine(int line)
	{


	}
	/*********************************************************************************************
	 
		Appends all of the document text into the buffer
	 
	 *********************************************************************************************/
	bool CJuceEditor::getDocumentText(std::string & buffer)
	{
		buffer.append( doc.getAllContent().toRawUTF8());
		return true;
	}
	/*********************************************************************************************

		Returns the document name.

	*********************************************************************************************/
	std::string CJuceEditor::getDocumentName() {
		for (signed int i = static_cast<signed int>(fullPath.length()); 
			i >= 0; 
			--i)
		{
			if (DIRC_COMP(fullPath[i])) // if fullPath is a path and not an unsaved file, shave the directory off
				// by looping backwards and finding the first backslash.
				return fullPath.substr(i + 1);
		}
		return fullPath;
	}
	/*********************************************************************************************

		Returns the directory the document reside in.

	*********************************************************************************************/
	std::string CJuceEditor::getDirectory() {
		//NOT DONE here. Pull the info from the project info file, if not exists - from file alone.")
		for (signed int i = fullPath.length(); i >= 0; --i) {
			if (DIRC_COMP(fullPath[i])) // if fullPath is a path and not an unsaved file, shave the file off
				// by looping backwards and finding the first backslash.
				return std::string(fullPath.begin(), fullPath.begin() + i);
		}
		return "";
	}
	/*********************************************************************************************
	 
		Tries to load the hotkeys from the config file
	 
	 *********************************************************************************************/
	bool CJuceEditor::loadHotkeys()
	{
		/*
		 try to read the hotkeys from editor {}
		 */
		try
		{
			std::string temp;
			auto & root = engine->getRootSettings();
			if(root["editor"].lookupValue("hkey_save", temp))
				userHotKeys[Command::FileSave] = temp;
			if(root["editor"].lookupValue("hkey_new", temp))
				userHotKeys[Command::FileNew] = temp;
			if(root["editor"].lookupValue("hkey_open", temp))
				userHotKeys[Command::FileOpen] = temp;
		}
		catch (const std::exception & e)
		{
			engine->getGraphicUI()->console->printLine(CColours::red,
													   "[Editor] : Error reading editor hotkeys from config... %s", e.what());
			return false;
		}
		
		appCM.registerAllCommandsForTarget(this);
		appCM.setFirstCommandTarget(this);
		
		return true;
	}
	/*********************************************************************************************

		Returns the directory the document reside in.

	*********************************************************************************************/
	std::string CJuceEditor::getExtension()
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
	/*********************************************************************************************

		Returns the name of the project

	*********************************************************************************************/
	std::string CJuceEditor::getProjectName()
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
	/*********************************************************************************************
	 
		Whether the document contains unsaved changes
	 
	 *********************************************************************************************/
	bool CJuceEditor::isDirty()
	{
		return doc.hasChangedSinceSavePoint();
	}
	/*********************************************************************************************
	 
		Asks the user if he wants to save the document if it is marked as unsaved.
	 
	 *********************************************************************************************/
	int CJuceEditor::saveIfUnsure()
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
	/*********************************************************************************************

		Starts a saving sequence

	*********************************************************************************************/
	void CJuceEditor::saveCurrentFile() {
		if (isActualFile)
			doSaveFile(fullPath);
		else
			saveAs();
	}
	/*********************************************************************************************

		Opens a file with the filename given

	*********************************************************************************************/
	bool CJuceEditor::openFile(const std::string & fileName)
	{

		fullPath = fileName;
		juce::File f(fullPath);
		juce::FileInputStream s(f);
		if (s.openedOk()) {
			setTitle();
			doc.replaceAllContent("");
			auto cec = window->getCodeEditor();
			if (cec)
			{
				// juce can crash if the window is scrolled further than the new files content. lol
				//cec->scrollToLine(0);
			}
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
	/*********************************************************************************************

		Save as a new file

	*********************************************************************************************/
	void CJuceEditor::saveAs() {
		 
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
	/*********************************************************************************************
		
		Saves contents into fileName and sets editor to use fileName as new file

	*********************************************************************************************/
	void CJuceEditor::doSaveFile(const std::string & fileName)
	{
		fullPath = fileName;
		FILE *fp = nullptr;
		#ifdef __MSVC__
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
	/*********************************************************************************************
	 
		The structure of an autosave file
	 
	 *********************************************************************************************/
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

	/*********************************************************************************************
	 
		Does an autosave.
	 
	 *********************************************************************************************/
	void CJuceEditor::autoSave()
	{
		// only create an autosave file if current contents are unsaved
		if (isDirty())
		{
			// close the currently open file
			if(!autoSaveFile.isOpened())
			{
				std::string path = cpl::Misc::DirectoryPath() + "/logs/autosave"
				+ std::to_string(engine->uniqueInstanceID()) + ".ape";
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
				engine->getGraphicUI()->console->printLine(CColours::red,
													"[Editor] : error opening file %s for autosave.", autoSaveFile.getName().c_str());
			}
			std::free(info);
			engine->getGraphicUI()->setStatusText("Autosaved...", CColours::lightgoldenrodyellow, 2000);
		}
	}
	/*********************************************************************************************
	 
		Returns a parent window, if any.
	 
	 *********************************************************************************************/
	void * CJuceEditor::getParentWindow()
	{
		if (isInitialized && window && window->isVisible())
			return window->getWindowHandle();
		return engine->getGraphicUI()->getSystemWindow();
	}
	/*********************************************************************************************
	 
		Asks user if he wants to restore a state from an autosave file
	 
	 *********************************************************************************************/
	bool CJuceEditor::restoreAutoSave(AutoSaveInfo * info, juce::File & parentFile)
	{
		using namespace cpl::Misc;
		bool ret = false; // whether we actually restored a state
		if (!info->wasDirty)
			return false;
		std::stringstream fmt;

		time_t timeObj = info->timeStamp;
		tm * ctime;
		#ifdef __MSVC__
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

		std::string fileName = fullFileName;

		for (signed int i = static_cast<signed int>(fullFileName.length());
			i >= 0;
			--i)
		{
			if (DIRC_COMP(fullFileName[i]))
			{
				fileName = fullFileName.substr(i + 1);
			}
		}


		auto answer = MsgBox(fmt.str(), "APE - Autorecover", MsgStyle::sYesNoCancel | MsgIcon::iQuestion, getParentWindow(), true);

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
	/*********************************************************************************************
	 
		Sets the content of our document
	 
	 *********************************************************************************************/
	void CJuceEditor::setContents(const juce::String & newContent)
	{
		doc.replaceAllContent(newContent);
	}
	/*********************************************************************************************
	 
		Iterates /logs/ to find any autosave files.		
	 
	 *********************************************************************************************/
	bool CJuceEditor::checkAutoSave()
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
				engine->getGraphicUI()->console->printLine(CColours::black,
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
					engine->getGraphicUI()->console->printLine(CColours::red,
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
};
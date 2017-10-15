/*************************************************************************************

Audio Programming Environment VST.

VST is a trademark of Steinberg Media Technologies GmbH.

Copyright (C) 2013 Janus Lynggaard Thorborg [LightBridge Studios]

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

file:SciLexer.cpp

Implementation of SciLexer.h

*************************************************************************************/

#include "SciLexer.h"
#include <sstream>
#include "EditorMenu.h"
#include "Misc.h"
#include "APE.h"
#include "Project.h"
#include "CConsole.h"
#include <string>
#include "GraphicUI.h"
#include "MacroConstants.h"
#include "PlatformSpecific.h"
#include "Misc.h"

namespace Scintilla {

	/*
	Map of editors. This is used in window callbacks
	to associate window handles with instances of editors.
	*/
	std::map<WHandle, SciLexer * > LookUpEditors;

	static const char appName[] = _PROGRAM_NAME_ABRV " Editor";
	static const char className[] = _PROGRAM_NAME_ABRV "EditorClass";

	/// Scintilla Colors structure
	struct SScintillaColors
	{
		int         iItem;
		COLORREF    rgb;
	};

	// A few basic colors
	const COLORREF black = RGB(0, 0, 0);
	const COLORREF white = RGB(255, 255, 255);
	const COLORREF green = RGB(0, 255, 0);
	const COLORREF red = RGB(255, 0, 0);
	const COLORREF blue = RGB(0, 0, 255);
	const COLORREF yellow = RGB(255, 255, 0);
	const COLORREF magenta = RGB(255, 0, 255);
	const COLORREF cyan = RGB(0, 255, 255);
	const COLORREF yellowGreenish = RGB(150, 159, 36);
	const COLORREF offWhite = RGB(0xFF, 0xFB, 0xF0);
	const COLORREF darkGreen = RGB(0, 0x80, 0);
	const COLORREF darkBlue = RGB(0, 0, 0x80);
	const COLORREF lightBlue = RGB(0, 0, 0xFF);
	const COLORREF lightGrey = RGB(190, 190, 190);


	/// Default color scheme
	static SScintillaColors g_rgbSyntaxCpp[] =
	{
		{ SCE_C_COMMENT, darkGreen },
		{ SCE_C_COMMENTLINE, darkGreen },
		{ SCE_C_COMMENTDOC, darkGreen },
		{ SCE_C_NUMBER, magenta },
		{ SCE_C_STRING, red },
		{ SCE_C_CHARACTER, red },
		{ SCE_C_UUID, cyan },
		{ SCE_C_OPERATOR, red },
		{ SCE_C_PREPROCESSOR, yellowGreenish },
		{ SCE_C_WORD, blue },
		{ SCE_C_DEFAULT, lightBlue },
		{ -1, 0 }
	};

	/*********************************************************************************************

	Constructor

	*********************************************************************************************/
	SciLexer::SciLexer(APE::Engine * e)
		: CCodeEditor(e), isInitialized(false), isActualFile(false), isSingleFile(true), isDirty(false),
		hInstance(nullptr), currentDialog(nullptr), wMain(nullptr), wEditor(nullptr), fullPath("Untitled"),
		isVisible(false), sciFunc(nullptr), sciData(0)
	{
	}
	/*********************************************************************************************

	Destructor

	*********************************************************************************************/
	SciLexer::~SciLexer()
	{
		SaveIfUnsure();
		quit();
	}
	/*********************************************************************************************

	Sends a message to the editor.

	*********************************************************************************************/
	OSRes SciLexer::sendEditor(unsigned int msg, UPtr param1, LPtr param2)
	{
		if (sciFunc && sciData)
			return sciFunc(sciData, msg, param1, param2);
		else
			#ifdef __WINDOWS__
				return ::SendMessage(static_cast<HWND>(wEditor), msg, param1, param2);
			#else
				#error NI
			#endif
	}
	/*********************************************************************************************

	Returns a project struct that describes the current project. Must be deallocated with
	APE::FreeProjectStruct() when done with it.

	*********************************************************************************************/
	APE::CProject * SciLexer::getProject()
	{
		/*
		Implement a proper interface instead of passing shitty c strings around.
		Consider the implementation of this.
		*/
		APE::CProject * project = nullptr;
		// for now we only support single files. 
		// we set the appropiate values in the project struct and fill it.
		if (isSingleFile) {
			project = APE::CreateProjectStruct();
			project->files = nullptr;
			project->nFiles = 0;
			project->uniqueID = (unsigned)-1;
			project->isSingleString = true;
			std::string tstring;
			// Copy strings here. We have to do it this tedious way to stay c-compatible.
			// only FreeProjectStruct is supposed to free this stuff.
			// copy document text
			getDocumentText(tstring);
			auto len = tstring.length();
			if (len > 0) {
				project->sourceString = new APE::char_t[len + 1];
				std::copy(tstring.begin(), tstring.end(), project->sourceString);
				project->sourceString[len] = '\0';
				// copy project name
				tstring = getProjectName();
				len = tstring.length();
				if (len > 0) {
					project->projectName = new APE::char_t[len + 1];
					std::copy(tstring.begin(), tstring.end(), project->projectName);
					project->projectName[len] = '\0';
					// copy path of program
					tstring = APE::Misc::DirectoryPath;
					len = tstring.length();
					if (len > 0) {
						project->rootPath = new APE::char_t[len + 1];
						std::copy(tstring.begin(), tstring.end(), project->rootPath);
						project->rootPath[len] = '\0';
						// copy extension
						tstring = getExtension();
						len = tstring.length();
						if (len > 0) {
							project->languageID = new APE::char_t[len + 1];
							std::copy(tstring.begin(), tstring.end(), project->languageID);
							project->languageID[len] = '\0';

							// copy filenames
							tstring = fullPath;
							len = tstring.length();
							if (len > 0) {
								project->files = new APE::char_t*[5];
								project->files[0] = new APE::char_t[len + 1];
								std::copy(tstring.begin(), tstring.end(), project->files[0]);
								project->files[0][len] = '\0';
								project->nFiles = 1;
								project->state = APE::CodeState::None;
								return project;
							}
						}

					}
				}

			}
		}
		// some error occured.
		APE::FreeProjectStruct(project);
		return nullptr;
	}
	/*********************************************************************************************

	Returns the document name.

	*********************************************************************************************/
	std::string SciLexer::getDocumentName() {
		for (int i = fullPath.length(); i >= 0; --i) {
			if (DIRC_COMP(fullPath[i])) // if fullPath is a path and not an unsaved file, shave the directory off
				// by looping backwards and finding the first backslash.
				return fullPath.substr(i + 1);
		}
		return fullPath;
	}
	/*********************************************************************************************

	Returns the directory the document reside in.

	*********************************************************************************************/
	std::string SciLexer::getDirectory() {
		//NOT DONE here. Pull the info from the project info file, if not exists - from file alone.")
		for (int i = fullPath.length(); i >= 0; --i) {
			if (DIRC_COMP(fullPath[i])) // if fullPath is a path and not an unsaved file, shave the file off
				// by looping backwards and finding the first backslash.
				return std::string(fullPath.begin(), fullPath.begin() + i);
		}
		return "";
	}
	/*********************************************************************************************

	Returns the directory the document reside in.

	*********************************************************************************************/
	std::string SciLexer::getExtension()
	{
		for (int i = fullPath.length(); i >= 0; --i) {
			if (fullPath[i] == '.') // if fullPath is a path and not an unsaved file, shave the path off
				// by looping backwards and finding the first dot.
				return fullPath.substr(i + 1);
		}
		return "";
	}
	/*********************************************************************************************

	Fills the buffer with the text in the document

	*********************************************************************************************/
	bool SciLexer::getDocumentText(std::string & buffer)
	{
		if (isInitialized) {
			sendEditor(SCI_SETREADONLY, true);
			const char * txt = reinterpret_cast<const char*>(sendEditor(SCI_GETCHARACTERPOINTER));
			if (txt) {
				buffer.append(txt);
				sendEditor(SCI_SETREADONLY, false);
				return true;
			}
		}
		return false;
	}
	/*********************************************************************************************

	Returns the name of the project

	*********************************************************************************************/
	std::string SciLexer::getProjectName()
	{
		if (isInitialized) {
			if (isSingleFile) {
				// fix this obviously when we start using project files.
				std::string ret = getDocumentName();
				int end = ret.size();
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

	Opens the editor. Initializes it, if it hasn't been done yet.

	*********************************************************************************************/
	bool SciLexer::openEditor()
	{
		if (!isInitialized) {

			initEditor();
			isInitialized = true;
			sciFunc = (SciFnDirect)sendEditor( SCI_GETDIRECTFUNCTION, 0, 0);
			sciData = (sptr_t)sendEditor(SCI_GETDIRECTPOINTER, 0, 0);

		}
		this->show();
		return true;
	}
	/*********************************************************************************************

	Closes (hides) the editor

	*********************************************************************************************/
	bool SciLexer::closeEditor()
	{
		this->hide();
		return true;
	}
	/*********************************************************************************************

	Hides the editor.

	*********************************************************************************************/
	void SciLexer::hide()
	{
		#ifdef __WINDOWS__
			::ShowWindow(static_cast<HWND>(this->wMain), SW_HIDE);
		#else
			#error NI
		#endif
		isVisible = false;
	}
	/*********************************************************************************************

	Shows the editor

	*********************************************************************************************/
	void SciLexer::show()
	{
		#ifdef __WINDOWS__
			::ShowWindow(static_cast<HWND>(this->wMain), SW_RESTORE);
			::SetFocus(static_cast<HWND>(this->wEditor));
		#else
			#error NI
		#endif
		isVisible = true;
	}
	/*********************************************************************************************

	Get range of text

	*********************************************************************************************/
	void SciLexer::GetRange(int start, int end, char *text)
	{
		Sci_TextRange tr;
		tr.chrg.cpMin = start;
		tr.chrg.cpMax = end;
		tr.lpstrText = text;
		//EM_GETTEXTRANGE
		sendEditor(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPtr>(&tr));
	}
	/*********************************************************************************************

	Sets the title of the editor

	*********************************************************************************************/
	void SciLexer::SetTitle()
	{
		std::string title;
		title += appName;
		if (isDirty)
			title += " * ";
		else
			title += " - ";
		title += fullPath;
#ifdef __WINDOWS__
		::SetWindowText(static_cast<HWND>(wMain), title.c_str());
#else
#error NI
#endif
	}
	/*********************************************************************************************

	Clears the document. Do not call this directly (what about save?)

	*********************************************************************************************/
	void SciLexer::New()
	{
		sendEditor(SCI_CLEARALL);
		sendEditor(EM_EMPTYUNDOBUFFER);
		fullPath.clear();
		SetTitle();
		isDirty = false;
		sendEditor(SCI_SETSAVEPOINT);
	}
	/*********************************************************************************************

	Opens a file with the filename given

	*********************************************************************************************/
	bool SciLexer::openFile(const std::string & fileName)
	{
		New();
		sendEditor(SCI_CANCEL);
		sendEditor(SCI_SETUNDOCOLLECTION, 0);

		fullPath = fileName;
		FILE *fp = fopen(fullPath.c_str(), "rb");
		if (fp) {
			SetTitle();
			char data[blockSize];
			int lenFile = fread(data, 1, sizeof(data), fp);
			while (lenFile > 0) {
				sendEditor(SCI_ADDTEXT, lenFile,
					reinterpret_cast<LPtr>(static_cast<char *>(data)));
				lenFile = fread(data, 1, sizeof(data), fp);
			}
			fclose(fp);
		}
		else {
			std::string msg("Could not open file \"");
			msg += fullPath;
			msg += "\".";
			APE::Misc::MsgBox(msg, appName, APE::Misc::MsgStyle::sOk, wMain);
			return false;
		}
		sendEditor(SCI_SETUNDOCOLLECTION, 1);
#ifdef __WINDOWS__
		::SetFocus(static_cast<HWND>(wEditor));
#else
#error NI
#endif
		sendEditor(SCI_EMPTYUNDOBUFFER);
		sendEditor(SCI_SETSAVEPOINT);
		sendEditor(SCI_GOTOPOS, 0);
		setEditorStyling();
		isActualFile = true;
		return true;
	}



	/*********************************************************************************************

	Prompts the user to select a file that will be opened.
	check this out http://www.kvraudio.com/forum/viewtopic.php?t=347679
	*********************************************************************************************/
	void SciLexer::Open()
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
				engine->getGraphicUI()->console->printLine(Colours::red,
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
								throw std::exception("Not a list");
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
							engine->getGraphicUI()->console->printLine(Colours::red,
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
			engine->getGraphicUI()->console->printLine(Colours::red,
				"[Editor] Error parsing file type extensions in config (%s).", e.what());
			return;
		}

		std::string validTypes;
		for (auto & str : filetypes)
			validTypes += "*." + str + ";";

		juce::File initialPath(APE::Misc::DirectoryPath + "/examples/");
		juce::FileChooser fileSelector(_PROGRAM_NAME_ABRV " - Select a source file...", initialPath, validTypes);

		if (fileSelector.browseForFileToOpen())
		{
			openFile(fileSelector.getResult().getFullPathName().toStdString());
		}
		else
		{
			APE::Misc::CStringFormatter fmt;
			fmt << "Error opening file dialog (" << GetLastError() << ")!";
			APE::Misc::MsgBox(fmt.str(), _PROGRAM_NAME_ABRV " Error!",
				APE::Misc::MsgStyle::sOk | APE::Misc::MsgIcon::iWarning, wMain);
		}

	}
	/*********************************************************************************************

	Starts a saving sequence

	*********************************************************************************************/
	void SciLexer::Save() {
		if (isActualFile)
			SaveFile(fullPath);
		else
			SaveAs();
	}
	/*********************************************************************************************

	Save as a new file

	*********************************************************************************************/
	void SciLexer::SaveAs() {

		juce::File suggestedPath(fullPath);

		juce::FileChooser fileSelector(_PROGRAM_NAME_ABRV " :: Select where to save your file...", suggestedPath);
		if (fileSelector.browseForFileToSave(true))
		{
			fullPath = fileSelector.getResult().getFullPathName().toStdString();

			SetTitle();
			SaveFile(fullPath);
#ifdef __WINDOWS__
			::InvalidateRect(static_cast<HWND>(wEditor), 0, 0);
#else
#error NI
#endif
		}
		else
		{
			APE::Misc::CStringFormatter fmt;
			fmt << "Error opening save file dialog (" << false << ")!";
			APE::Misc::MsgBox(fmt.str(), _PROGRAM_NAME_ABRV " Error!",
				APE::Misc::MsgStyle::sOk | APE::Misc::MsgIcon::iWarning, wMain);
		}
	}
	/*********************************************************************************************

	Saves contents into fileName and sets editor to use fileName as new file

	*********************************************************************************************/
	void SciLexer::SaveFile(const std::string & fileName)
	{
		fullPath = fileName;
		FILE *fp = fopen(fullPath.c_str(), "wb");
		if (fp) {
			char data[blockSize + 1];
			int lengthDoc = sendEditor(SCI_GETLENGTH);
			for (int i = 0; i < lengthDoc; i += blockSize) {
				int grabSize = lengthDoc - i;
				if (grabSize > blockSize)
					grabSize = blockSize;
				GetRange(i, i + grabSize, data);
				fwrite(data, grabSize, 1, fp);
			}
			fclose(fp);
			sendEditor(SCI_SETSAVEPOINT);
		}
		else {
			using namespace APE::Misc;
			CStringFormatter fmt;
			fmt << "Could not save file \"" << fullPath << "\".";
			MsgBox(fmt.str(), _PROGRAM_NAME_ABRV " error!", MsgStyle::sOk | MsgIcon::iStop, wMain);
		}
	}
	/*********************************************************************************************

	Asks if user wants to save if there has been unsaved edits

	*********************************************************************************************/
	int SciLexer::SaveIfUnsure()
	{
		using namespace APE::Misc;
		if (isDirty) {
			std::string msg("Save changes to \"");
			msg += fullPath;
			msg += "\"?";
			int decision = MsgBox(msg, _PROGRAM_NAME_ABRV, MsgStyle::sYesNoCancel | MsgIcon::iQuestion, wMain, true);
			if (decision == MsgButton::bYes) {
				if (isActualFile)
					Save();
				else
					SaveAs();
			}
			return decision;
		}
		return MsgButton::bYes;
	}
	/*********************************************************************************************

	Command callback, acts upon menu choices

	*********************************************************************************************/
	void SciLexer::Command(int id)
	{
		switch (id) {
		case IDM_FILE_NEW:
			if (SaveIfUnsure() != APE::Misc::MsgButton::bYes) {
				New();
			}
			break;
		case IDM_FILE_OPEN:
			if (SaveIfUnsure() != APE::Misc::MsgButton::bCancel) {
				Open();
			}
			break;
		case IDM_FILE_SAVE:
			Save();
			break;
		case IDM_FILE_SAVEAS:
			SaveAs();
			break;
		case IDM_FILE_EXIT:
			/*
			if (SaveIfUnsure() != IDCANCEL) {
			::PostQuitMessage(0);
			}
			*/
			this->hide();
			break;

		case IDM_EDIT_UNDO:
			sendEditor(SCI_UNDO);
			break;
		case IDM_EDIT_REDO:
			sendEditor(SCI_REDO);
			break;
		case IDM_EDIT_CUT:
			sendEditor(SCI_CUT);
			break;
		case IDM_EDIT_COPY:
			sendEditor(SCI_COPY);
			break;
		case IDM_EDIT_PASTE:
			sendEditor(SCI_PASTE);
			break;
		case IDM_EDIT_DELETE:
			sendEditor(SCI_CLEAR);
			break;
		case IDM_EDIT_SELECTALL:
			sendEditor(SCI_SELECTALL);
			break;
		};
	}
	/*********************************************************************************************

	(De)activates the menuitem with the id.

	*********************************************************************************************/
	void SciLexer::EnableAMenuItem(int id, bool enable)
	{
#ifdef __WINDOWS__
		if (enable)
			::EnableMenuItem(::GetMenu(static_cast<HWND>(wMain)), id, MF_ENABLED | MF_BYCOMMAND);
		else
			::EnableMenuItem(::GetMenu(static_cast<HWND>(wMain)), id, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
#else
#error NI
#endif
	}
	/*********************************************************************************************

	Depending on state, deactivates or activates menu items.

	*********************************************************************************************/
	void SciLexer::CheckMenus()
	{
		EnableAMenuItem(IDM_FILE_SAVE, isDirty);
		EnableAMenuItem(IDM_EDIT_UNDO, !!sendEditor(SCI_CANUNDO));
		EnableAMenuItem(IDM_EDIT_REDO, !!sendEditor(SCI_CANREDO));
		EnableAMenuItem(IDM_EDIT_PASTE, !!sendEditor(SCI_CANPASTE));
	}
	/*********************************************************************************************

	Notify callback.

	*********************************************************************************************/
	void SciLexer::Notify(SCNotification *notification)
	{
		switch (notification->nmhdr.code) {
		case SCN_SAVEPOINTREACHED:
		{
									 isDirty = false;
									 SetTitle();
									 CheckMenus();
		}
			break;

		case SCN_SAVEPOINTLEFT:
			isDirty = true;
			SetTitle();
			CheckMenus();
			break;
		}
	}
	/*********************************************************************************************

	Marks an line for error and shows it.

	*********************************************************************************************/
	void SciLexer::setErrorLine(int nLine)
	{
		if (nLine != -1) {
			compiler_error.bSet = true;
			compiler_error.nError = 0;
			compiler_error.nLine = nLine;
			sendEditor(SCI_GOTOLINE, nLine);
		}

	}
	/*********************************************************************************************

	Destroys the editor window

	*********************************************************************************************/
	void SciLexer::quit()
	{
#ifdef __WINDOWS__
		DestroyWindow(static_cast<HWND>(this->wMain));
#else
#error NI
#endif
		isInitialized = false;
	}
	/*********************************************************************************************

	Sets a style in the editor

	*********************************************************************************************/
	void SciLexer::SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face)
	{
		sendEditor(SCI_STYLESETFORE, style, fore);
		sendEditor(SCI_STYLESETBACK, style, back);
		if (size >= 1)
			sendEditor(SCI_STYLESETSIZE, style, size);
		if (face)
			sendEditor(SCI_STYLESETFONT, style, reinterpret_cast<LPtr>(face));
	}
	/*********************************************************************************************

	Updates the styling of the window to the settings given in the config for the current
	language.

	*********************************************************************************************/
	void SciLexer::setEditorStyling()
	{

		/*
		collect settings from config file
		*/
		int lexer = -1;
		std::string keywords;
		try
		{
			auto & root = engine->getRootSettings();
			const std::string & lang = getExtension();
			auto & curLang = root["languages"][lang];
			if (curLang.exists("lexer")) {
				auto & stx = curLang["lexer"];
				if (!stx.lookupValue("scilex_number", lexer))
					lexer = -1;
				const char * groups[] = { "keywords", "types", "userdefined" };
				for (int i = 0; i < ArraySize(groups); ++i) {
					if (stx.exists(groups[i])) {
						auto & block = stx[groups[i]];
						if (block.isGroup() && block.exists("words")) {
							keywords += " ";
							keywords += block["words"].c_str();
						}
					}
				}
				auto & keywords = stx["keywords"];
			}
		}
		catch (const libconfig::SettingNotFoundException & e)
		{
			engine->getGraphicUI()->console->printLine(Colours::red,
				"Error reading lexer settings for language %s (%s)", getExtension().c_str(), e.getPath());

		}
		catch (const std::exception & e)
		{
			engine->getGraphicUI()->console->printLine(Colours::red,
				"Error reading lexer settings for language %s (%s)", getExtension().c_str(), e.what());

		};

		// found a lexer style?
		if (lexer != -1)
			sendEditor(SCI_SETLEXER, SCLEX_CPP);

		// Set number of style bits to use
		sendEditor(SCI_SETSTYLEBITS, 5);
		//Indent guides
		sendEditor(SCI_SETINDENTATIONGUIDES, SC_IV_LOOKBOTH);
		SetAStyle(STYLE_INDENTGUIDE, lightGrey);
		sendEditor(SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
		sendEditor(SCI_SETMARGINWIDTHN, 0, 38);
		// Set tab width
		sendEditor(SCI_SETTABWIDTH, 4);

		// set up keywords
		if (keywords.length())
			sendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<LPtr>(keywords.c_str()));
		// Set up the global default style. These attributes
		// are used wherever no explicit choAPEs are made.
		SetAStyle(STYLE_DEFAULT, black, white, 10, "Consolas");

		// Set caret foreground color
		sendEditor(SCI_SETCARETFORE, RGB(0, 0, 0));

		// Set all styles
		sendEditor(SCI_STYLECLEARALL);

		// Set selection color
		sendEditor(SCI_SETSELBACK, TRUE, lightGrey);

		// Set syntax colors
		for (long i = 0; g_rgbSyntaxCpp[i].iItem != -1; i++)
			SetAStyle(g_rgbSyntaxCpp[i].iItem, g_rgbSyntaxCpp[i].rgb);

	}
	/*********************************************************************************************

	Main window procedure.

	*********************************************************************************************/
	OSRes OSCallBack SciLexer::WndProc(WHandle hWnd, unsigned int iMessage, UPtr wParam, LPtr lParam)
	{
		WHandle hEditor;
		SciLexer * _this = nullptr;
		if (iMessage != WM_CREATE)
			// We can only know this if the window is already initialized, see WM_CREATE.
			_this = LookUpEditors[hWnd];

		//_this->effect->gui->console->printLine(kBlackCColor, "WndProc called 0x%X - %d", hWnd, iMessage);

		switch (iMessage) {

		case WM_CREATE:
			/*
			Window is being created here, and more importantly: LookUpEditors[hWnd] has not yet been defined -
			because we do not know the window handle before CreateWindowEx returns!! Therefore, a pointer to
			the Scintilla::SciLexer object is passed through lParam as a ((CREATESTRUCT*)lparam)->lpCreateParams.
			We use this value to initialize the LookUpEditors[hWnd], so we safely can retrieve the sciteditorobject
			associated with the window on laters calls to this procedure.
			*/
			LookUpEditors[hWnd] = reinterpret_cast<Scintilla::SciLexer*>(
				(reinterpret_cast<CREATESTRUCT*>(lParam))->lpCreateParams
				);
			_this = LookUpEditors[hWnd];
			hEditor = ::CreateWindow(
				"Scintilla",
				"Source",
				WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
				0, 0,
				100, 100,
				static_cast<HWND>(hWnd),
				0,
				static_cast<HMODULE>(_this->hInstance),
				0);
			_this->wEditor = hEditor;
			::ShowWindow(static_cast<HWND>(hEditor), SW_SHOW);
			::SetFocus(static_cast<HWND>(_this->wEditor));
			return 0;

		case WM_SIZE:
			if (wParam != 1) {
				RECT rc;
				::GetClientRect(static_cast<HWND>(hWnd), &rc);
				::SetWindowPos(static_cast<HWND>(_this->wEditor), 0, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
			}
			return 0;

		case WM_COMMAND:
			_this->Command(LOWORD(wParam));
			_this->CheckMenus();
			return 0;

		case WM_NOTIFY:
			_this->Notify(reinterpret_cast<SCNotification *>(lParam));
			return 0;

		case WM_MENUSELECT:
			_this->CheckMenus();
			return 0;

		case WM_CLOSE:
			//if (_this->SaveIfUnsure() != IDCANCEL) {
			ShowWindow(static_cast<HWND>(_this->wMain), SW_HIDE);
			_this->engine->getGraphicUI()->setParameter(APE::kTags::tagEditor, 0.0f);
			//::DestroyWindow(app.wEditor);
			//::PostQuitMessage(0);
			//}
			return 0;
		default:
			return DefWindowProc(static_cast<HWND>(hWnd), iMessage, wParam, lParam);
		}
	}
	/*********************************************************************************************

	Registers the window class.

	*********************************************************************************************/
	void SciLexer::registerWindowClass()
	{
#ifdef __WINDOWS__
		WNDCLASS wndclass;
		::memset(&wndclass, sizeof wndclass, 0);

		// Checking if the class already exists: if it does, no need to proceed.
		if (GetClassInfo(static_cast<HMODULE>(hInstance), className, &wndclass))
			return;

		::memset(&wndclass, sizeof wndclass, 0);
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		// cast here is because of incompabilities between handles - not an issue.
		wndclass.lpfnWndProc = reinterpret_cast<decltype(wndclass.lpfnWndProc)>(WndProc);
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = static_cast<HMODULE>(hInstance);
		wndclass.hIcon = 0;
		wndclass.hCursor = NULL;
		wndclass.hbrBackground = NULL;
		wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
		wndclass.lpszClassName = className;

		if (!::RegisterClass(&wndclass))
			APE::Misc::MsgBox("Error registrating the window class of the code Editor! Application unstable.",
			_PROGRAM_NAME_ABRV " Error!", APE::Misc::MsgStyle::sOk | APE::Misc::MsgIcon::iWarning, wMain);
#endif
	}
	/*********************************************************************************************

	Creates the menu

	*********************************************************************************************/
	HMENU SciLexer::createMenu()
	{
		HMENU hMenu;
		hMenu = CreateMenu();
		HMENU hMenuPopup;
		if (hMenu == NULL)
			return FALSE;

		hMenuPopup = CreatePopupMenu();

		for (auto & item : APE::FileItems)
			AppendMenu(hMenuPopup, MF_STRING, item.type, item.title);
		AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuPopup, "File");

		hMenuPopup = CreatePopupMenu();

		for (auto & item : APE::EditItems)
			AppendMenu(hMenuPopup, MF_STRING, item.type, item.title);
		AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuPopup, "Edit");

		return hMenu;
	}
	/*********************************************************************************************

	Creates accelerators. Does not work: Accelerators are checked in the main message loop
	(outside of our program), dont know what to do about that.

	*********************************************************************************************/
	HACCEL SciLexer::createAccels()
	{
		HACCEL hAccel = CreateAcceleratorTable(APE::Accs, ArraySize(APE::Accs));
		return hAccel;
	}
	/*********************************************************************************************

	The function that spawns and creates the editor. Designed to be threaded and have
	a message loop.

	*********************************************************************************************/
	int SciLexer::wndThread(SciLexer * _this)
	{
		// load scite module
		auto ret = _this->hScite.load(APE::Misc::DirectoryPath + "\\SciLexer.dll");
		_this->hInstance = _this->hScite.getHandle();

		_this->registerWindowClass();
		_this->wMain = ::CreateWindowEx(
			WS_EX_CLIENTEDGE/* | WS_EX_TOPMOST*/, // consider making it a child instead.
			className,
			"Demonstration",
			WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
			WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
			WS_MAXIMIZE | WS_CLIPCHILDREN,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			NULL,
			_this->createMenu(),
			static_cast<HMODULE>(_this->hInstance),
			_this);

		_this->SetTitle();

		/*
		open a default file from settings
		*/
		std::string file;
		try
		{
			auto & root = _this->engine->getRootSettings();
			root["languages"].lookupValue("default_file", file);
		}
		catch (const std::exception & e)
		{
			_this->engine->getGraphicUI()->console->printLine(Colours::red, "Error reading default file from config... %s", e.what());
		}
		if (file.length())
			_this->openFile((APE::Misc::DirectoryPath() + file).c_str());

		_this->show();
		UpdateWindow(static_cast<HWND>(_this->wMain));
#ifdef _EDITOR_USE_THREADED_MSG_LOOP
		HACCEL hAccTable = _this->createAccels();
		MSG msg;
		BOOL bRet;

		while ((bRet = GetMessage(&msg, _this->wMain, 0, 0)) != 0)
		{
			if (bRet == -1)
			{
				break;
			}
			else
			{
				// Check for accelerator keystrokes. 

				if (!TranslateAccelerator(
					_this->wMain,  // handle to receiving window 
					hAccTable,    // handle to active accelerator table 
					&msg))         // message data 
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
		return msg.wParam;
#endif
		return 0;
	}
	/*********************************************************************************************

	Initializes the editor

	*********************************************************************************************/
	bool SciLexer::initEditor()
	{
#ifdef _EDITOR_USE_THREADED_MSG_LOOP
		std::thread(wndThread, this).detach();
#else
		return !wndThread(this);
#endif
	}
}
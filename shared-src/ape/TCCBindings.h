/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.4.0.
	 
	 Copyright (C) 2017 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:TCCBindings.h
	
		Automatic, threadsafe and reentrant bindings to TCC

*************************************************************************************/

#ifndef TCC_BINDINGS_H
	#define TCC_BINDINGS_H

	#include "APE.h"
	#include "libtcc.h"
	#include <mutex>
	#include <memory>
	#ifndef TCC_STATIC_LINK
		#include <cpl/CModule.h>
	#endif
	#include <cpl/Utility.h>

	namespace APE
	{
		class TCCBindings
		{
		public:

			class CompilerAccess : cpl::Utility::CNoncopyable
			{
			public:

				CompilerAccess() : bindings(instance()), compilerLock(bindings.compilerMutex) {}

				bool isLinked() const noexcept { return bindings.linkedCorrectly; }

				TCCState * createState() const { return bindings.newState(); }
				void deleteState(TCCState * s) const { return bindings.deleteState(s); }
				void setLibPath(TCCState * s, const char *path) const { return bindings.setLibPath(s, path); }
				bool addLibPath(TCCState * s, const char *path) const { return bindings.addLibPath(s, path) != -1; }
				void setErrorFunc(TCCState * s, void * errorOpaque, APE_ErrorFunc errorFunction) const { return bindings.setErrorFunc(s, errorOpaque, errorFunction); }
				void addIncludePath(TCCState * s, const char * pathName) const { bindings.addIncludePath(s, pathName); }
				void defineSymbol(TCCState * s, const char * symbol, const char * value) const { return bindings.defineSymbol(s, symbol, value); }
				bool compileString(TCCState * s, const char * buffer) const { return bindings.compileString(s, buffer) != -1; }
				int setOutputType(TCCState * s, int outputType) const { return bindings.setOutputType(s, outputType); }
				bool relocate(TCCState * s, void * options) const { return bindings.relocate(s, options) != -1; }
				void * getSymbol(TCCState * s, const char * name) const { return bindings.getSymbol(s, name); }
				void addSymbol(TCCState * s, const char * name, const void * value) const { bindings.addSymbol(s, name, value); }
				void setOptions(TCCState * s, const char * commands) const { return bindings.setOptions(s, commands); }
				bool addFile(TCCState * s, const char * file) const { return bindings.addFile(s, file) != -1; }
				bool outputFile(TCCState * s, const char * ofile) const { return bindings.outputFile(s, ofile) != -1; }

			private:

				TCCBindings & bindings;
				std::lock_guard<std::recursive_mutex> compilerLock;
			};

		private:

			TCCBindings()
			{
#ifdef TCC_STATIC_LINK
				newState = tcc_new;
				setLibPath = tcc_set_lib_path;
				addLibPath = tcc_add_library_path;
				addIncludePath = tcc_add_include_path;
				setOutputType = tcc_set_output_type;
				setErrorFunc = tcc_set_error_func;
				compileString = tcc_compile_string;
				deleteState = tcc_delete;
				relocate = tcc_relocate;
				addSymbol = tcc_add_symbol;
				getSymbol = tcc_get_symbol;
				defineSymbol = tcc_define_symbol;
				setOptions = tcc_set_options;
				addFile = tcc_add_file;
				outputFile = tcc_output_file;
#else
				if (!tccDLib.load(cpl::Misc::DirectoryPath() + "/libtcc.dll"))
				{
					newState = (decltype(newState))tccDLib.getFuncAddress("tcc_new");
					setLibPath = (decltype(setLibPath))tccDLib.getFuncAddress("tcc_set_lib_path");
					addLibPath = (decltype(addLibPath))tccDLib.getFuncAddress("tcc_add_library_path");
					addIncludePath = (decltype(addIncludePath))tccDLib.getFuncAddress("tcc_add_include_path");
					setOutputType = (decltype(setOutputType))tccDLib.getFuncAddress("tcc_set_output_type");
					setErrorFunc = (decltype(setErrorFunc))tccDLib.getFuncAddress("tcc_set_error_func");
					compileString = (decltype(compileString))tccDLib.getFuncAddress("tcc_compile_string");
					deleteState = (decltype(deleteState))tccDLib.getFuncAddress("tcc_delete");
					relocate = (decltype(relocate))tccDLib.getFuncAddress("tcc_relocate");
					getSymbol = (decltype(getSymbol))tccDLib.getFuncAddress("tcc_get_symbol");
					addSymbol = (decltype(addSymbol))tccDLib.getFuncAddress("tcc_add_symbol");
					defineSymbol = (decltype(defineSymbol))tccDLib.getFuncAddress("tcc_define_symbol");
					setOptions = (decltype(setOptions))tccDLib.getFuncAddress("tcc_set_options");
					addFile = (decltype(addFile))tccDLib.getFuncAddress("tcc_add_file");
					outputFile = (decltype(outputFile))tccDLib.getFuncAddress("tcc_output_file");
				}
#endif

				// test whether ALL functions pointers are valid.
				linkedCorrectly = newState && setLibPath && addSymbol && addLibPath && addIncludePath && setOutputType && setErrorFunc
					&& compileString && deleteState && relocate && getSymbol && defineSymbol && setOptions
					&& addFile && outputFile;
			}

			static TCCBindings & instance()
			{
				static TCCBindings bindings;
				return bindings;
			}

			decltype(tcc_new) * newState;
			decltype(tcc_delete) * deleteState;
			decltype(tcc_set_lib_path) * setLibPath;
			decltype(tcc_add_library_path) * addLibPath;
			decltype(tcc_set_error_func) * setErrorFunc;
			decltype(tcc_add_include_path) * addIncludePath;
			decltype(tcc_define_symbol) * defineSymbol;
			decltype(tcc_add_symbol) * addSymbol;
			decltype(tcc_compile_string) * compileString;
			decltype(tcc_set_output_type) * setOutputType;
			decltype(tcc_relocate) * relocate;
			decltype(tcc_get_symbol) * getSymbol;
			decltype(tcc_set_options) * setOptions;
			decltype(tcc_add_file) * addFile;
			decltype(tcc_output_file) * outputFile;

#ifndef TCC_STATIC_LINK
			cpl::CModule tccDLib;
#endif

		private:

			bool linkedCorrectly;
			std::recursive_mutex compilerMutex;
		};

		class TCCDeleter
		{
			public: void operator()(TCCState * s) { TCCBindings::CompilerAccess().deleteState(s); }
		};
		typedef std::unique_ptr<TCCState, TCCDeleter> UniqueTCC;

	}
#endif
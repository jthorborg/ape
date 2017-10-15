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

	file:CExclusiveFile.h
	
		A file class that marks owned file as exclusive. This implies that other instances
		of this class cannot open the same file. This lock is obtained atomically and can
		be used to allow sequental access to files.
		Whether a file is exclusive can be tested with the static method isFileExclusive().
		Notice that isFileExclusive() is only predictable on existing files.
		Class is RAII safe (file is closed on destruction).
 
	options:
		#define __CEF_USE_CSTDLIB
			causes the header to use the cstdlib, losing all previous properties but provided
			for compability

*************************************************************************************/

#ifndef _CEXCLUSIVEFILE_H
	#define _CEXCLUSIVEFILE_H

	#include "MacroConstants.h"
	#include <cstring>
	#include <string>
	#if defined(__CEF_USE_CSTDLIB)
		#include <cstdio>
		typedef FILE * FileHandle;
	#elif defined(__WINDOWS__)
		#include <Windows.h>
		typedef HANDLE FileHandle;
	#elif defined(__MAC__)
		#include <unistd.h>
		#include <fcntl.h>
		typedef int FileHandle;
	#endif


	namespace APE
	{
		
		class CExclusiveFile
		{
		public:
			enum mode
			{
				#ifdef __WINDOWS__
					readMode = GENERIC_READ,
					writeMode = GENERIC_WRITE,
					readWriteMode = readMode | writeMode
				#elif defined(__MAC__)
					readMode = 2,
					writeMode = 4
				#endif
			};
			
		private:
			
			bool isOpen;
			FileHandle handle;
			std::string fileName;
			CExclusiveFile(const CExclusiveFile &);
			mode fileMode;
			
		public:
			
			CExclusiveFile();
			~CExclusiveFile();

			bool open(const std::string & path, mode m = writeMode, bool waitForLock = false);
			bool read(void * src, std::size_t bufsiz);
			bool write(const void * src, std::size_t bufsiz);
			static bool isFileExclusive(const std::string & path);
			bool reset();
			bool remove();
			bool write(const char * src);
			bool isOpened();
			const std::string & getName();
			bool flush();
			bool close();
		};
	} //namespace APE'
#endif
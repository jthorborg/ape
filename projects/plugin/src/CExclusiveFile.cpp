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
 
	 file:CExclusiveFile.cpp
	 
		Implementation of CExclusiveFile class.
 
 *************************************************************************************/

#include "CExclusiveFile.h"
#include "MacroConstants.h"
#include "Misc.h"

namespace APE 
{

	CExclusiveFile::CExclusiveFile()
		: isOpen(false), handle(0)
	{

	}

	CExclusiveFile::~CExclusiveFile()
	{
		if (isOpened())
			close();
	}

	bool CExclusiveFile::open(const std::string & path, mode m, bool waitForLock)
	{
		if (isOpened())
			close();
		fileName = path;
		fileMode = m;
		#if defined(__CEF_USE_CSTDLIB)

			handle = ::fopen(fileName.c_str(), "w");
			if(!handle) {
				isOpen = false;
				handle = 0;
			}
			isOpen = true;
			return isOpened();

		#elif defined(__WINDOWS__)
			// if we're opening in writemode, we always create a new, empty file
			// if it's reading mode, we only open existing files.
			auto openMode = fileMode & writeMode ? CREATE_ALWAYS : OPEN_EXISTING;
			do
			{
				handle = ::CreateFile(path.c_str(),
					fileMode,
					0x0, // this is the important part - 0 as dwShareMode will open the file exclusively (default)
					nullptr,
					openMode,
					FILE_ATTRIBUTE_NORMAL,
					nullptr);
				if (handle != INVALID_HANDLE_VALUE)
					break;
				Misc::Delay(0); // yield to other threads.
			} while (waitForLock);
			if (handle != INVALID_HANDLE_VALUE)
			{
				isOpen = true;
			}
			else
			{
				isOpen = false;
				handle = 0;
			}

		#elif defined(__MAC__)

			int openMask = m == mode::writeMode ? O_WRONLY | O_CREAT : O_RDONLY ;
			if(m == mode::writeMode)
			{

				mode_t permission = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;

				handle = ::open(fileName.c_str(), openMask, permission);
			}
			else
			{
				handle = ::open(fileName.c_str(), openMask);

			}
			if (handle < 0)
			{
				isOpen = false;
				handle = 0;
				return false;
			}
			int mask = waitForLock ? 0 : LOCK_NB;
			mask |= LOCK_EX;
			auto ret = ::flock(handle, mask);
			// could not obtain lock on file, file is opened in another instance
			if (ret)
			{
				this->close();
				return false;
			}
			isOpen = true;

		#endif

		return isOpened();
	}

	bool CExclusiveFile::read(void * src, std::size_t bufsiz)
	{
		#if defined(__CEF_USE_CSTDLIB)
			if (isOpen)
				return std::fwrite(src, bufsiz, 1, handle) == bufsiz;
		#elif defined(__WINDOWS__)
			DWORD dwRead(0);
			DWORD dwSize = static_cast<DWORD>(bufsiz);
			auto ret = ::ReadFile(handle, src, dwSize, &dwRead, nullptr);
			return (ret != FALSE) && (dwRead == dwSize);
		#elif defined(__MAC__)
			if (isOpen)
				return ::read(handle, src, bufsiz) == bufsiz;
			#endif
		return false;
	}

	bool CExclusiveFile::write(const void * src, std::size_t bufsiz)
	{
		#if defined(__CEF_USE_CSTDLIB)
			if (isOpen)
				return std::fwrite(src, bufsiz, 1, handle) == bufsiz;
		#elif defined(__WINDOWS__)
			DWORD dwWritten(0);
			DWORD dwSize = static_cast<DWORD>(bufsiz);
			auto ret = ::WriteFile(handle, src, dwSize, &dwWritten, nullptr);
			return (ret != FALSE) && (dwWritten == dwSize);
		#elif defined(__MAC__)
			if (isOpen)
				return ::write(handle, src, bufsiz) == bufsiz;
		#endif
		return false;
	}

	bool CExclusiveFile::isFileExclusive(const std::string & path)
	{
		CExclusiveFile f;
		f.open(path, mode::readMode, false);
		return !f.isOpened();
	}

	bool CExclusiveFile::reset()
	{
		close();
		return open(fileName);
	}

	bool CExclusiveFile::remove()
	{

		if (isOpened())
		{
			close();
			std::remove(fileName.c_str());
			return true;
		}
		return false;
	}

	bool CExclusiveFile::write(const char * src)
	{
		return this->write(src, std::strlen(src));
	}

	bool CExclusiveFile::isOpened()
	{
		return isOpen;
	}

	const std::string & CExclusiveFile::getName()
	{
		return fileName;
	}

	bool CExclusiveFile::flush()
	{
		if (!isOpen)
			return false;
		#if defined(__CEF_USE_CSTDLIB)
			return !std::fflush(handle);
		#elif defined(__WINDOWS__)
			return !!::FlushFileBuffers(handle);
		#elif defined(__MAC__)
			return ::fsync(handle) < 0 ? false : true;
		#endif
		return false;
	}

	bool CExclusiveFile::close()
	{
		if (!isOpen)
			return false;
		bool ret(false);
		#if defined(__CEF_USE_CSTDLIB)
			ret = !std::fclose(handle);
		#elif defined(__WINDOWS__)
			ret = !!::CloseHandle(handle);
		#elif defined(__MAC__)
			::flock(handle, LOCK_UN);
			ret = ::close(handle) < 0 ? false : true;
		#endif
		isOpen = false;
		handle = (decltype(handle))0;
		return ret;
	}
};
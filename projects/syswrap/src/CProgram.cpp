/*************************************************************************************

	syswrap Compiler for Audio Programming Environment. 

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

	file:CProgram.cpp
		
		Implementation of CProgram.h

*************************************************************************************/

#include "CProgram.h"

namespace syswrap
{

	CProgram::CProgram(std::string name, std::string commandline)
		: name(name), commandline(commandline)
	{
		ZeroMemory(&pi, sizeof pi);
		ZeroMemory(&si, sizeof si);
	}

	int CProgram::run()
	{
		SECURITY_ATTRIBUTES sa;
		DWORD bSuccess, dwRead;
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = true;
		sa.lpSecurityDescriptor = nullptr;

		// Create a pipe for the child process's STDOUT. 

		if (!CreatePipe(&pRead, &pWrite, &sa, 0))
			return -2; // error creating pipe

		// Ensure the read handle to the pipe for STDOUT is not inherited.

		if (!SetHandleInformation(pRead, HANDLE_FLAG_INHERIT, 0))
			return -3; // error setting handle information

		// set startup info
		si.cb = sizeof(si);
		si.hStdError = pWrite;
		si.hStdOutput = pWrite;
		si.hStdInput = pRead;
		si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		// format command line
		char szCmd[MAX_PATH * 2];
		ZeroMemory(szCmd, sizeof szCmd);
		name += " ";
		strcat_s(szCmd, name.c_str());
		strcat_s(szCmd, commandline.c_str());
		szCmd[MAX_PATH - 1] = '\0';

		bSuccess = CreateProcessA(nullptr,
			szCmd,     // command line 
			nullptr,          // process security attributes 
			nullptr,          // primary thread security attributes 
			true,          // handles are inherited 
			0,             // creation flags 
			nullptr,          // use parent's environment 
			nullptr,          // use parent's current directory 
			&si,  // STARTUPINFO pointer 
			&pi);  // receives PROCESS_INFORMATION 
		// error out
		if (!bSuccess)
			return -1; // error running program
		// close handles and pipes.
		CloseHandle(pi.hThread);
		CloseHandle(pWrite);

		// read output of program through pRead pipe
		char readBuf[1024];
		while (true)
		{
			bSuccess = ReadFile(pRead, readBuf, sizeof readBuf, &dwRead, nullptr);
			if (!bSuccess || dwRead == 0)
				break;
			output.append(readBuf, dwRead);
		}

		DWORD exitCode, result;
		result = WaitForSingleObject(pi.hProcess, 30 * 1000);
		GetExitCodeProcess(pi.hProcess, &exitCode);
		// close rest of handles
		CloseHandle(pi.hProcess);
		CloseHandle(pRead);
		if (result == WAIT_TIMEOUT)
			return -4; // time out
		else
			return exitCode; // correct termination, return %errorlvl%
	}

	const std::string & CProgram::getOutput()
	{
		return output;
	}

}
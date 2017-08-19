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

	file:CProgram.h
		
		Implements the interface for the CProgram class, that runs programs and 
		captures their return code and console output.

*************************************************************************************/

#include <Windows.h>
#include <string>

namespace syswrap
{
	class CProgram
	{
		PROCESS_INFORMATION pi;
		STARTUPINFOA si;
		HANDLE pRead, pWrite;

		std::string name, commandline;
		std::string output;
	public:
		CProgram(std::string name, std::string commandline);
		int run();
		const std::string & getOutput();
	};
};
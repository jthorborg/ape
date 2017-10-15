/*************************************************************************************

	C++ compiler for Audio Programming Environment. 

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

	file:TranslationUnit.h
		
		Class that handles a complete translation unit.

*************************************************************************************/
#include <cpl/Misc.h>

namespace CppAPE
{
	using namespace APE;

	class TranslationUnit
	{
		TranslationUnit(const std::string& source, const std::string& name)
			: input(source), name(name)
		{
		}

	public:

		static TranslationUnit FromSource(const std::string& source, const std::string& name)
		{
			return { source, name };
		}

		static TranslationUnit FromFile(const std::string& path)
		{
			auto contents = cpl::Misc::ReadFile(path);

			if (!contents.first)
			{
				CPL_RUNTIME_EXCEPTION("Couldn't read file: " + path);
			}

			return { std::move(contents.second), path };
		}


		TranslationUnit & includeDirs(const std::vector<std::string>& idirs)
		{
			dirs.insert(dirs.end(), idirs.begin(), idirs.end());
			return *this;
		}

		TranslationUnit & preArgs(const std::string& ppArgs)
		{
			prearg += ppArgs + " ";
			return *this;
		}

		TranslationUnit & cppArgs(const std::string& cppArgs)
		{
			cpparg += cppArgs + " ";
			return *this;
		}

		bool translate() 
		{
			auto root = cpl::Misc::DirectoryPath();
			auto build = root + "/build/";

			auto infile = "\"" + build + "input.c\"";
			auto outfile = "\"" + build + "output.c\"";

			{
				std::string ppCommandLine = "mcpp.exe " + prearg;
				for (auto&& s : dirs)
					ppCommandLine += "-I" + s + " ";

				if (!cpl::Misc::WriteFile(build + "input.c", input))
					CPL_RUNTIME_EXCEPTION("Can't write contents of " + name + " to " + build + "input.c");

				ppCommandLine += " " + infile;

				ppCommandLine += " " + outfile;

				auto response = cpl::Misc::ExecCommand(ppCommandLine + " 2>&1");

				if (response.first != 0)
				{
					error = "Error executing: " + ppCommandLine + "\n";
					error += response.second + "\n";
					error += "Last error(" + std::to_string((int)cpl::Misc::GetLastOSErrorCode()) + "): " + cpl::Misc::GetLastOSErrorMessage();
					return false;
				}
			}

			{
				std::string cppCommandLine = "cfront " + cpparg;

				auto response = cpl::Misc::ExecCommand(cppCommandLine + " < " + outfile + " > " + infile + "2>&1");

				if (response.first != 0)
				{
					error = response.second;
					return false;
				}

				response = cpl::Misc::ReadFile(infile);
				if (response.first != 0)
				{
					error = "Error reading cfront output";
					return false;
				}

				translation = std::move(response.second);
			}

			return true;
		}

		const std::string & getTranslation() const noexcept
		{
			return translation;
		}

		const std::string & getError() const noexcept
		{
			return error;
		}

	private:

		std::vector<std::string> dirs;
		std::string input, prearg, cpparg, error, name, translation;
	};

};



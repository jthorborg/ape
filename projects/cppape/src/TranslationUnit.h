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
#include <experimental/filesystem>
#include <cpl/Process.h>

namespace CppAPE
{
	using namespace APE;
	namespace fs = std::experimental::filesystem;

	class TranslationUnit
	{

		TranslationUnit(const std::string& source, const std::string& name)
			: input(source)
			, name(name)
		{
			preArguments.argPair("-I", fs::path(name).parent_path().string(), preArguments.NoSpace | preArguments.Escaped);
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
			for (auto & d : idirs)
				preArguments.argPair("-I", d, preArguments.NoSpace | preArguments.Escaped);

			return *this;
		}

		TranslationUnit & preArgs(cpl::Args args)
		{
			preArguments += std::move(args);
			return *this;
		}

		TranslationUnit & cppArgs(cpl::Args args)
		{
			cppArguments += std::move(args);
			return *this;
		}

		bool translate() 
		{
			using namespace cpl;

			fs::path root = cpl::Misc::DirectoryPath();
			auto allStreams = Process::IOStreamFlags::In | Process::IOStreamFlags::Out | Process::IOStreamFlags::Err;

			auto pp = Process::Builder((root / "mcpp.exe").string())
				.launch(preArguments.arg("-+"), allStreams);

			std::string s;

			pp.cin() << input << std::endl;
			pp.cin().close();

			auto cfront = Process::Builder((root / "cfront.exe").string())
				.launch(cppArguments, allStreams);

			while (std::getline(pp.cout(), s))
			{
				if(cfront.alive())
					cfront.cin() << s << '\n';
			}

			cfront.cin() << std::endl;
			cfront.cin().close();

			while (std::getline(cfront.cout(), s))
				translation += s + '\n';


			while (std::getline(pp.cerr(), s))
				error += "PP: " + s + "\n";

			while (std::getline(cfront.cerr(), s))
				error += "C++: " + s + "\n";

			pp.join();
			cfront.join();

			return pp.getExitCode() == EXIT_SUCCESS && cfront.getExitCode() == EXIT_SUCCESS;

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

		cpl::Args preArguments, cppArguments;
		std::string input, error, name, translation;
	};

};



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
#ifndef CPPAPE_TRANSLATIONUNIT_H
#define CPPAPE_TRANSLATIONUNIT_H
#include <cpl/Misc.h>
#include <cpl/Process.h>
#include <fstream>
#include <experimental/filesystem>
#include <future>

namespace CppAPE
{
	namespace fs = std::experimental::filesystem;
	using namespace APE;

	class TranslationUnit
	{

		TranslationUnit(std::string source, std::string name)
			: input(std::move(source))
			, name(std::move(name))
		{
			preArguments
				// unset system headers
				.arg("-I-")
				// include directory of file itself
				.argPair("-I", fs::path(name).parent_path().string(), preArguments.NoSpace | preArguments.Escaped)
				// compile C++ source compability
				.arg("-+")
				.argPair("-D", "__cplusplus=199711L", preArguments.NoSpace)
				.argPair("-V", "199901L", preArguments.NoSpace)
				// cfront does not support signed, ignore
				.argPair("-D", "signed=\"\"", preArguments.NoSpace)
				.argPair("-D", "__cfront", preArguments.NoSpace)
				.argPair("-W", "0", preArguments.NoSpace);
	
			cppArguments
				// support long doubles
				.arg("+a1");
		}

	public:

		struct CommonOptions
		{
			fs::path szalFile;
			fs::path constructorSinkFile;
			fs::path destructorSinkFile;
			fs::path globalSymSinkFile;

			explicit CommonOptions(const fs::path& root, const fs::path& szalFile, const std::string& prefix = "")
				: szalFile(szalFile)
				, constructorSinkFile(root / (prefix + "ctors.gen.inl"))
				, destructorSinkFile(root / (prefix + "dtors.gen.inl"))
				, globalSymSinkFile(root / (prefix + "sym.gen.h"))
			{

			}

			void ensureCreated() const
			{
				for (auto f : {szalFile, constructorSinkFile, destructorSinkFile, globalSymSinkFile})
				{
					if (f.is_relative())
					{
						f = cpl::Misc::DirectoryPath() / f;
					}
					std::fclose(std::fopen(f.string().c_str(), "a+"));

				}
			}

			void clean() const
			{
				for (auto f : {szalFile, constructorSinkFile, destructorSinkFile, globalSymSinkFile})
				{
					if (f.is_relative())
					{
						f = cpl::Misc::DirectoryPath() / f;
					}

					std::remove(f.string().c_str());
				}
			}
		};

		static TranslationUnit FromSource(std::string source, std::string name)
		{
			return { source, name };
		}

		static TranslationUnit FromFile(std::string path)
		{
			auto contents = cpl::Misc::ReadFile(path);

			if (!contents.first)
			{
				CPL_RUNTIME_EXCEPTION("Couldn't read file: " + path);
			}

			return { std::move(contents.second), std::move(path) };
		}

		static TranslationUnit FromFile(fs::path path)
		{
			return FromFile(path.string());
		}

		TranslationUnit& addSource(fs::path path)
		{
			return addSource(cpl::Misc::ReadFile(path.string()).second);
		}

		TranslationUnit& addSource(const std::string& contents)
		{
			input += "\n";
			input += contents;
			return *this;
		}

		TranslationUnit & options(const CommonOptions& options)
		{
			if(fs::exists(options.szalFile))
				cppArguments.argPair("+x", options.szalFile.string(), preArguments.NoSpace);

			cppArguments.argPair("+y", options.constructorSinkFile.string(), preArguments.NoSpace);
			cppArguments.argPair("+z", options.destructorSinkFile.string(), preArguments.NoSpace);
			cppArguments.argPair("+q", options.globalSymSinkFile.string(), preArguments.NoSpace);

			return *this;
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
				.workingDir(root.string())
				.launch(preArguments, allStreams);

			std::ofstream 
				preOut((root / "build" / "mcpp_out.cpp").c_str()),
				cfOut((root / "build" / "cfront_out.c").c_str());

			auto cfront = Process::Builder((root / "cfront.exe").string())
				.workingDir(root.string())
				.launch(cppArguments, allStreams);

			auto pipingProcess = std::async(
				[&]() 
				{
					std::string s;
					while (std::getline(pp.cout(), s))
					{
						cfront.cin() << s << '\n';
						preOut << s << '\n';
					}

					cfront.cin() << std::endl;
					cfront.cin().close();
				}
			);

			auto resultProcess = std::async(
				[&]()
				{
					std::string s;
					while (std::getline(cfront.cout(), s))
						translation += s + '\n';
				}
			);

			pp.cin() << "#line 2 \"" << fs::path(name).filename() << "\"" << std::endl;
			pp.cin() << "#line 1 \"" << fs::path(name).filename() << "\"" << std::endl;

			pp.cin() << input << std::endl;
			pp.cin().close();

			pipingProcess.wait();
			resultProcess.wait();

			std::string s;

			while (std::getline(pp.cerr(), s))
				error += "PP: " + s + "\n";

			while (std::getline(cfront.cerr(), s))
				error += "C++: " + s + "\n";

			cfOut << translation << std::endl;

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


#endif
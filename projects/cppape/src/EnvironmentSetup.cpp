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

	file:EnvironmentSetup.cpp
		
		Sets up a C++ environment for CppAPE

*************************************************************************************/

#include "CppAPE.h"
#include <ape/CompilerBindings.h>
#include <cpl/CExclusiveFile.h>
#include <cstdarg>

namespace CppAPE
{
	namespace
	{
		thread_local std::string stdOut, stdErr;

		int APE_API_VARI printf_hook(const char * fmt, ...)
		{
			char buf[1024];
			va_list args;
			va_start(args, fmt);
			auto count = vsprintf(buf, fmt, args);
			va_end(args);
			stdOut += buf;
			return count;
		}

		int APE_API_VARI fprintf_hook(FILE * file, const char * fmt, ...)
		{
			char buf[1024];
			va_list args;
			va_start(args, fmt);
			auto count = vsprintf(buf, fmt, args);
			va_end(args);
			stdErr += buf;
			return count;
		}
	}

	bool ScriptCompiler::ClearEnvironment()
	{
		return
			::remove(translationOptions().constructorSinkFile.string().c_str()) != -1 &&
			::remove(translationOptions().destructorSinkFile.string().c_str()) != -1 &&
			::remove(translationOptions().globalSymSinkFile.string().c_str()) != -1;
	}

	bool ScriptCompiler::SetupEnvironment()
	{
		auto root = fs::path(cpl::Misc::DirectoryPath());

		if (fs::exists(root / "runtime" / "runtime.o"))
			return true;


		APE::TCCBindings::CompilerAccess bindings;

		UniqueTCC tcc(bindings.createState());
		bindings.setLibPath(tcc.get(), (root).string().c_str());

		bindings.setErrorFunc(tcc.get(), getErrorFuncDetails().first, getErrorFuncDetails().second);
		bindings.setOutputType(tcc.get(), TCC_OUTPUT_MEMORY);

		bindings.addIncludePath(tcc.get(), (root / ".." / ".." / "includes" / "tcc").string().c_str());

		if (!bindings.addFile(tcc.get(), (root / "szal.c").string().c_str()))
		{
			print("[CppAPE] : couldn't compile szal.c");
			return false;
		}

		bindings.addSymbol(tcc.get(), "printf", &printf_hook);
		bindings.addSymbol(tcc.get(), "fprintf", &fprintf_hook);

		if (!bindings.relocate(tcc.get(), TCC_RELOCATE_AUTO))
		{
			print("[CppAPE] : couldn't relocate szal.c");
			return false;
		}

		typedef int(APE_API_VARI * Entry)();

		stdOut.clear(); stdErr.clear();

		Entry e = reinterpret_cast<Entry>(bindings.getSymbol(tcc.get(), "main"));

		if (!e)
		{
			print("[CppAPE] : no main() in szal.c");
			return false;
		}

		e();

		if (stdErr.size())
		{
			print(stdErr);
			return false;
		}

		cpl::Misc::WriteFile(translationOptions().szalFile.string(), stdOut);

		ClearEnvironment();

		try
		{
			auto unit = TranslationUnit::FromFile(root / "runtime" / "runtime.cpp");

			unit
				.includeDirs({
					(root / ".." / ".." / "includes" / "tcc").string(), 
					(root / ".." / ".." / "includes").string()}
				)
				.preArgs(sizeTypeDefines)
				.options(translationOptions());

			auto result = unit.translate();

			if (unit.getError().size())
			{
				print(unit.getError());
			}

			if (!result)
			{
				return false;
			}
			tcc.reset(nullptr);
			tcc.reset(bindings.createState());
			bindings.setLibPath(tcc.get(), (root).string().c_str());
			bindings.setErrorFunc(tcc.get(), getErrorFuncDetails().first, getErrorFuncDetails().second);
			bindings.setOutputType(tcc.get(), TCC_OUTPUT_OBJ);


			if (!bindings.compileString(tcc.get(), unit.getTranslation().c_str()))
			{
				return false;
			}

			if(!bindings.outputFile(tcc.get(), (root / "runtime" / "runtime.o").string().c_str()))
				return false;

		}
		catch (const std::exception& e)
		{
			print((std::string("Exception while compiling: ") + e.what()).c_str());
			return Status::STATUS_ERROR;
		}

		return true;

	}
}
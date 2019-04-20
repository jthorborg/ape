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
	bool ScriptCompiler::SetupEnvironment()
	{
		auto root = fs::path(cpl::Misc::DirectoryPath());

		if (fs::exists(root / "runtime" / "runtime.bc") && fs::exists(root / "runtime" / "libcxx.bc") && fs::exists(root / "runtime" / "common.h.pch"))
			return true;

		memoryEffectPCH.clear();

		CxxTranslationUnit::Builder builder;

		builder
			.onMessage(
				[this](auto e, auto msg) 
				{
					print(JitToDiagnostic(e), msg); 
				}
			)
			.includeDirs({
				(root / "runtime").string(),
				(root / ".." / ".." / "includes" / "libcxx").string(),
				(root / ".." / ".." / "includes" / "ccore").string(),
				(root / ".." / ".." / "includes").string() }
			);

		try
		{
			builder.args()
				.arg("-v")
				//.arg("fno-short-wchar")
				.arg("-fexceptions")

				.arg("-fms-extensions")
				.arg("-fcxx-exceptions")
				.arg("-O2")
				.arg("-D_LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS")

				.argPair("-D__STDC_VERSION__=", "199901L", cpl::Args::NoSpace)
				.argPair("-std=", "c++17", cpl::Args::NoSpace);

			for (auto define : defines)
				builder.args().argPair("-D", define, cpl::Args::NoSpace);

			builder
				.fromFile((root / "runtime" / "runtime.cpp").string())
				.save((root / "runtime" / "runtime.bc").string());

			print(APE_Diag_Info, "runtime.cpp -> runtime.bc");

			builder
				.generatePCH(root / ".." / ".." / "includes" / "common.h", root / "runtime" / "common.h.pch");

			print(APE_Diag_Info, "includes/common.h -> runtime/common.h.pch");

			builder
				.fromFile((root / "runtime" / "libcxx.cpp").string())
				.save((root / "runtime" / "libcxx.bc").string());

			print(APE_Diag_Info, "libcxx.cpp -> libcxx.bc");

			return true;
		}
		catch (const CxxTranslationUnit::CompilationException& e)
		{
			print(APE_Diag_CompilationError, std::string("Exception while compiling: ") + e.what());
			return false;
		}

	}
}
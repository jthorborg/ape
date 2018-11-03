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

	file:CppCompilerInterface.cpp
		
		Implementation of the compiler API.

*************************************************************************************/

#include "ape.h"
#include "ProtoCompiler.hpp"
#include "Project.h"
#include "CompilerBindings.h"

APE_Status CleanCompilerCache();
ape::ProtoCompiler * CreateCompiler();
void DeleteCompiler(ape::ProtoCompiler * toBeDeleted);

using namespace ape;

extern "C" 
{
	EXPORTED APE_Status APE_API CreateProject(APE_Project * p)
	{
		ProtoCompiler * compiler = CreateCompiler();
		p->userData = compiler;
		return STATUS_OK;
	}

	EXPORTED Status APE_API CompileProject(Project * p, void * op, ErrorFunc e)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		compiler->setErrorFunc(op, e);
		return compiler->compileProject();
	}

	EXPORTED Status APE_API ReleaseProject(Project * p)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		auto s = compiler->releaseProject();
		DeleteCompiler(compiler);
		return s;
	}

	EXPORTED Status	APE_API InitProject(Project * p)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		return compiler->initProject();
	}

	EXPORTED Status	APE_API ActivateProject(Project * p)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		return compiler->activateProject();
	}

	EXPORTED Status	APE_API DisableProject(Project * p, int misbehaved)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		return compiler->disableProject(misbehaved ? true : false);
	}

	EXPORTED Status	APE_API ProcessReplacing(Project * p, const float * const * in, float * const * out, size_t sampleFrames)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		return compiler->processReplacing(in, out, sampleFrames);
	}

	EXPORTED Status	APE_API OnEvent(Project * p, Event * e)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		return compiler->onEvent(e);
	}

	EXPORTED Status	APE_API CleanCache()
	{
		return CleanCompilerCache();
	}
}

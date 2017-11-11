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

ape::ProtoCompiler * CreateCompiler();
void DeleteCompiler(ape::ProtoCompiler * toBeDeleted);

using namespace ape;

extern "C" 
{
	EXPORTED void * APE_API GetSymbol(Project * p, char * s) 
	{
		return nullptr;
	}

	EXPORTED Status APE_API CompileProject(Project * p, void * op, ErrorFunc e)
	{
		ProtoCompiler * compiler = CreateCompiler();
		p->userData = compiler;
		compiler->setProject(p);
		compiler->setErrorFunc(op, e);
		return compiler->compileProject();
	}

	EXPORTED Status	APE_API SetErrorFunc(Project * p, void * op, ErrorFunc e)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		compiler->setErrorFunc(op, e);
		return STATUS_OK;
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

	EXPORTED Status	APE_API GetState(Project * p)
	{
		return STATUS_NOT_IMPLEMENTED;
	}

	EXPORTED Status	APE_API AddSymbol(Project * p, const char * name, void * mem)
	{
		return STATUS_NOT_IMPLEMENTED;
	}

	EXPORTED Status	APE_API ProcessReplacing(Project * p, float ** in, float ** out, int sampleFrames)
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

}

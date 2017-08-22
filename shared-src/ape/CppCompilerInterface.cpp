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

#include "APE.h"
#include "ProtoCompiler.hpp"
#include "Project.h"
#include "CompilerBindings.h"

APE::ProtoCompiler * CreateCompiler();
void DeleteCompiler(APE::ProtoCompiler * toBeDeleted);

using namespace APE;

extern "C" 
{
	EXPORTED void * APE_API GetSymbol(CProject * p, char * s) 
	{
		return nullptr;
	}

	EXPORTED Status APE_API CompileProject(CProject * p, void * op, ErrorFunc e)
	{
		ProtoCompiler * compiler = CreateCompiler();
		p->userData = compiler;
		compiler->setProject(p);
		compiler->setErrorFunc(op, e);
		return compiler->compileProject();
	}

	EXPORTED Status	APE_API SetErrorFunc(CProject * p, void * op, ErrorFunc e)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		return compiler->setErrorFunc(op, e);
	}

	EXPORTED Status APE_API ReleaseProject(CProject * p)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		auto s = compiler->releaseProject();
		DeleteCompiler(compiler);
		return s;
	}

	EXPORTED Status	APE_API InitProject(CProject * p)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		return compiler->initProject();
	}

	EXPORTED Status	APE_API ActivateProject(CProject * p)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		return compiler->activateProject();
	}

	EXPORTED Status	APE_API DisableProject(CProject * p)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		return compiler->disableProject();
	}

	EXPORTED Status	APE_API GetState(CProject * p)
	{
		return STATUS_NOT_IMPLEMENTED;
	}

	EXPORTED Status	APE_API AddSymbol(CProject * p, const char * name, void * mem)
	{
		return STATUS_NOT_IMPLEMENTED;
	}

	EXPORTED Status	APE_API ProcessReplacing(CProject * p, float ** in, float ** out, int sampleFrames)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		return compiler->processReplacing(in, out, sampleFrames);
	}

	EXPORTED Status	APE_API OnEvent(CProject * p, CEvent * e)
	{
		ProtoCompiler * compiler = reinterpret_cast<ProtoCompiler*>(p->userData);
		compiler->setProject(p);
		return compiler->onEvent(e);
	}

}

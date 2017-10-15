/*************************************************************************************

	Tiny C Compiler for Audio Programming Environment. 

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

	file:Interface.cpp
		
		Implementation of the compiler API.

*************************************************************************************/

#include "APESupport.h"
#include "Tcc4APE.h"

using namespace TCC4Ape;

#ifdef __cplusplus
	extern  "C" 
	{
#endif

EXPORTED void * STD_API GetSymbol(CProject * p, char * s) 
{
	return nullptr;
}
EXPORTED Status STD_API CompileProject(CProject * p, void * op, errorFunc_t e)
{
	CCompiler * _this = new CCompiler;
	p->userData = _this;
	_this->setProject(p);
	_this->setErrorFunc(op, e);
	return _this->compileProject();
}
EXPORTED Status	STD_API SetErrorFunc(CProject * p, void * op, errorFunc_t e)
{
	CCompiler * _this = reinterpret_cast<CCompiler*>(p->userData);
	_this->setProject(p);
	return _this->setErrorFunc(op, e);
}
EXPORTED Status STD_API ReleaseProject(CProject * p)
{
	CCompiler * _this = reinterpret_cast<CCompiler*>(p->userData);
	_this->setProject(p);
	auto s = _this->releaseProject();
	delete _this;
	return s;
}
EXPORTED Status	STD_API InitProject(CProject * p) 
{
	CCompiler * _this = reinterpret_cast<CCompiler*>(p->userData);
	_this->setProject(p);
	return _this->initProject();
}
EXPORTED Status	STD_API ActivateProject(CProject * p) 
{
	CCompiler * _this = reinterpret_cast<CCompiler*>(p->userData);
	_this->setProject(p);
	return _this->activateProject();
}
EXPORTED Status	STD_API DisableProject(CProject * p) 
{
	CCompiler * _this = reinterpret_cast<CCompiler*>(p->userData);
	_this->setProject(p);
	return _this->disableProject();
}
EXPORTED Status	STD_API GetState(CProject * p) 
{
	return Status::STATUS_NOT_IMPLEMENTED;
}
EXPORTED Status	STD_API AddSymbol(CProject * p, const char * name, void * mem) 
{
	return Status::STATUS_NOT_IMPLEMENTED;
}
EXPORTED Status	STD_API ProcessReplacing(CProject * p, float ** in, float ** out, int sampleFrames) 
{
	CCompiler * _this = reinterpret_cast<CCompiler*>(p->userData);
	_this->setProject(p);
	return _this->processReplacing(in, out, sampleFrames);
}
EXPORTED Status	STD_API OnEvent(CProject * p, CEvent * e) 
{
	CCompiler * _this = reinterpret_cast<CCompiler*>(p->userData);
	_this->setProject(p);
	return _this->onEvent(e);
}

#ifdef __cplusplus
	}
#endif
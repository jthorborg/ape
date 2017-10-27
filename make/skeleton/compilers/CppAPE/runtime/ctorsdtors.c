#include "ctorsdtors.h"
#include "cpp.sym.gen.h"
#include "../build/sym.gen.h"

PFV constructors[] = 
{
	#include "cpp.ctors.gen.inl"
	#include "../build/ctors.gen.inl"
	(PFV)0
};

PFV destructors[] =
{
	#include "cpp.dtors.gen.inl"
	#include "../build/dtors.gen.inl"
	(PFV)0
};

void runtime_init()
{
	PFV * constructor = constructors;
	while (*constructor)
	{
		(*constructor)();
		constructor++;
	}
}

void runtime_exit()
{
	PFV * destructor = destructors;
	while (*destructor)
	{
		(*destructor)();
		destructor++;
	}
}

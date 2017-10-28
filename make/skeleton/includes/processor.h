#ifndef CPPAPE_PROCESSOR_H
#define CPPAPE_PROCESSOR_H

#include "baselib.h"

class Processor
{
public:

	Processor()
	{

	}

	virtual void init()
	{

	}

	virtual ~Processor() 
	{
	
	}

};

class FactoryBase
{
public:
	typedef Processor * (*ProcessorCreater)();
	static void SetCreater(ProcessorCreater factory);

};

template<class ProcessorType>
class ProcessorFactory
{
public:

	static Processor * create()
	{
		return new ProcessorType();
	}
};

template<class ProcessorType>
int registerClass(ProcessorType* formal_null);

#define GlobalData(type, str) \
	class type; \
	int __ ## type ## __unneeded = registerClass((type*)0);

#endif
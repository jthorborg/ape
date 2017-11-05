#ifndef CPPAPE_PROCESSOR_H
#define CPPAPE_PROCESSOR_H

#include "baselib.h"
#include "shared-src/ape/Events.h"

class Processor
{
public:

	Processor()
	{

	}

	virtual void init() {}

	virtual void close() {}

	virtual void process(float ** inputs, float ** outputs, size_t frames)
	{
		(void)inputs;
		(void)outputs;
		(void)frames;
	}

	virtual Status onEvent(APE_Event * event)
	{
		(void)event;
		return Status::NotImplemented;
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
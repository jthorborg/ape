#ifndef CPPAPE_EFFECT_H
#define CPPAPE_EFFECT_H

#include "baselib.h"
#include "processor.h"
#include "parameter.h"
#include <complex>
#include <map>
#include <functional>
#include <algorithm>
#include <typeinfo>
#include "label.h"

class Effect : public Processor
{
protected:

	Effect()
	{

	}

	virtual ~Effect()
	{
		getInterface().destroyResource(&getInterface(), 0, 0);
	}

};

#endif
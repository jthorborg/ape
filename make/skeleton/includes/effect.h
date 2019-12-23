#ifndef CPPAPE_EFFECT_H
#define CPPAPE_EFFECT_H

#include "common.h"
#include "fpoint.h"

namespace ape
{
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

    class TransportEffect : public TransportProcessor
    {
    protected:

        TransportEffect()
        {

        }

        virtual ~TransportEffect()
        {
            getInterface().destroyResource(&getInterface(), 0, 0);
        }

    };
}


#endif
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
	
	template<class TEffect>
	class EmbeddedEffect
	{
	public:

		static_assert(std::is_base_of<Processor, TEffect>::value, "Embedded effects must derive from Processor");

		EmbeddedEffect()
		{
			effect.init();
		}

		~EmbeddedEffect()
		{
			effect.close();
		}

		void process(const_umatrix<float> inputs, umatrix<float> outputs, size_t frames)
		{
			effect.processFrames(inputs, outputs, frames);
		}

		void start(const IOConfig& cfg)
		{
			APE_Event_IOChanged ioEvent;
			ioEvent.inputs = cfg.inputs;
			ioEvent.outputs = cfg.outputs;
			ioEvent.blockSize = cfg.maxBlockSize;
			ioEvent.sampleRate = cfg.sampleRate;

			APE_Event e;
			e.eventType = IOChanged;
			e.event.eIOChanged = &ioEvent;

			effect.onEvent(&e);

			APE_Event_PlayStateChanged playState;
			playState.isPlaying = true;

			e.eventType = PlayStateChanged;
			e.event.ePlayStateChanged = &playState;

			effect.onEvent(&e);
		}

		void stop()
		{
			APE_Event e;

			APE_Event_PlayStateChanged playState;
			playState.isPlaying = false;

			e.eventType = PlayStateChanged;
			e.event.ePlayStateChanged = &playState;

			effect.onEvent(&e);
		}

		TEffect* operator ->()
		{
			return &effect;
		}

	private:

		TEffect effect;
	};
}


#endif
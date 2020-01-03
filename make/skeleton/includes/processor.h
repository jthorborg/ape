#ifndef CPPAPE_PROCESSOR_H
#define CPPAPE_PROCESSOR_H

#include "baselib.h"
#include "shared-src/ape/Events.h"
#include "misc.h"

namespace ape
{
	struct IOConfig
	{
		std::size_t
			inputs,
			outputs,
			maxBlockSize;

		double sampleRate;
	};

	class Processor
	{
	public:

		Processor()
		{

		}

		void init() {}
		void close() {}

		void processFrames(umatrix<const float> inputs, umatrix<float> outputs, size_t frames)
		{
			assert(configuration.sampleRate != 0);

            processingHook();
			process(inputs, outputs, frames);
		}

		virtual Status onEvent(Event * e)
		{
			switch (e->eventType)
			{
				case IOChanged:
				{
					const auto old = config();
					const auto newC = *e->event.eIOChanged;
					configuration.inputs = newC.inputs;
					configuration.outputs = newC.outputs;
					configuration.maxBlockSize = newC.blockSize;
					configuration.sampleRate = newC.sampleRate;
					return StatusCode::Handled;
				}

				case PlayStateChanged:
				{
					e->event.ePlayStateChanged->isPlaying ? start(config()) : stop();
					return StatusCode::Handled;
				}

				default:
					return StatusCode::NotImplemented;
			}
		}

		virtual ~Processor()
		{

		}

		const IOConfig& config() const 
		{
			return configuration;
		}

        std::size_t sharedChannels() const noexcept
        {
            return config().inputs > config().outputs ? config().outputs : config().inputs;
        }

	protected:

        /// <summary>
        /// Internal use only
        /// </summary>
        virtual void processingHook() {}

		void setTriggeringChannel(int channel)
		{
			getInterface().setTriggeringChannel(&getInterface(), channel);
		}
		
		void defaultProcess(umatrix<const float> inputs, umatrix<float> outputs, size_t frames)
		{
			const auto shared = sharedChannels();

			for (std::size_t c = 0; c < shared; ++c)
			{
				for (std::size_t n = 0; n < frames; ++n)
					outputs[c][n] = inputs[c][n];
			}

			clear(outputs, shared);
		}


		virtual void start(const IOConfig& config) { }
		virtual void stop() { }

		virtual void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames)
		{
			defaultProcess(inputs, outputs, frames);
		}

	private:
		IOConfig configuration;
	};

    class TransportProcessor : public Processor
    {
    public:

        Status onEvent(Event * e) override
        {
            if (e->eventType == PlayStateChanged && !e->event.ePlayStateChanged->isPlaying)
            {
                if (position.isPlaying)
                {
                    pause();
                    position.isPlaying = false;
                }
            }

            return Processor::onEvent(e);
        }

    protected:

        virtual void play() {}
        virtual void pause() {}

        const APE_PlayHeadPosition& getPlayHeadPosition()
        {
            return position;
        }

    private:

        void processingHook() override
        {
            bool wasTransportPlaying = position.isPlaying;
            if (getInterface().getPlayHeadPosition(&getInterface(), &position) != 0)
            {
                if (wasTransportPlaying && !position.isPlaying)
                {
                    pause();
                }
                else if (!wasTransportPlaying && position.isPlaying)
                {
                    play();
                }
            }
        }

        APE_PlayHeadPosition position{};
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
}

template<class ProcessorType>
static int registerClass(ProcessorType* formal_null);

#define GlobalData(type, str) \
	class type; \
	int __ ## type ## __unneeded = registerClass((type*)0);

#endif

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

		void defaultProcess(const_umatrix<float> inputs, umatrix<float> outputs, size_t frames)
		{
			const auto shared = configuration.inputs > configuration.outputs ? configuration.outputs : configuration.inputs;

			for (std::size_t c = 0; c < shared; ++c)
			{
				for (std::size_t n = 0; n < frames; ++c)
					outputs[c][n] = inputs[c][n];
			}

			for (std::size_t c = shared; c < configuration.outputs; ++c)
			{
				for (std::size_t n = 0; n < frames; ++c)
					outputs[c][n] = 0;
			}
		}

		virtual void init() {}
		virtual void close() {}

		void processBase(float** inputs, float** outputs, size_t frames)
		{
			process(
				{ inputs, frames, configuration.inputs }, 
				{ outputs, frames, configuration.outputs },
				frames
			);
		}

		virtual void process(const_umatrix<float> inputs, umatrix<float> outputs, size_t frames)
		{
			defaultProcess(inputs, outputs, frames);
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

		const IOConfig& config()
		{
			return configuration;
		}

	protected:

		virtual void start(const IOConfig& config) { }
		virtual void stop() { }

	private:
		IOConfig configuration;
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
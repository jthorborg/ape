/** @file */

#ifndef CPPAPE_PROCESSOR_H
#define CPPAPE_PROCESSOR_H

#include "baselib.h"
#include "shared-src/ape/Events.h"
#include "misc.h"

namespace ape
{
	/// <summary>
	/// Configuration structure with information needed for running a plugin.
	/// </summary>
	struct IOConfig
	{
		std::size_t
			/// <summary>
			/// How many inputs this plugin is initialized with
			/// </summary>
			inputs,
			/// <summary>
			/// How many outputs this plugin is initialized with
			/// </summary>
			outputs,
			/// <summary>
			/// The maximum amount of sample frames that can be requested at any given time.
			/// </summary>
			/// <remarks>
			/// Note that functions like <see cref="Effect::process()"/> and <see cref="Generator::process()"/>
			/// may be called with less or equal frames.
			/// </remarks>
			maxBlockSize;

		/// <summary>
		/// The sample rate this plugin is running at.
		/// </summary>
		double sampleRate;
	};

	class Processor : public UIObject
	{
	public:

		/// <summary>
		/// Called after every constructor in the inheritance chain has run
		/// </summary>
		void init() {}
		/// <summary>
		/// Called just before any destructor is run. 
		/// </summary>
		void close() {}

		/// <summary>
		/// Trigger processing of the <paramref name="inputs"/> into the <paramref name="outputs"/>.
		/// <seealso cref="EmbeddedEffect::process"/>
		/// <seealso cref="EmbeddedGenerator::process"/>
		/// </summary>
		void processFrames(umatrix<const float> inputs, umatrix<float> outputs, size_t frames)
		{
			assert(configuration.sampleRate != 0);

            processingHook();
			process(inputs, outputs, frames);
		}

		/// <summary>
		/// Send an event to this processor.
		/// </summary>
		/// <param name="Event">
		/// The polymorphic event to process.
		/// </param>
		/// <returns>
		/// Whether the event was handled (<see cref="StatusCode::Handled"/>) or not implemented
		/// (<see cref="StatusCode::NotImplemented"/>).
		/// </returns>
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

		/// <summary>
		/// Polymorphically destruct this processor
		/// </summary>
		virtual ~Processor()
		{

		}

		/// <summary>
		/// Return the configuration this processor is initialized with.
		/// </summary>
		const IOConfig& config() const 
		{
			return configuration;
		}

		/// <summary>
		/// Returns the minimum number of shared channels between inputs and outputs.
		/// </summary>
        std::size_t sharedChannels() const noexcept
        {
            return config().inputs > config().outputs ? config().outputs : config().inputs;
        }

	protected:

		Processor()
		{

		}

        /// <summary>
        /// Internal use only
        /// </summary>
        virtual void processingHook() {}

		/// <summary>
		/// Request the oscilloscope to trigger on a specific channel (default is the first output channel from the plugin).
		/// </summary>
		/// <param name="channel">
		/// 1 equals the first input.
		/// 1 + number of inputs equals the first output. 
		/// </param>
		void setTriggeringChannel(int channel)
		{
			getInterface().setTriggeringChannel(&getInterface(), channel);
		}
		
		/// <summary>
		/// Copy the number of shared channels from <paramref name="inputs"/> to <paramref name="outputs"/>, clearing
		/// any extra outputs in <paramref name="outputs"/>.
		/// <seealso cref="sharedChannels"/>
		/// <seealso cref="clear()"/>
		/// </summary>
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

		/// <summary>
		/// Start processing with a certain configuration.
		/// Resources can be allocated here.
		/// </summary>
		virtual void start(const IOConfig& config) { }
		/// <summary>
		/// Stop processing.
		/// Here's a good place to release any large resources.
		/// </summary>
		virtual void stop() { }

		/// <summary>
		/// Callback for processing a buffer switch in real-time.
		/// </summary>
		/// <param name="inputs">
		/// Read-only channel data for any inputs into this plugin.
		/// </param>
		/// <param name="outputs">
		/// Writable channel data for outputs from this plugin.
		/// </param>
		/// <param name="frames">
		/// How many samples to process from <paramref name="inputs"/> and <paramref name="outputs"/>
		/// </param>
		virtual void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames)
		{
			defaultProcess(inputs, outputs, frames);
		}

	private:
		detail::PluginResource resource;
		IOConfig configuration;
	};

	/// <summary>
	/// A <see cref="Processor"/> with additional access to the transport / playhead of the host.
	/// </summary>
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

		/// <summary>
		/// Callback when the projects starts to "play".
		/// <seealso cref="stop()"/>
		/// </summary>
		/// <remarks>
		/// Called from the audio thread.
		/// </remarks>
        virtual void play() {}
		/// <summary>
		/// Callback when the project stops playback.
		/// <seealso cref="play()"/>
		/// </summary>
		/// <remarks>
		/// Called from the audio thread.
		/// </remarks>
		virtual void pause() {}

		/// <summary>
		/// Returns current position info about the playhead.
		/// </summary>
		/// <remarks>
		/// Only sensical when called from within a <see cref="Processor::process()"/> callback
		/// </remarks>
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

	/// <summary>
	/// Class for easily embedding processors within your processor.
	/// Base functionality for <see cref="EmbeddedEffect"/> and <see cref="EmbeddedGenerator"/>.
	/// </summary>
	template<class TProcessor>
	class EmbeddedProcessor
	{
	public:

		static_assert(std::is_base_of<Processor, TProcessor>::value, "Embedded processors must derive from Processor");

		/// <summary>
		/// Initializes the processor.
		/// <see cref="Processor::init()"/>
		/// </summary>
		EmbeddedProcessor()
		{
			processor.init();
		}

		/// <summary>
		/// Initializes the processor.
		/// <see cref="Processor::init()"/>
		/// </summary>
		~EmbeddedProcessor()
		{
			processor.close();
		}

		/// <summary>
		/// Starts the processor with a specific configuration.
		/// <see cref="Processor::start()"/>
		/// </summary>
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

			processor.onEvent(&e);

			APE_Event_PlayStateChanged playState;
			playState.isPlaying = true;

			e.eventType = PlayStateChanged;
			e.event.ePlayStateChanged = &playState;

			processor.onEvent(&e);
		}

		/// <summary>
		/// Stops the processor.
		/// <see cref="Processor::stop()"/>
		/// </summary>
		void stop()
		{
			APE_Event e;

			APE_Event_PlayStateChanged playState;
			playState.isPlaying = false;

			e.eventType = PlayStateChanged;
			e.event.ePlayStateChanged = &playState;

			processor.onEvent(&e);
		}

		/// <summary>
		/// Access the wrapped <typeparamref name="TProcessor"/> instance.
		/// </summary>
		TProcessor* operator ->()
		{
			return &processor;
		}

	protected:

		TProcessor processor;
	};

	namespace detail
	{
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
		static int registerClass(ProcessorType* formal_null);
	}


}

/// <summary>
/// Declares an <see cref="ape::Effect"/> or <see cref="ape::Generator"/> to be instanced when a script containing this line is compiled.
/// As you can have multiple plugins defined in a translation unit, each successive invocation of this macro takes precedence 
/// (or in other words, the last plugin wins). 
/// </summary>
#define GlobalData(type, str) \
	class type; \
	int __ ## type ## __unneeded = ape::detail::registerClass((type*)0);

#endif

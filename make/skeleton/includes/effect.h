#ifndef CPPAPE_EFFECT_H
#define CPPAPE_EFFECT_H

#include "common.h"
#include "fpoint.h"
#include "simd.h"

namespace ape
{
	/// <summary>
	/// An effect is a simple processor that can modify an audio stream.
	/// An effect is guaranteed to the same number of inputs and outputs.
	/// <seealso cref="GlobalData"/>
	/// </summary>
	class Effect : public Processor
	{
	protected:
		Effect() {}
	};

	/// <summary>
	/// The same as <see cref="Effect"/>, except it also can query information about the playhead (<see cref="TransportProcessor::getPlayHeadPosition()"/>,
	/// as well as having play() and pause() callbacks.
	/// <see cref="TransportProcessor"/> for more information.
	/// <seealso cref="GlobalData"/>
	/// </summary>
    class TransportEffect : public TransportProcessor
    {
	protected:
		TransportEffect() {}
    };
	
	/// <summary>
	/// A utility class to embed another <see cref="Effect"/> inside your own.
	/// It takes care of initialization.
	/// </summary>
	/// <remarks>
	/// Take care to call <see cref="EmbeddedProcessor{TEffect}::start()"/> on any embedded plugins.
	/// </remarks>
	template<class TEffect>
	class EmbeddedEffect final : public EmbeddedProcessor<TEffect>
	{
	public:

		static_assert(std::is_base_of<Effect, TEffect>::value || std::is_base_of<TransportEffect, TEffect>::value, "Embedded effects must derive from a effect");

		/// <summary>
		/// Call this to invoke the embedded effect's processor routine.
		/// </summary>
		void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames)
		{
			this->processor.processFrames(inputs, outputs, frames);
		}
	};
}


#endif
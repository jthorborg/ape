#ifndef CPPAPE_EFFECT_H
#define CPPAPE_EFFECT_H

#include "common.h"
#include "fpoint.h"

namespace ape
{
	class Effect : public Processor
	{
	protected:
		Effect() {}
	};


    class TransportEffect : public TransportProcessor
    {
	protected:
		TransportEffect() {}
    };
	
	template<class TGenerator>
	class EmbeddedEffect final : public EmbeddedProcessor<TGenerator>
	{
	public:

		static_assert(std::is_base_of<Effect, TGenerator>::value || std::is_base_of<TransportEffect, TGenerator>::value, "Embedded effects must derive from a effect");

		void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames)
		{
			this->processor.processFrames(inputs, outputs, frames);
		}
	};
}


#endif
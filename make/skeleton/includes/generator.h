#ifndef CPPAPE_GENERATOR_H
#define CPPAPE_GENERATOR_H

#include "common.h"
#include "fpoint.h"

namespace ape
{
	class Generator : public Processor
	{
	protected:

		Generator() {}

		virtual void process(umatrix<float> buffer, size_t frames) {}

	private:

		void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames) final
		{
			process(outputs, frames);
		}
	};

	class TransportGenerator : public TransportProcessor
	{
	protected:

		TransportGenerator() {}

		virtual void process(umatrix<float> buffer, size_t frames) {}

	private:

		void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames) final
		{
			process(outputs, frames);
		}
	};

	template<class TGenerator>
	class EmbeddedGenerator final : public EmbeddedProcessor<TGenerator>
	{
	public:

		static_assert(std::is_base_of<Generator, TGenerator>::value || std::is_base_of<TransportGenerator, TGenerator>::value, "Embedded generators must derive from a generator");

		void process(umatrix<float> buffer, size_t frames)
		{
			this->processor.processFrames(buffer, buffer, frames);
		}
	};
}


#endif
#ifndef CPPAPE_GENERATOR_H
#define CPPAPE_GENERATOR_H

#include "common.h"
#include "fpoint.h"

namespace ape
{
	/// <summary>
	/// A generator is a processor that only creates sounds (so it has no inputs).
	/// <seealso cref="GlobalData"/>
	/// <seealso cref="Effect"/>
	/// </summary>
	class Generator : public Processor
	{
	protected:

		Generator() {}

		/// <summary>
		/// Override this to emit sound.
		/// </summary>
		/// <param name="frames">The number of sample frames to be processed.</param>
		virtual void process(umatrix<float> buffer, size_t frames) {}

	private:

		void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames) final
		{
			process(outputs, frames);
		}
	};

	/// <summary>
	/// A generator is a processor that only creates sounds (so it has no inputs).
	/// <seealso cref="GlobalData"/>
	/// <seealso cref="Effect"/>
	/// </summary>
	class TransportGenerator : public TransportProcessor
	{
	protected:

		TransportGenerator() {}

		/// <summary>
		/// Override this to emit sound.
		/// </summary>
		/// <param name="frames">The number of sample frames to be processed.</param>
		virtual void process(umatrix<float> buffer, size_t frames) {}

	private:

		void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames) final
		{
			process(outputs, frames);
		}
	};

	/// <summary>
	/// A utility class to embed another <see cref="Generator"/> inside your own.
	/// It takes care of initialization.
	/// <seealso cref="ape::EmbeddedEffect"/>
	/// </summary>
	/// <remarks>
	/// Take care to call <see cref="EmbeddedProcessor{TEffect}::start()"/> on any embedded plugins.
	/// </remarks>
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
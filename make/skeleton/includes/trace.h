#ifndef CPPAPE_TRACE_H
#define CPPAPE_TRACE_H

#ifndef CPPAPE_TRACING_ENABLED

#define TRC(...) __VA_ARGS__

#else
	#include <map>
	#include <vector>
	#include <algorithm>
	#include <complex>

	namespace Tracing
	{

		using Twine = std::pair<const char*, const char*>;

		template<typename T>
		struct Trace
		{
			void reset() noexcept { index = 0; }
			void enqueue(T thing)
			{
				if (index < data.size())
					data[index++] = thing;
				else
				{
					data.resize(std::max<std::size_t>(1, data.size() * 2));
					data[index++] = thing;
				}

			}
			std::size_t index;
			std::vector<T> data;
		};

		class Tracer
		{

		public:

			Trace<float>& getTrace(Twine id) noexcept
			{
				return traces[id];
			}

			void reset() noexcept
			{
				for (auto& pair : traces)
					pair.second.reset();
			}

			const std::map<Twine, Trace<float>>& getTraces() const noexcept { return traces; }

		private:
			std::map<Twine, Trace<float>> traces;
		};

		template<typename Input, typename = std::true_type>
		struct TraceTraits;

		template<typename Input>
		struct TraceTraits<Input, typename std::is_arithmetic<Input>::type>
		{
			static constexpr std::size_t numElements() noexcept { return 1; }
			static constexpr const char* nameForElement(std::size_t index) noexcept { return nullptr; }
			static constexpr float valueForElement(const Input& r, std::size_t index) noexcept { return static_cast<float>(r); }
		};

		template<typename InnerType>
		struct TraceTraits<std::complex<InnerType>>
		{
			static constexpr std::size_t numElements() noexcept { return 2; }
			static constexpr const char* nameForElement(std::size_t index) noexcept { return index == 0 ? "real" : "imag"; }
			static constexpr float valueForElement(const std::complex<InnerType>& r, std::size_t index) noexcept { return index == 0 ? r.real() : r.imag(); }
		};

		template<typename Result>
		inline typename std::enable_if<TraceTraits<Result>::numElements() == 1>::type
			TraceData(std::size_t index, const char* identifier, const Result& result)
		{
			extern Tracer GlobalTracer;

			GlobalTracer
				.getTrace(std::make_pair(identifier, nullptr))
				.enqueue(TraceTraits<Result>::valueForElement(result, 0));
		}

		template<typename Result>
		inline typename std::enable_if<TraceTraits<Result>::numElements() != 1>::type
			TraceData(std::size_t index, const char* identifier, const Result& result)
		{
			extern Tracer GlobalTracer;

			for (std::size_t i = 0; i < TraceTraits<Result>::numElements(); ++i)
			{
				GlobalTracer
					.getTrace(std::make_pair(identifier, TraceTraits<Result>::nameForElement(i)))
					.enqueue(TraceTraits<Result>::valueForElement(result, i));
			}
		}

	}


	#define TRC(...) Tracing::TraceData(0, #__VA_ARGS__, (__VA_ARGS__))

#endif

#endif
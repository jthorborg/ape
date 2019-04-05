#ifndef CPPAPE_MISC_H
#define CPPAPE_MISC_H

#include <cstdint>
#include <vector>
#include <assert.h>
#include "interpolation.h"

namespace ape
{
	template<typename T>
	struct uarray
	{
	public:

		uarray(std::vector<T>& source) : uarray(source.data(), source.size()) {}

		uarray(T* buffer, std::size_t length)
			: buffer(buffer), length(length)
		{
#ifndef CPPAPE_RELEASE
			assert(buffer);
#endif
		}

		uarray(T* begin, T* end)
			: buffer(begin), length(end - begin)
		{
#ifndef CPPAPE_RELEASE
			assert(begin);
			assert(end);
#endif
		}

		T& operator [] (std::size_t index)
		{
#ifndef CPPAPE_RELEASE
			assert(index < length);
#endif
			return buffer[index];
		}

		const T& operator [] (std::size_t index) const
		{
#ifndef CPPAPE_RELEASE
			assert(index < length);
#endif
			return buffer[index];
		}

		T* begin() noexcept { return buffer; }
		const T* begin() const noexcept { return buffer; }

		T* end() noexcept { return buffer + length; }
		const T* end() const noexcept { return buffer + length; }

		T* data() noexcept { return buffer; }
		const T* data() const noexcept { return buffer; }

		std::size_t size() const noexcept { return length; }

		void clear() noexcept
		{
			std::fill(begin(), end(), T());
		}

	private:

		T* buffer;
		std::size_t length;
	};

	template<typename T>
	struct const_uarray
	{
	public:

		const_uarray(const std::vector<T>& source) : const_uarray(source.data(), source.size()) {}


		const_uarray(const T* buffer, std::size_t length)
			: buffer(buffer), length(length)
		{
#ifndef CPPAPE_RELEASE
			assert(buffer);
#endif
		}

		const_uarray(T* begin, T* end)
			: buffer(begin), length(end - begin)
		{
#ifndef CPPAPE_RELEASE
			assert(begin);
			assert(end);
#endif
		}

		const T& operator [] (std::size_t index) const
		{
#ifndef CPPAPE_RELEASE
			assert(index < length);
#endif
			return buffer[index];
		}

		const T* begin() const noexcept { return buffer; }
		const T* end() const noexcept { return buffer + length; }
		const T* data() const noexcept { return buffer; }

		std::size_t size() const noexcept { return length; }

	private:

		const T* buffer;
		std::size_t length;
	};

	template<typename T>
	struct const_umatrix
	{
		struct const_iterator
		{

			const_iterator(const_umatrix<T> matrix, std::size_t column = 0)
				: source(matrix), column(column)
			{

			}

			const_iterator& operator ++()
			{
				column++;
				return *this;
			}

			const_iterator& operator --()
			{
				column--;
				return *this;
			}

			const_iterator operator ++(int)
			{
				const_iterator copy{ this };
				column++;
				return copy;
			}

			const_iterator operator --(int)
			{
				const_iterator copy{ this };
				column--;
				return copy;
			}

			const uarray<T> operator *()
			{
				return source[column];
			}

			bool operator == (const const_iterator& right) const noexcept
			{
				return column == right.column && source.data == right.source.data;
			}

			bool operator != (const const_iterator& right) const noexcept
			{
				return !(*this == right);
			}

		private:
			const_umatrix<T> source;
			std::size_t column;
		};

		const_umatrix(T* const * data, std::size_t samples, std::size_t channels)
			: data(data), rows(samples), columns(channels)
		{

		}

		const_uarray<T> operator [] (std::size_t channel) const CPPAPE_NOEXCEPT_IF_RELEASE
		{
#ifndef CPPAPE_RELEASE
			assert(channel < columns);
#endif

			return { data[channel], rows };
		}

		const_iterator begin() const noexcept { return { *this }; }
		const_iterator end() const noexcept { return { *this, columns }; }

		std::size_t samples() const noexcept { return rows; }
		std::size_t channels() const noexcept { return columns; }

	protected:

		T* const * data;
		std::size_t rows, columns;
	};

	template<typename T>
	struct umatrix : public const_umatrix<T>
	{
	private:

		using const_umatrix<T>::data;
		using const_umatrix<T>::rows;
		using const_umatrix<T>::columns;

	public:
		struct iterator
		{

			iterator(umatrix<T> matrix, std::size_t column = 0)
				: source(matrix), column(column)
			{

			}

			iterator& operator ++()
			{
				column++;
				return *this;
			}

			iterator& operator --()
			{
				column--;
				return *this;
			}

			iterator operator ++(int)
			{
				iterator copy{ this };
				column++;
				return copy;
			}

			iterator operator --(int)
			{
				iterator copy{ this };
				column--;
				return copy;
			}

			uarray<T> operator *()
			{
				return source[column];
			}

			bool operator == (const iterator& right) const noexcept
			{
				return column == right.column && source.data == right.source.data;
			}

			bool operator != (const iterator& right) const noexcept
			{
				return !(*this == right);
			}

		private:
			umatrix<T> source;
			std::size_t column;
		};

		umatrix(T** data, std::size_t samples, std::size_t channels)
			: const_umatrix<T>(data, samples, channels)
		{

		}

		uarray<T> operator [] (std::size_t channel) CPPAPE_NOEXCEPT_IF_RELEASE
		{
#ifndef CPPAPE_RELEASE
			assert(channel < columns);
#endif

			return { data[channel], rows };
		}

		iterator begin() noexcept { return { *this }; }
		iterator end() noexcept { return { *this, columns }; }

	};

	template<typename T>
	struct DynamicSampleMatrix
	{
		void resize(std::size_t channelCount, std::size_t samples)
		{
			channels.resize(channelCount);
			buffer.resize(channelCount * samples);

			for (std::size_t c = 0; c < channelCount; ++c)
				channels[c] = buffer.data() + c * samples * channelCount;
		}

		operator const umatrix<T>() noexcept
		{
			return umatrix<T> { channels.data(), buffer.size() / channels.size(), channels.size() };
		}

		std::vector<T*> channels;
		std::vector<T> buffer;
	};


	template<typename T>
	struct circular_signal
	{
	public:

		circular_signal(const_uarray<T> source) : source(source) {}

		T operator()(std::int64_t x) const noexcept
		{
			auto mod = x % source.size();
			x = mod < 0 ? mod + source.size() : mod;
			return source[x];
		}

		T operator()(std::int32_t x) const noexcept
		{
			auto mod = x % source.size();
			x = mod < 0 ? mod + source.size() : mod;
			return source[x];
		}

		T operator()(std::uint64_t x) const noexcept
		{
			return source[x % source.size()];
		}

		T operator()(std::uint32_t x) const noexcept
		{
			return source[x % source.size()];
		}

		T operator() (double x) const noexcept
		{
			return hermite4(*this, x);
		}

	private:

		const_uarray<T> source;
	};

	template<typename T>
	struct windowed_signal
	{
	public:

		windowed_signal(const_uarray<T> source) : source(source) {}

		T operator()(std::int64_t x) const noexcept
		{
			if (x >= 0 && x < source.size())
				return source[x];

			return static_cast<T>(0);
		}

		T operator()(std::int32_t x) const noexcept
		{
			if (x >= 0 && x < source.size())
				return source[x];

			return static_cast<T>(0);
		}

		T operator()(std::uint64_t x) const noexcept
		{
			if (x < source.size())
				return source[x];

			return static_cast<T>(0);
		}

		T operator()(std::uint32_t x) const noexcept
		{
			if (x < source.size())
				return source[x];

			return static_cast<T>(0);
		}

		T operator() (double x) const noexcept
		{
			return hermite4(*this, x);
		}

	private:

		const_uarray<T> source;
	};


	template<typename T>
	struct circular_iterator // : std::iterator<std::forward_iterator_tag, T>
	{
	public:

		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using reference = value_type&;
		using pointer = T*;
		using iterator_category = std::forward_iterator_tag;
		using this_type = circular_iterator<T>;

		template<typename Container>
		friend auto cyclic_begin(Container& c, std::size_t offset);

		template<typename Container>
		friend auto cyclic_end(Container& c, std::size_t offset, std::size_t length);

		// LegacyIterator
		reference operator * () noexcept { return base[origin]; }

		this_type operator ++(int) noexcept
		{
			this_type copy(*this);
			++copy;
			return copy;
		}



		// LegacyInputIterator
		this_type& operator ++() noexcept
		{
			if (++origin >= bounds)
				origin -= bounds;

			distance++;

			return *this;
		}

		pointer operator -> () noexcept { return base + origin; }


		// LegacyBidirectionalIterator
		this_type operator --(int) noexcept
		{
			this_type copy(*this);
			--copy;
			return copy;
		}

		// LegacyInputIterator
		this_type& operator --() noexcept
		{
			if (origin == 0)
				origin = bounds - 1;
			else
				origin--;

			distance--;

			return *this;
		}


		friend bool operator == (circular_iterator<T> left, circular_iterator<T> right)
		{
			return
				left.base == right.base &&
				left.distance == right.distance &&
				left.origin == right.origin &&
				left.bounds == right.bounds;
		}

		friend bool operator != (circular_iterator<T> left, circular_iterator<T> right)
		{
			return !(left == right);
		}

		// TODO: Make private when clang supports it
		pointer base;
		std::ptrdiff_t distance;
		std::size_t origin;
		std::size_t bounds;
	};


	template<typename Container>
	auto cyclic_begin(Container& c, std::size_t offset)
	{
		circular_iterator<typename Container::value_type> ret;
		ret.base = c.data();
		ret.distance = 0;
		ret.origin = offset % c.size();
		ret.bounds = c.size();
		return ret;
	}

	template<typename Container>
	auto cyclic_end(Container& c, std::size_t offset, std::size_t length)
	{
		circular_iterator<typename Container::value_type> ret;
		ret.base = c.data();
		ret.distance = length;
		ret.origin = (offset + length) % c.size();
		ret.bounds = c.size();

		return ret;
	}
}

#endif
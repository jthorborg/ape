#ifndef CPPAPE_MISC_H
#define CPPAPE_MISC_H

#include <cstdint>
#include <vector>
#include <assert.h>
#include <type_traits>

#include "interpolation.h"

namespace ape
{
	/// <summary>
	/// An unowned array wrapper - a mutable "view" of something else, that cannot be resized.
	/// Construction parameters are referenced directly, and no copies of data are ever taken / made.
	/// This also means you should take care to ensure referred-to data outlives any uarray. 
	/// Following that, <see cref="uarray"/>s should probably only ever exist on the stack.
	/// </summary>
	/// <typeparam name="T">
	/// The source content type.
	/// Append const to the type for perfectly enforced read-only access to the contents.
	/// </typeparam>
	template<typename T>
	struct uarray
	{
	public:

		/// <summary>
		/// Alias for <typeparamref name="T"/>
		/// </summary>
		typedef T value_type;

		/// <summary>
		/// Construct from a mutable vector.
		/// </summary>
		uarray(std::vector<T>& source) : uarray(source.data(), source.size()) {}
		/// <summary>
		/// Construct a read-only uarray from a const-qualified vector.
		/// </summary>
		uarray(const std::vector<typename std::remove_const_t<T>>& source) : uarray(source.data(), source.size()) {}
		/// <summary>
		/// Construct from a possibly cv-qualified source
		/// </summary>
		uarray(T* buffer, std::size_t length)
			: buffer(buffer), length(length)
		{
#ifndef CPPAPE_RELEASE
			assert(buffer);
#endif
		}
		/// <summary>
		/// Construct from a possibly cv-qualified iterator pair
		/// </summary>
		uarray(T* begin, T* end)
			: buffer(begin), length(end - begin)
		{
#ifndef CPPAPE_RELEASE
			assert(begin);
			assert(end);
#endif
		}

		/// <summary>
		/// Access a potential read-only element at <paramref name="index"/>
		/// </summary>
		T& operator [] (std::size_t index)
		{
#ifndef CPPAPE_RELEASE
			assert(index < length);
#endif
			return buffer[index];
		}

		/// <summary>
		/// Access a read-only element at <paramref name="index"/>
		/// </summary>
		const T& operator [] (std::size_t index) const
		{
#ifndef CPPAPE_RELEASE
			assert(index < length);
#endif
			return buffer[index];
		}

		/// <summary>
		/// Returns an iterator to the beginning of the array pointed to
		/// </summary>
		T* begin() noexcept { return buffer; }
		/// <summary>
		/// Returns a const iterator to the beginning of the array pointed to
		/// </summary>
		const T* begin() const noexcept { return buffer; }
		/// <summary>
		/// Returns an iterator pointing to 1 element past the end of the array pointed to
		/// </summary>
		T* end() noexcept { return buffer + length; }
		/// <summary>
		/// Returns a const iterator pointing to 1 element past the end of the array pointed to
		/// </summary>
		const T* end() const noexcept { return buffer + length; }
		/// <summary>
		/// Retrieve a raw pointer to the array pointed to
		/// </summary>
		T* data() noexcept { return buffer; }
		/// <summary>
		/// Retrieve a const raw pointer to the array pointed to
		/// </summary>
		const T* data() const noexcept { return buffer; }

		/// <summary>
		/// Returns the size of the array pointed to by this <see cref="uarray"/>
		/// </summary>
		std::size_t size() const noexcept { return length; }
		
		/// <summary>
		/// Returns a new, constant uarray formed from a slice of the original.
		/// </summary>
		/// <param name="offset">
		/// How much to skip from the start
		/// </param>
		/// <param name="newLength">
		/// The length of the slice, starting from <paramref name="offset"/>.
		/// The default value adopts the current length, and substract the offset
		/// (ie. the remaining). 
		/// </param>
		uarray<T> slice(std::size_t offset, std::size_t newLength = -1) noexcept
		{
			assert(offset <= length);
			if (newLength == -1)
				newLength = length - offset;

			assert((offset + newLength) <= length);
			return { buffer + offset, newLength };
		}

		template<typename Other>
		typename std::enable_if<std::is_standard_layout<Other>::value && !std::is_const<T>::value, uarray<Other>>::type
			reinterpret() noexcept
		{
			static_assert((sizeof(T) / sizeof(Other)) * sizeof(Other) == sizeof(T), "Other is not divisble by T");
			static_assert(alignof(T) <= alignof(Other), "Other is less aligned than T");

			constexpr auto ratio = sizeof(T) / sizeof(Other);

			return { reinterpret_cast<Other*>(buffer), length * ratio };
		} 

		template<typename Other>
		typename std::enable_if<std::is_standard_layout<Other>::value && std::is_const<T>::value, uarray<const Other>>::type
			reinterpret() const noexcept
		{
			static_assert((sizeof(T) / sizeof(Other)) * sizeof(Other) == sizeof(T), "Other is not divisble by T");
			static_assert(alignof(T) <= alignof(Other), "Other is less aligned than T");

			constexpr auto ratio = sizeof(T) / sizeof(Other);

			return { reinterpret_cast<const Other*>(buffer), length * ratio };
		}

		/// <summary>
		/// Returns a new, constant uarray formed from a slice of the original.
		/// </summary>
		/// <param name="offset">
		/// How much to skip from the start
		/// </param>
		/// <param name="newLength">
		/// The length of the slice, starting from <paramref name="offset"/>.
		/// The default value adopts the current length, and substract the offset
		/// (ie. the remaining). 
		/// </param>
		uarray<const T> slice(std::size_t offset, std::size_t newLength = -1) const noexcept
		{
			assert(offset <= length);

			if (newLength == -1)
				newLength = length - offset;

			assert((offset + newLength) <= length);
			return { buffer + offset, newLength };
		}


		/// <summary>
		/// Implicit conversion operator to a const / read-only version of this <see cref="uarray"/>
		/// </summary>
		operator uarray<const T>() const noexcept
		{
			return { begin(), end() };
		}

	private:

		T* const buffer;
		const std::size_t length;
	};

	/// <summary>
	/// A container representing a 2d rectangular array (and can be used syntactically like one).
	/// As with <see cref="uarray"/>, the source contents is unowned and the dimensionality is constant.
	/// Row-major order, where each "row" is a channel and can be accessed through a <see cref="uarray"/>.
	/// </summary>
	/// <remarks>
	/// This class is a drop-in safe replacements for expressions like:
	/// T** data, int samples, int channels
	/// Or aliasing actually mutable 2d audio sources like <see cref="AudioFile"/> or <see cref="DynamicSampleMatrix"/>.
	/// </remarks>
	/// <typeparam name="T">
	/// The source content type.
	/// Append const to the type for perfectly enforced read-only access to the contents.
	/// </typeparam>
	template<typename T>
	struct umatrix
	{
	public:

		typedef T value_type;

		/// <summary>
		/// Iterator for enumerating the rows of a <see cref="umatrix"/>
		/// <see cref="umatrix{T}::begin()"/>
		/// </summary>
		/// <remarks>
		/// Conforms to LegacyBidirectionalIterator
		/// </remarks>
		struct iterator
		{
			typedef std::ptrdiff_t difference_type;
			typedef T value_type;
			typedef value_type* pointer;
			typedef value_type& reference;
			typedef std::bidirectional_iterator_tag iterator_category;

			iterator(umatrix<T> matrix, std::size_t row = 0)
				: source(matrix), row(row)
			{

			}

			iterator& operator ++()
			{
				row++;
				return *this;
			}

			iterator& operator --()
			{
				row--;
				return *this;
			}

			iterator operator ++(int)
			{
				iterator copy{ this };
				row++;
				return copy;
			}

			iterator operator --(int)
			{
				iterator copy{ this };
				row--;
				return copy;
			}

			uarray<T> operator *()
			{
				return source[row];
			}

			bool operator == (const iterator& right) const noexcept
			{
				return row == right.row && source.data == right.source.data;
			}

			bool operator != (const iterator& right) const noexcept
			{
				return !(*this == right);
			}

		private:
			umatrix<T> source;
			std::size_t row;
		};

		/// <summary>
		/// Construct from a pointer array.
		/// </summary>
		umatrix(T * const * data, std::size_t channelsOrRows, std::size_t samplesOrColumns)
			: data(data)
			, numRows(channelsOrRows)
			, numColumns(samplesOrColumns)
		{

		}

		/// <summary>
		/// Access the <paramref name="row"/> in the matrix.
		/// </summary>
		/// <returns>
		/// A <see cref="uarray"/> aliasing the contents.
		/// </returns>
		uarray<T> operator [] (std::size_t row) const CPPAPE_NOEXCEPT_IF_RELEASE
		{
#ifndef CPPAPE_RELEASE
			assert(row < numRows);
#endif

			return { data[row], numColumns };
		}

		/// <summary>
		/// Retrieve an <see cref="iterator"/> that can enumerate channels contained in this matrix.
		/// </summary>
		iterator begin() const noexcept { return { *this }; }
		/// <summary>
		/// Retrieve an <see cref="iterator"/> pointing to one past the last channel in this matrix.
		/// <seealso cref="begin()"/>
		/// </summary>
		iterator end() const noexcept { return { *this, numRows }; }

		/// <returns>
		/// How many columns / samples are in this matrix
		/// </returns>
		std::size_t samples() const noexcept { return numColumns; }
		/// <returns>
		/// How many rows / channels are in this matrix
		/// </returns>
		std::size_t channels() const noexcept { return numRows; }
		/// <summary>
		/// <seealso cref="channels()"/>
		/// </summary>
		std::size_t rows() const noexcept { return numRows; }
		/// <summary>
		/// <seealso cref="samples()"/>
		/// </summary>
		std::size_t columns() const noexcept { return numColumns; }

		/// <summary>
		/// Returns a possibly cv-qualified <typeparamref name="T"/>* const*
		/// </summary>
		auto pointers() { return data; }
		const auto pointers() const noexcept { return data; }

		/// <summary>
		/// Implicit conversion operator to a constant read-only version of this <see cref="umatrix"/>
		/// </summary>
		operator umatrix<const T> () const noexcept 
		{
			return { data, numColumns, numRows };
		}

	protected:
		T* const * data;
		std::size_t numRows, numColumns;
	};

	/// <summary>
	/// An owned 2d rectangular matrix that supports a T** representation and being aliased as a <see cref="umatrix"/>.
	/// </summary>
	template<typename T>
	struct DynamicSampleMatrix
	{
		/// <summary>
		/// Retranslate the buffers and contents to adhere to the dimensionality given by the arguments.
		/// No memory reallocation done if there's capacity enough.
		/// </summary>
		void resize(std::size_t channelCount, std::size_t samples)
		{
			channels.resize(channelCount);
			buffer.resize(channelCount * samples);

			for (std::size_t c = 0; c < channelCount; ++c)
				channels[c] = buffer.data() + c * samples;
		}

		/// <summary>
		/// Implicitly alias as a mutable <see cref="umatrix"/>.
		/// <seealso cref="asMatrix"/>
		/// </summary>
		operator umatrix<T>() noexcept
		{
			return umatrix<T> { channels.data(), channels.size(), buffer.size() / channels.size() };
		}

		/// <summary>
		/// Alias as a read-only <see cref="umatrix"/>.
		/// </summary>
		operator umatrix<const T>() const noexcept
		{
			return umatrix<const T> { channels.data(), channels.size(), buffer.size() / channels.size() };
		}

		/// <summary>
		/// Explicitly alias as a mutable <see cref="umatrix"/>.
		/// </summary>
		umatrix<T> asMatrix() noexcept
		{
			return umatrix<T> { channels.data(), channels.size(), buffer.size() / channels.size() };
		}

		std::vector<T*> channels;
		std::vector<T> buffer;
	};

	/// <summary>
	/// An infinitely indexable read-only signal that repeats the original signal.
	/// Supports signed and unsigned integer indices or hermite-interpolated fractional indices.
	/// </summary>
	template<typename T>
	struct circular_signal
	{
	public:

		circular_signal(uarray<const T> source) : source(source) {}

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

		/// <summary>
		/// Interpolates using <see cref="hermite4"/>
		/// </summary>
		T operator() (double x) const noexcept
		{
			return hermite4(*this, x);
		}

	private:

		uarray<const T> source;
	};

	/// <summary>
	/// An infinitely indexable read-only signal that windows the original signal 
	/// (ie. it is zero outside of the bounds of the source material).
	/// Supports signed and unsigned integer indices or hermite-interpolated fractional indices.
	/// </summary>
	template<typename T>
	struct windowed_signal
	{
	public:

		windowed_signal(uarray<const T> source) : source(source) {}

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

		/// <summary>
		/// Interpolates using <see cref="hermite4"/>
		/// </summary>
		T operator() (double x) const noexcept
		{
			return hermite4(*this, x);
		}

	private:

		uarray<const T> source;
	};

	/// <summary>
	/// LegacyForwardIterator with capability of iterating N steps around a flat source with an offset.
	/// In other words, if exceeding the "end" of any referenced container, it will wrap around and start
	/// from the beginning again.
	/// <seealso cref="cyclic_begin"/>
	/// <seealso cref="cyclic_end"/>
	/// </summary>
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

	private:
		pointer base;
		std::ptrdiff_t distance;
		std::size_t origin;
		std::size_t bounds;
	};


	/// <summary>
	/// Constructs a suitable beginning iterator of a cyclic iteration on <paramref name="c"/>.
	/// Iteration effectively wraps around, until it compares equal to something returned by <see cref="cyclic_end"/>.
	/// </summary>
	/// <param name="c">
	/// Any "container" supporting size() and data()
	/// </param>
	/// <param name="offset">
	/// Where to start in <paramref name="c"/>.
	/// </param>
	template<typename Container>
	inline auto cyclic_begin(Container& c, std::size_t offset)
	{
		circular_iterator<typename Container::value_type> ret;
		ret.base = c.data();
		ret.distance = 0;
		ret.origin = offset % c.size();
		ret.bounds = c.size();
		return ret;
	}

	/// <summary>
	/// Specifies an "end" to <see cref="cyclic_begin"/>
	/// </summary>
	/// <param name="c">
	/// Any "container" supporting size() and data()
	/// </param>
	/// <param name="offset">
	/// Where to start in <paramref name="c"/>. Must match the parameter given to <see cref="cyclic_begin"/>!
	/// </param>
	/// <param name="length">
	/// How many iterations to be performed before the pair compares equal.
	/// </param>
	template<typename Container>
	inline auto cyclic_end(Container& c, std::size_t offset, std::size_t length)
	{
		circular_iterator<typename Container::value_type> ret;
		ret.base = c.data();
		ret.distance = length;
		ret.origin = (offset + length) % c.size();
		ret.bounds = c.size();

		return ret;
	}

	/// <summary>
	/// Clear a <see cref="uarray"/> of non-const qualified <typeparamref name="T"/> elements to a default-initialized value. 
	/// </summary>
	template<typename T>
	inline void clear(std::vector<T>& arr) noexcept
	{
		std::fill(arr.begin(), arr.end(), T());
	}

	/// <summary>
	/// Clear a <see cref="uarray"/> of non-const qualified <typeparamref name="T"/> elements to a default-initialized value. 
	/// </summary>
	template<typename T>
	inline typename std::enable_if<!std::is_const_v<T>>::type 
		clear(uarray<T> arr) noexcept
	{
		std::fill(arr.begin(), arr.end(), T());
	}

	/// <summary>
	/// Clear a <see cref="umatrix"/> of non-const qualified <typeparamref name="T"/> elements to a default-initialized value. 
	/// </summary>
	template<typename T>
	inline typename std::enable_if<!std::is_const_v<T>>::type
		clear(umatrix<T> mat, std::size_t offset = 0) noexcept
	{
		for (std::size_t c = offset; c < mat.channels(); ++c)
		{
			clear(mat[c]);
		}
	}

	template<typename T>
	inline uarray<T> as_uarray(std::vector<T>& vec)
	{
		return { vec };
	}

	template<typename T>
	inline uarray<const T> as_uarray(const std::vector<T>& vec)
	{
		return { vec };
	}

	template<typename T>
	inline uarray<T> as_uarray(T* data, std::size_t size)
	{
		return { data, size };
	}

	template<typename T>
	inline uarray<const T> as_uarray(const T* data, std::size_t size)
	{
		return { data, size };
	}
}

#endif
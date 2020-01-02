#ifndef CPPAPE_PRINT_H
#define CPPAPE_PRINT_H

#include "baselib.h"

#include <cstddef>
#include <charconv>
#include <complex>

int printf(const char * fmt, ...);
extern "C" int sprintf(char* buffer, const char* fmt, ...);

namespace std
{
	using ::printf;
	using ::sprintf;
}

#define CPPAPE_CONCAT_IMPL( x, y ) x##y
#define CPPAPE_MACRO_CONCAT( x, y ) CPPAPE_CONCAT_IMPL( x, y )

namespace ape
{
	/*
	 */

	namespace detail
	{
		template<typename T>
		inline char* format(const T& val, char* b, char* e)
		{
#define __CPPAPE_FORMAT_ERROR "<format error>"

			using namespace std;
			auto result = to_chars(b, e, val);

			if (result.ec != std::errc())
			{
				memcpy(b, __CPPAPE_FORMAT_ERROR, sizeof(__CPPAPE_FORMAT_ERROR) - 1);
				return b + sizeof(__CPPAPE_FORMAT_ERROR) - 1;
			}

#undef __CPPAPE_FORMAT_ERROR

			return result.ptr;
		}

		template<typename T>
		inline char* format(const std::complex<T>& val, char* b, char* e)
		{
			return b + std::sprintf(b, "(%f, %fi)", val.real(), val.imag());
		}

		inline char* format(const long double& val, char* b, char* e)
		{
			return b + std::sprintf(b, "%f", (double)val);
		}

		inline char* format(const double& val, char* b, char* e)
		{
			return b + std::sprintf(b, "%f", val);
		}

		inline char* format(const float& val, char* b, char* e)
		{
			return b + std::sprintf(b, "%f", val);
		}

		inline char* format(const void* val, char* b, char* e)
		{
			return b + std::sprintf(b, "0x%p", val);
		}

		inline char* format(std::string_view val, char* b, char* e)
		{
			assert(e > (b + val.size()));
			std::memcpy(b, val.data(), val.size());
			return b + val.size();
		}

		inline char* format(const char* val, char* b, char* e)
		{
			return format(std::string_view(val), b, e);
		}

		inline char* format(const std::string& val, char* b, char* e)
		{
			return format(std::string_view(val), b, e);
		}

		struct print_buffer
		{
			static constexpr std::size_t size = 4096;

			void putc(char c)
			{
				if (begin() == end())
					return;

				*begin() = c;
				current++;
			}

			void seal()
			{
				*begin() = '\0';
			}

			char* begin() { return current; }
			char* end() { return buffer + size; }

			char buffer[size];
			char* current = buffer;
		};

		template<typename T>
		inline print_buffer& operator << (print_buffer& left, const T& value)
		{
			left.current = format(value, left.begin(), left.end());
			return left;
		}

		inline void safe_printf(print_buffer& buffer, std::string_view view)
		{
			for (auto it = view.begin(); it != view.end(); ++it)
			{
				if (*it == '%')
				{
					auto next = it;
					next++;
					if (next != view.end() && *next == '%')
					{
						it = next;
					}
					else
					{
						abort("too few arguments for print()");
					}
				}

				buffer.putc(*it);
			}
		}

		template<typename T, typename... Args>
		inline void safe_printf(print_buffer& buffer, std::string_view view, const T& value, Args&&... args)
		{
			for (auto it = view.begin(); it != view.end(); ++it)
			{
				if (*it == '%')
				{
					auto next = it;
					next++;
					if (next != view.end() && *next == '%')
					{
						it = next;
					}
					else
					{
						buffer << value;
						safe_printf(buffer, view.substr(std::distance(view.begin(), next)), std::forward<Args>(args)...); // call even when *s == 0 to detect extra arguments
						return;
					}
				}

				buffer.putc(*it);
			}

			abort("too many arguments provided to print()");
		}
	}

	template<typename Derived>
	struct PrintFormatter
	{
		friend std::to_chars_result to_chars(char* begin, char* end, const Derived& type)
		{
			if (!type.format(begin, end, &begin))
				return { end, std::errc::value_too_large };

			return { begin, std::errc() };
		}
	};

}

template<typename... Args>
inline void print(std::string_view fmt, Args&&... args)
{
	ape::detail::print_buffer stackBuffer;
	ape::detail::safe_printf(stackBuffer, fmt, std::forward<Args>(args)...);

	stackBuffer.seal();

	printf("%s", stackBuffer.buffer);
}

#define __cppape_printf_once(cname, ...) \
	static bool cname = false; \
	if (!cname) \
	{	\
		cname = true; \
		printf(__VA_ARGS__); \
	}

#define printf_once(...) __cppape_printf_once(CPPAPE_MACRO_CONCAT(__once_flag, __COUNTER__), __VA_ARGS__)

#define __cppape_print_once(cname, ...) \
	static bool cname = false; \
	if (!cname) \
	{	\
		cname = true; \
		print(__VA_ARGS__); \
	}

#define print_once(...) __cppape_print_once(CPPAPE_MACRO_CONCAT(__once_flag, __COUNTER__), __VA_ARGS__)

#endif
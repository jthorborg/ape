#ifndef CPPAPE_PRINT_H
#define CPPAPE_PRINT_H

#include "baselib.h"

#include <cstddef>
/*#include <sstream>
#include <string_view> 
#include <charconv>*/

int printf(const char * fmt, ...);

#define CPPAPE_CONCAT_IMPL( x, y ) x##y
#define CPPAPE_MACRO_CONCAT( x, y ) CPPAPE_CONCAT_IMPL( x, y )
#if 0 
namespace ape
{

	struct print_buffer
	{
		char* reserve(std::size_t n)
		{

		}

		void advance(std::size_t n)
		{

		}
	};

	namespace detail
	{
		void format(print_buffer& buffer, )

		void safe_printf(std::ostringstream& buffer, std::string_view view)
		{
			for (auto it = view.begin(); it != view.end(); ++it)
			{
				auto c = *it;

				if (c == '%')
				{
					auto next = it++;
					if (next != view.end() && *next == '%')
					{
						it = next;
					}
					else
					{
						abort("too few arguments for print()");
					}
				}

				buffer << c;
			}
		}

		template<typename T, typename... Args>
		void safe_printf(std::ostringstream& buffer, std::string_view view, T& value, Args&&... args)
		{
			for (auto it = view.begin(); it != view.end(); ++it)
			{
				auto c = *it;

				if (c == '%')
				{
					auto next = it++;
					if (next != view.end() && *next == '%')
					{
						it = next;
					}
					else
					{
						buffer << value;
						safe_printf(buffer, view.substr(std::distance(it, view.begin())), std::forward(args)...); // call even when *s == 0 to detect extra arguments
					}
				}

				buffer << c;
			}

			abort("too many arguments provided to print()");
		}

		template<std::size_t Size>
		struct StaticReadBuf : public std::streambuf
		{
			char buffer[Size];

			StaticReadBuf()
			{
				setg(buffer, buffer, buffer + Size);
			}

			void seal()
			{
				*gptr() = '\0';
			}
		};
	}
}

template<typename T, typename... Args>
void print(std::string_view fmt, Args&&... args)
{
	detail::StaticReadBuf<4096> stackBuffer;

	{
		std::ostringstream buffer(&stackBuffer);
		detail::safe_printf(buffer, fmt, std::forward(args)...);
	}

	stackBuffer.seal();

	printf("%s", stackBuffer.buffer);
}

#endif

#define printf_once(...) \
	static bool CPPAPE_MACRO_CONCAT(__once_flag, __COUNTER__) = false; \
	if(!(CPPAPE_MACRO_CONCAT(__once_flag, __COUNTER__))) \
	{	\
		CPPAPE_MACRO_CONCAT(__once_flag, __COUNTER__) = true; \
		printf(__VA_ARGS__); \
	}

#endif
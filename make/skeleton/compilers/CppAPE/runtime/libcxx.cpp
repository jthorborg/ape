#include <cstdarg>
#include <cstddef>

void *operator new(std::size_t am);
void operator delete(void * loc);

extern "C"
{
	int vsnprintf(char * s, size_t n, const char * format, va_list arg);
	int snprintf(char * s, size_t n, const char * format, ...);

	void abort();
};

int printf(const char * fmt, ...);


#include "libcxx-src/string.cpp"
#include "libcxx-src/exception.cpp"
#include "libcxx-src/vector.cpp"
#include "libcxx-src/stdexcept.cpp"
#include "libcxx-src/typeinfo.cpp"
#include "libcxx-src/charconv.cpp"
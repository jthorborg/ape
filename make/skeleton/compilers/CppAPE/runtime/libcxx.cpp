#include <cstdarg>
#include <cstddef>
#include <new>

namespace ape
{
	void* memoryAlloc(std::size_t am, std::size_t align);
	void memoryFree(void* loc);
}

void *operator new(std::size_t am)
{
	return ape::memoryAlloc(am, 64);
}

void *operator new(std::size_t am, std::align_val_t align)
{
	return ape::memoryAlloc(am, static_cast<std::size_t>(align));
}

void operator delete(void * loc) noexcept
{
	return ape::memoryFree(loc);
}

void operator delete(void * loc, std::align_val_t align) noexcept
{
	return ape::memoryFree(loc);
}

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
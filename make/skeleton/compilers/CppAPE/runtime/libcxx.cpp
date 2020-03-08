#include <cstdarg>
#include <cstddef>
#include <new>
#include <__debug>

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

int printf(const char * fmt, ...);
void abort(const char* reason);

extern "C"
{
	int vsnprintf(char * s, size_t n, const char * format, va_list arg);
	int snprintf(char * s, size_t n, const char * format, ...);
	void abort()
	{
		abort("unknown fatal libc++ error");
	}
};

_LIBCPP_BEGIN_NAMESPACE_STD

_LIBCPP_NORETURN void __ape_abort_trampoline(__libcpp_debug_info const& info)
{
	::printf(
		"%s:%d: _LIBCPP_ASSERT '%s' failed. %s",
		info.__file_,
		info.__line_,
		info.__pred_,
		info.__msg_
	);

	::abort("assertion error");
}

_LIBCPP_SAFE_STATIC __libcpp_debug_function_type
__libcpp_debug_function = __ape_abort_trampoline;

_LIBCPP_END_NAMESPACE_STD

#include "libcxx-src/string.cpp"
#include "libcxx-src/exception.cpp"
#include "libcxx-src/vector.cpp"
#include "libcxx-src/stdexcept.cpp"
#include "libcxx-src/typeinfo.cpp"
#include "libcxx-src/charconv.cpp"
#ifndef CPPAPE_FPOINT_H
#define CPPAPE_FPOINT_H

namespace ape
{
#ifndef __CPPAPE_PRECISION__
#define __CPPAPE_PRECISION__ 32
#endif

#if __CPPAPE_PRECISION__ == 32
	/// <summary>
	/// A typedef for a data type supporting any floating point operation,
	/// with variable compile-time precision.
	/// The user has the capability to change this to a double for instance.
	/// This is a quick way to "template" your code, and hot-recompile it with
	/// different precision to evaluate precision / performance.
	/// <seealso cref="consts{T}"/>
	/// </summary>
	/// <remarks>
	/// Default is 32-bits (ie. a float)
	/// </remarks>
	typedef float fpoint;
#elif __CPPAPE_PRECISION__ == 64
	typedef double fpoint;
#elif __CPPAPE_PRECISION__ == 80
	typedef long double fpoint;
#endif

	/// <summary>
	/// Alias for <see cref="fpoint"/>
	/// </summary>
	using float_t = fpoint;
}

#endif
#ifndef CPPAPE_FPOINT_H
#define CPPAPE_FPOINT_H

namespace ape
{
#ifndef __CPPAPE_PRECISION__
#define __CPPAPE_PRECISION__ 32
#endif

#if __CPPAPE_PRECISION__ == 32
	typedef float fpoint;
#elif __CPPAPE_PRECISION__ == 64
	typedef double fpoint;
#elif __CPPAPE_PRECISION__ == 80
	typedef long double fpoint;
#endif

	using float_t = fpoint;
}

#endif
#ifndef CPPAPE_MATHLIB_H
#define CPPAPE_MATHLIB_H

#include <limits.h>
#include <stdint.h>
#include <float.h>
//#define HAS_F_VERSIONS


#ifdef HAS_F_VERSIONS

#define MATH_FUNC1(name) \
	extern "C" double name(double x1); \
	extern "C" float name ## f(float x1); \
	inline float name(float x) { return name ## f(x); }

#define MATH_FUNC2(name) \
	extern "C" double name(double x1, double y2); \
	extern "C" float name ## f(float x1, float y2); \
	inline float name(float x, float y) { return name ## f(x, y); }

#else

#define MATH_FUNC1(name) \
	extern "C" double name(double x1); \
	inline float name(float x) { return (float)name((double)x); }

#define MATH_FUNC2(name) \
	extern "C" double name(double x1, double y2); \
	inline float name(float x, float y) { return (float)name((double)x, (double)y); }

#endif

#define MATH_TRIG(name) \
	MATH_FUNC1(name); \
	MATH_FUNC1(a ## name); \
	MATH_FUNC1(name ## h); \
	MATH_FUNC1(a ## name ## h);

// basic
MATH_FUNC1(ceil);
MATH_FUNC1(floor);
MATH_FUNC1(round);
MATH_FUNC2(mod);
MATH_FUNC2(fmin);
MATH_FUNC2(fmax);
MATH_FUNC2(remainder);
MATH_FUNC2(copysign);

// trigonometry
MATH_TRIG(sin);
MATH_TRIG(cos);
MATH_TRIG(tan);
MATH_FUNC2(atan2);

// exponential
MATH_FUNC1(exp);
MATH_FUNC1(log);
MATH_FUNC1(log10);
MATH_FUNC1(log2);

// power
MATH_FUNC1(sqrt);
MATH_FUNC2(pow);
MATH_FUNC2(hypot);

// error gamma
MATH_FUNC1(erf);
MATH_FUNC1(tgamma);
MATH_FUNC1(erfc);
MATH_FUNC1(lgamma);


extern "C"
{
	#define M_E 2.71828182845904523536
	#define M_LOG2E 1.44269504088896340736
	#define M_LOG10E 0.434294481903251827651
	#define M_LN2 0.693147180559945309417
	#define M_LN10 2.30258509299404568402
	#define M_PI 3.14159265358979323846
	#define M_PI_2 1.57079632679489661923
	#define M_PI_4 0.785398163397448309616
	#define M_1_PI 0.318309886183790671538
	#define M_2_PI 0.636619772367581343076
	#define M_2_SQRTPI 1.12837916709551257390
	#define M_SQRT2 1.41421356237309504880
	#define M_SQRT1_2 0.707106781186547524401

	double j0(double _X);
	double j1(double _X);
	double jn(int _X, double _Y);
	double y0(double _X);
	double y1(double _X);
	double yn(int _X, double _Y);

	double fabs(double x);
};

#define NAN (0.0F/0.0F)
#define HUGE_VALF (1.0F/0.0F)
#define HUGE_VALL (1.0L/0.0L)
#define INFINITY (1.0F/0.0F)


#define FP_NAN		0x0100
#define FP_NORMAL	0x0400
#define FP_INFINITE	(FP_NAN | FP_NORMAL)
#define FP_ZERO		0x4000
#define FP_SUBNORMAL	(FP_NORMAL | FP_ZERO)
/* 0x0200 is signbit mask */


extern int fpclassify(double x);
extern int fpclassify(float x);

/*
We can't __CRT_INLINE float or double, because we want to ensure truncation
to semantic type before classification.
(A normal long double value might become subnormal when
converted to double, and zero when converted to float.)
*/

/* 7.12.3.2 */
#define isfinite(x) ((fpclassify(x) & FP_NAN) == 0)

/* 7.12.3.3 */
#define isinf(x) (fpclassify(x) == FP_INFINITE)

/* 7.12.3.4 */
/* We don't need to worry about truncation here:
A NaN stays a NaN. */
#define isnan(x) (fpclassify(x) == FP_NAN)

/* 7.12.3.5 */
#define isnormal(x) (fpclassify(x) == FP_NORMAL)


#define ABS_O(type) inline type abs(type val) { return val > 0 ? val : -val; }

ABS_O(char);
ABS_O(short);
ABS_O(int);
ABS_O(long);
ABS_O(long long);

inline float abs(float val) { return (float)fabs((double)val); }
inline double abs(double val) { return fabs(val); }

/*
extern "C"
{
	//float fabs(float val);
	double abs(double val);

	//float powf(float base, float exp);
	double pow(double base, double exp);

	double floor(double val);
} 

float abs(float val)
{
	return abs(val);
}

float pow(float base, float exp)
{
	return (float)pow((double)base, (double)exp);
}

float floor(float val)
{
	return (float)floor((double)val);
}
*/

#endif
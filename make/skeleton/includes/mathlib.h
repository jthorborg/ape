#ifndef CPPAPE_MATHLIB_H
#define CPPAPE_MATHLIB_H

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
	double j0(double _X);
	double j1(double _X);
	double jn(int _X, double _Y);
	double y0(double _X);
	double y1(double _X);
	double yn(int _X, double _Y);
};


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
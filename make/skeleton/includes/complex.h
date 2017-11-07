#ifndef CPPAPE_COMPLEX_H
#define CPPAPE_COMPLEX_H

#include "mathlib.h"

template<class T>
class complex
{
public:
	
	complex(T re = 0, T im = 0)
		: re(re), im(im)
	{

	}

	complex(const complex<T>& other)
		: re(other.re), im(other.im)
	{

	}

	complex<T>& operator = (const complex<T>& other)
	{
		re = other.re;
		im = other.im;
		return *this;
	}

	complex<T>& operator = (const T& value)
	{
		re = value;
		im = 0;
		return *this;
	}


	T real() const { return re; }
	T imag() const { return im; }

	complex<T>& operator+=(const T& other)
	{
		re += other;
		return *this;
	}

	complex<T>& operator+=(const complex<T>& other)
	{
		re += other.re;
		im += other.im;

		return *this;
	}

	complex<T>& operator-=(const T& other)
	{
		re -= other;
		return *this;
	}

	complex<T>& operator-=(const complex<T>& other)
	{
		re -= other.re;
		im -= other.im;
		return *this;
	}
	
	complex<T>& operator*=(const T& other)
	{
		re *= other;
		im *= other;

		return *this;
	}

	complex<T>& operator*=(const complex<T>& other)
	{
		const T x = re * other.re - im * other.im;
		im = re * other.im + im * other.re;
		re = x;

		return *this;
	}

	complex<T>& operator/=(const T& other)
	{
		re /= other;
		im /= other;

		return *this;
	}

	complex<T>& operator/=(const complex<T>& other)
	{
		T conda = (other.im < 0 ? -other.im : +other.im);
		T condb = (other.re < 0 ? -other.re : +other.re);

		if (conda < condb)
		{
			const T wr = other.im / other.re;
			const T wd = other.re + wr * other.im;

			const T x = (re + re * wr) / wd;
			re = (re - re * wr) / wd;
			re = x;

		}
		else
		{
			const T wr = other.re / other.im;
			const T wd = other.im + wr * other.re;

			const T x = (re * wr + re) / wd;
			re = (re * wr - re) / wd;
			re = x;
		}

		return *this;
	}

private:
	T re, im;
};

template<class T>
inline bool operator == (const complex<T>& left, const complex<T>& right)
{
	return left.real() == right.real() && left.imag() == right.imag();
}

template<class T, class Y>
inline bool operator == (const complex<T>& left, const Y& right)
{
	return left.real() == right && left.imag() == 0;
}

template<class T, class Y>
inline bool operator == (const Y& left, const complex<T>& right)
{
	return left == right.real() && right.imag() == 0;
}

template<class T>
inline bool operator != (const complex<T>& left, const complex<T>& right)
{
	return !(left == right);
}

template<class T, class Y>
inline bool operator != (const complex<T>& left, const Y& right)
{
	return !(left == right);
}

template<class T, class Y>
inline bool operator != (const Y& left, const complex<T>& right)
{
	return !(left == right);
}

#define CMPLX_OP(op) \
template<class T, class Y> \
inline complex<T> operator op (complex<T> left, const Y& right) { return left op ## = right; } \
template<class T, class Y> \
inline complex<T> operator op (const Y& left, complex<T> right) { return right op ## = left; }

CMPLX_OP(+);
CMPLX_OP(-);
CMPLX_OP(*);
CMPLX_OP(/);

template<class T>
inline complex<T> operator+(const complex<T>& val) { return val; }
template<class T>
inline complex<T> operator-(const complex<T>& val) { return complex<T>(-val.real(), -val.imag()); }

template<class T>
T real(const complex<T>& val) { return val.real(); }

template<class T>
T real(const T& val) { return val; }

template<class T>
T imag(const complex<T>& val) { return val.imag(); }

template<class T>
T imag(const T& val) { return val; }

template<class T>
T abs(const complex<T>& val) { return hypot(val.real(), val.imag()); }

template<class T>
T arg(const complex<T>& val) { return atan2(val.imag(), val.real()); }

template<class T>
complex<T> polar(const T& r) { return complex<T>(cos(r), sin(r)); }

template<class T>
complex<T> polar(const T& r, const T& theta) { return complex<T>(cos(r), sin(r)) * theta; }

template<class T>
complex<T> conj(const complex<T>& val) { return complex<T>(val.real(), -val.imag()); }

template<class T>
complex<T> norm(const complex<T>& val) { return isinf(val.real()) ? abs(val.imag()) : isinf(val.imag()) ? abs(val.real()) : complex<T>(val.real() * val.real(), val.imag() * val.imag()); }

template<class T>
complex<T> proj(const complex<T>& val) { return isinf(val.real()) || isinf(val.imag()) ? complex<T>((T)INFINITY, abs(left.imag())) : val; }

// from libc++

template<class T>
complex<T> sqrt(const complex<T>& val)
{
	if (isinf(val.imag()))
		return complex<T>(T(INFINITY), val.imag());

	if (isinf(val.real()))
	{
		if (val.real() > T(0))
			return complex<T>(val.real(), isnan(val.imag()) ? val.imag() : copysign(T(0), val.imag()));
		return complex<T>(isnan(val.imag()) ? val.imag() : T(0), copysign(val.real(), val.imag()));
	}

	return polar(sqrt(abs(val)), arg(val) / T(2));
}

template<class T>
complex<T> log(const complex<T>& x)
{
	return complex<T>(log(abs(x)), arg(x));
}

template<class T>
complex<T> log10(const complex<T>& x)
{
	return log(x) / log(T(10));
}

template<class T>
complex<T> exp(const complex<T>& val)
{
	T im = val.imag();
	if (isinf(val.real()))
	{
		if (val.real() < T(0))
		{
			if (!isfinite(im))
				im = T(1);
		}
		else if (im == 0 || !isfinite(im))
		{
			if (isinf(im))
				im = T(NAN);
			return complex<T>(val.real(), im);
		}
	}
	else if (isnan(val.real()) && val.imag() == 0)
		return val;

	T expn = exp(val.real());
	return complex<T>(expn * cos(im), expn * sin(im));
}

template<class T>
complex<T> pow(const complex<T>& x, const complex<T>& y)
{
	return exp(y * log(x));
}

#define __PIT(T) ((T)M_PI)

template<class T>
complex<T> asinh(const complex<T>& val)
{
	if (isinf(val.real()))
	{
		if (isnan(val.imag()))
			return val;
		if (isinf(val.imag()))
			return complex<T>(val.real(), copysign(__PIT(T) * T(0.25), val.imag()));
		return complex<T>(val.real(), copysign(T(0), val.imag()));
	}
	if (isnan(val.real()))
	{
		if (isinf(val.imag()))
			return complex<T>(val.imag(), val.real());
		if (val.imag() == 0)
			return val;
		return complex<T>(val.real(), val.real());
	}
	if (isinf(val.imag()))
		return complex<T>(copysign(val.imag(), val.real()), copysign(__PIT(T) / T(2), val.imag()));
	complex<T> z = log(val + sqrt(pow(val, T(2)) + T(1)));
	return complex<T>(copysign(z.real(), val.real()), copysign(z.imag(), val.imag()));
}

template<class T>
complex<T> acosh(const complex<T>& val)
{
	if (isinf(val.real()))
	{
		if (isnan(val.imag()))
			return complex<T>(abs(val.real()), val.imag());
		if (isinf(val.imag()))
		{
			if (val.real() > 0)
				return complex<T>(val.real(), copysign(__PIT(T) * T(0.25), val.imag()));
			else
				return complex<T>(-val.real(), copysign(__PIT(T) * T(0.75), val.imag()));
		}
		if (val.real() < 0)
			return complex<T>(-val.real(), copysign(__PIT(T), val.imag()));
		return complex<T>(val.real(), copysign(T(0), val.imag()));
	}
	if (isnan(val.real()))
	{
		if (isinf(val.imag()))
			return complex<T>(abs(val.imag()), val.real());
		return complex<T>(val.real(), val.real());
	}
	if (isinf(val.imag()))
		return complex<T>(abs(val.imag()), copysign(__PIT(T) / T(2), val.imag()));
	complex<T> z = log(val + sqrt(pow(val, T(2)) - T(1)));
	return complex<T>(copysign(z.real(), T(0)), copysign(z.imag(), val.imag()));
}

template<class T>
complex<T> atanh(const complex<T>& val)
{
	if (isinf(val.imag()))
	{
		return complex<T>(copysign(T(0), val.real()), copysign(__PIT(T) / T(2), val.imag()));
	}
	if (isnan(val.imag()))
	{
		if (isinf(val.real()) || val.real() == 0)
			return complex<T>(copysign(T(0), val.real()), val.imag());
		return complex<T>(val.imag(), val.imag());
	}
	if (isnan(val.real()))
	{
		return complex<T>(val.real(), val.real());
	}
	if (isinf(val.real()))
	{
		return complex<T>(copysign(T(0), val.real()), copysign(__PIT(T) / T(2), val.imag()));
	}
	if (abs(val.real()) == T(1) && val.imag() == T(0))
	{
		return complex<T>(copysign(T(INFINITY), val.real()), copysign(T(0), val.imag()));
	}
	complex<T> z = log((T(1) + val) / (T(1) - val)) / T(2);
	return complex<T>(copysign(z.real(), val.real()), copysign(z.imag(), val.imag()));
}

template<class T>
complex<T> sinh(const complex<T>& val)
{
	if (isinf(val.real()) && !isfinite(val.imag()))
		return complex<T>(val.real(), T(NAN));
	if (val.real() == 0 && !isfinite(val.imag()))
		return complex<T>(val.real(), T(NAN));
	if (val.imag() == 0 && !isfinite(val.real()))
		return val;
	return complex<T>(sinh(val.real()) * cos(val.imag()), cosh(val.real()) * sin(val.imag()));
}

template<class T>
complex<T> cosh(const complex<T>& val)
{
	if (isinf(val.real()) && !isfinite(val.imag()))
		return complex<T>(abs(val.real()), T(NAN));
	if (val.real() == 0 && !isfinite(val.imag()))
		return complex<T>(T(NAN), val.real());
	if (val.real() == 0 && val.imag() == 0)
		return complex<T>(T(1), val.imag());
	if (val.imag() == 0 && !isfinite(val.real()))
		return complex<T>(abs(val.real()), val.imag());
	return complex<T>(cosh(val.real()) * cos(val.imag()), sinh(val.real()) * sin(val.imag()));
}

template<class T>
complex<T> tanh(const complex<T>& val)
{
	if (isinf(val.real()))
	{
		if (!isfinite(val.imag()))
			return complex<T>(T(1), T(0));
		return complex<T>(T(1), copysign(T(0), sin(T(2) * val.imag())));
	}
	if (isnan(val.real()) && val.imag() == 0)
		return val;

	T c2r(T(2) * val.real());
	T c2i(T(2) * val.imag());
	T dc(cosh(c2r) + cos(c2i));
	T c2rsh(sinh(c2r));

	if (isinf(c2rsh) && isinf(dc))
		return complex<T>(c2rsh > T(0) ? T(1) : T(-1),
			c2i > T(0) ? T(0) : T(-0.));
	return  complex<T>(c2rsh / dc, sin(c2i) / dc);
}

template<class T>
complex<T> asin(const complex<T>& val)
{
	complex<T> z = asinh(complex<T>(-val.imag(), val.real()));
	return complex<T>(z.imag(), -z.real());
}

template<class T>
complex<T> acos(const complex<T>& val)
{
	if (isinf(val.real()))
	{
		if (isnan(val.imag()))
			return complex<T>(val.imag(), val.real());
		if (isinf(val.imag()))
		{
			if (val.real() < T(0))
				return complex<T>(T(0.75) * __PIT(T), -val.imag());
			return complex<T>(T(0.25) * __PIT(T), -val.imag());
		}
		if (val.real() < T(0))
			return complex<T>(__PIT(T), signbit(val.imag()) ? -val.real() : val.real());
		return complex<T>(T(0), signbit(val.imag()) ? val.real() : -val.real());
	}
	if (isnan(val.real()))
	{
		if (isinf(val.imag()))
			return complex<T>(val.real(), -val.imag());
		return complex<T>(val.real(), val.real());
	}
	if (isinf(val.imag()))
		return complex<T>(__PIT(T) / T(2), -val.imag());
	if (val.real() == 0)
		return complex<T>(__PIT(T) / T(2), -val.imag());
	complex<T> z = log(val + sqrt(pow(val, T(2)) - T(1)));
	if (signbit(val.imag()))
		return complex<T>(abs(z.imag()), abs(z.real()));
	return complex<T>(abs(z.imag()), -abs(z.real()));
}

template<class T>
complex<T> atan(const complex<T>& val)
{
	complex<T> z = atanh(complex<T>(-val.imag(), val.real()));
	return complex<T>(z.imag(), -z.real());
}

template<class T>
complex<T> sin(const complex<T>& val)
{
	complex<T> z = sinh(complex<T>(-val.imag(), val.real()));
	return complex<T>(z.imag(), -z.real());
}

template<class T>
complex<T> cos(const complex<T>& val)
{
	return cosh(complex<T>(-val.imag(), val.real()));
}

template<class T>
complex<T> tan(const complex<T>& val)
{
	complex<T> z = tanh(complex<T>(-val.imag(), val.real()));
	return complex<T>(z.imag(), -z.real());
}

#endif
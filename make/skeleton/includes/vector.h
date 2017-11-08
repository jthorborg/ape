#ifndef CPPAPE_VECTOR_H
#define CPPAPE_VECTOR_H

#include "baselib.h"
#include "algorithm.h"

template<class T>
class vector
{
public:
	vector() : vsize(0), vcapacity(0), ptr(NULL)
	{

	}

	vector(size_t count) : vsize(0), vcapacity(0), ptr(NULL)
	{
		resize(count);
	}

	vector(const T* start, const T* _end);

	void resize(size_t newSize)
	{
		if (vsize == newSize)
			return;

		if (newSize < vsize)
		{
			// shrink, kill els

			for (size_t i = vsize - newSize; i < vsize; ++i)
				ptr[i].~T();
		}
		else if (newSize < vcapacity)
		{
			// default construct new
			for (size_t i = vsize; i < newSize; ++i)
				new (&ptr[i]) T;
		}
		else
		{
			// relocate
			vcapacity = nextpow2(newSize);
			T* temp_ptr = (T*) operator new(sizeof(T) * vcapacity);
			size_t i;
			// construct new from old
			for (i = 0; i < vsize; ++i)
				new (&temp_ptr[i]) T(ptr[i]);

			// delete old
			for (i = 0; i < vsize; ++i)
				ptr[i].~T();

			// default construct new
			for (i = vsize; i < newSize; ++i)
				new (&temp_ptr[i]) T;

			operator delete(ptr);

			ptr = temp_ptr;

		}

		vsize = newSize;

	}

	void reserve(size_t newSize)
	{
		if (newSize <= vcapacity)
			return;

		// relocate
		vcapacity = nextpow2(newSize);
		T* temp_ptr = (T*) operator new(sizeof(T) * vcapacity);
		size_t i;
		// construct new from old
		for (i = 0; i < vsize; ++i)
			new (&temp_ptr[i]) T(ptr[i]);

		// delete old
		for (i = 0; i < vsize; ++i)
			ptr[i].~T();

		operator delete(ptr);

		ptr = temp_ptr;
	}

	void push_back(const T& el)
	{
		if (vsize + 1 >= vcapacity)
			reserve(vsize + 1);

		new (&ptr[vsize++]) T(el);
	}

	void pop_back()
	{
		if (vsize != 0)
		{
			ptr[vsize - 1].~T();
			vsize--;
		}
	}

	T* data()
	{
		return begin();
	}

	const T* begin() const
	{
		return ptr;
	}

	const T* end() const
	{
		return ptr + vsize;
	}

	T* begin()
	{
		return ptr;
	}

	T* end()
	{
		return ptr + vsize;
	}

	T& at(size_t idx)
	{
		return ptr[idx];
	}

	const T& at(size_t idx) const
	{
		return ptr[idx];
	}

	T& operator[] (size_t idx)
	{
		return ptr[idx];
	}

	const T& operator[] (size_t idx) const
	{
		return ptr[idx];
	}

	size_t size() const
	{
		return vsize;
	}

	size_t capacity() const
	{
		return vcapacity;
	}

	bool empty() const
	{
		return vsize != 0;
	}

	void clear()
	{
		for (size_t i = 0; i < vsize; ++i)
			ptr[i].~T();

		vsize = 0;
	}

	void swap(vector<T>& other)
	{
		::swap(ptr, other.ptr);
		::swap(vsize, other.vsize);
		::swap(vcapacity, other.vcapacity);
	}

	~vector()
	{
		clear();
		operator delete(ptr);
	}

private:
	T * ptr;
	size_t vsize, vcapacity;
};

template<class T>
vector<T>::vector(const T* start, const T* _end)
{
	vsize = vcapacity = 0;
	ptr = NULL;
	size_t size = distance(start, _end);
	reserve(size);
	for (size_t i = 0; i < size; ++i)
	{
		new (&ptr[i]) T(start[i]);
	}

	vsize = size;
}

#endif
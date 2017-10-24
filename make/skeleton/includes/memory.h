#ifndef CPPAPE_MEMORY_H
#define CPPAPE_MEMORY_H

template<class T>
class auto_ptr
{
public:
	auto_ptr() { ptr = 0; }
	auto_ptr(T * instance) { ptr = instance; }
	auto_ptr(auto_ptr<T>& other)
	{
		ptr = other.ptr;
		other.ptr = NULL;
	}

	void reset(T* newPtr)
	{
		if (ptr)
			delete ptr;

		ptr = newPtr;
	}

	T* operator -> () { return ptr; }

	~auto_ptr()
	{
		if (ptr)
			delete ptr;
	}

private:
	T * ptr;
};

template<class T>
class auto_array
{
public:
	auto_array() { ptr = 0; }
	auto_array(T * instance) { ptr = instance; }
	auto_array(auto_array<T>& other)
	{
		ptr = other.ptr;
		other.ptr = NULL;
	}

	void reset(T* newPtr)
	{
		if (ptr)
			delete ptr;

		ptr = newPtr;
	}

	T& operator [] (size_t index) { return ptr[index]; }
	~auto_array()
	{
		if (ptr)
			delete[] ptr;
	}

private:
	T * ptr;
};

#endif
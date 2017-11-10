#ifndef CPPAPE_ITERATOR_H
#define CPPAPE_ITERATOR_H

#include "tcc/stddef.h"

template<class T>
ssize_t distance(const T * begin, const T * end)
{
	return end - begin;
}

template <class InputIterator, class Distance>
void advance(InputIterator& it, Distance n)
{
	if (n > 0)
		while (n--)
			it++;
	else if (n < 0)
		while (n++)
			it--;
}

#endif
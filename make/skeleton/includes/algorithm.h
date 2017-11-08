#ifndef CPPAPE_ALGORITHM_H
#define CPPAPE_ALGORITHM_H

#include "tcc/memory.h"

template <class ForwardIterator>
ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last)
{
	if (first != last)
	{
		ForwardIterator next = first; ++next;
		while (next != last) {
			if (*first == *next)     // or: if (pred(*first,*next)), for version (2)
				return first;
			++first; ++next;
		}
	}
	return last;
}

template <class T> 
void swap(T& a, T& b)
{
	T c(a); a = b; b = c;
}

template<class InputIterator, class OutputIterator>
OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result)
{
	while (first != last) {
		*result = *first;
		++result; ++first;
	}
	return result;
}



#endif
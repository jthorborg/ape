#ifndef CPPAPE_ALGORITHM_H
#define CPPAPE_ALGORITHM_H

#include "utility.h"
#include "iterator.h"

template<class InputIterator, class UnaryPredicate>
bool all_of(InputIterator first, InputIterator last, UnaryPredicate pred)
{
	while (first != last) 
	{
		if (!pred(*first)) return false;
		++first;
	}
	return true;
}

template<class InputIterator, class UnaryPredicate>
bool any_of(InputIterator first, InputIterator last, UnaryPredicate pred)
{
	while (first != last) 
	{
		if (pred(*first)) return true;
		++first;
	}
	return false;
}

template<class InputIterator, class UnaryPredicate>
bool none_of(InputIterator first, InputIterator last, UnaryPredicate pred)
{
	while (first != last) 
	{
		if (pred(*first)) return false;
		++first;
	}
	return true;
}

template<class InputIterator, class Function>
Function for_each(InputIterator first, InputIterator last, Function fn)
{
	while (first != last) 
	{
		fn(*first);
		++first;
	}
	return fn; 
}

template<class InputIterator, class T>
InputIterator find(InputIterator first, InputIterator last, const T& val)
{
	while (first != last) 
	{
		if (*first == val) 
			return first;
		++first;
	}
	return last;
}

template<class InputIterator, class UnaryPredicate>
InputIterator find_if(InputIterator first, InputIterator last, UnaryPredicate pred)
{
	while (first != last) 
	{
		if (pred(*first)) 
			return first;
		++first;
	}
	return last;
}

template<class InputIterator, class UnaryPredicate>
InputIterator find_if_not(InputIterator first, InputIterator last, UnaryPredicate pred)
{
	while (first != last) 
	{
		if (!pred(*first)) 
			return first;
		++first;
	}
	return last;
}

template<class ForwardIterator1, class ForwardIterator2>
ForwardIterator1 find_end(ForwardIterator1 first1, ForwardIterator1 last1,
	ForwardIterator2 first2, ForwardIterator2 last2)
{
	if (first2 == last2) return last1;  // specified in C++11

	ForwardIterator1 ret = last1;

	while (first1 != last1)
	{
		ForwardIterator1 it1 = first1;
		ForwardIterator2 it2 = first2;
		while (*it1 == *it2) {    // or: while (pred(*it1,*it2)) for version (2)
			++it1; ++it2;
			if (it2 == last2) { ret = first1; break; }
			if (it1 == last1) return ret;
		}
		++first1;
	}
	return ret;
}

template<class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
ForwardIterator1 find_end(ForwardIterator1 first1, ForwardIterator1 last1,
	ForwardIterator2 first2, ForwardIterator2 last2, BinaryPredicate pred)
{
	if (first2 == last2) return last1;  // specified in C++11

	ForwardIterator1 ret = last1;

	while (first1 != last1)
	{
		ForwardIterator1 it1 = first1;
		ForwardIterator2 it2 = first2;
		while (pred(*it1, *it2))
		{ 
			++it1; ++it2;
			if (it2 == last2) 
			{
				ret = first1; 
				break; 
			}
			if (it1 == last1)
				return ret;
		}
		++first1;
	}
	return ret;
}

template<class InputIterator, class ForwardIterator>
InputIterator find_first_of(InputIterator first1, InputIterator last1,
	ForwardIterator first2, ForwardIterator last2)
{
	while (first1 != last1) {
		for (ForwardIterator it = first2; it != last2; ++it) 
		{
			if (*it == *first1)          // or: if (pred(*it,*first)) for version (2)
				return first1;
		}
		++first1;
	}
	return last1;
}

template<class InputIterator, class ForwardIterator, class BinaryPredicate>
InputIterator find_first_of(InputIterator first1, InputIterator last1,
	ForwardIterator first2, ForwardIterator last2, BinaryPredicate pred)
{
	while (first1 != last1) {
		for (ForwardIterator it = first2; it != last2; ++it)
		{
			if (pred(*it, *first1)) 
				return first1;
		}
		++first1;
	}
	return last1;
}

template <class ForwardIterator>
ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last)
{
	if (first != last)
	{
		ForwardIterator next = first; ++next;
		while (next != last)
		{
			if (*first == *next)
				return first;
			++first; ++next;
		}
	}
	return last;
}

template<class ForwardIt, class BinaryPredicate>
ForwardIt adjacent_find(ForwardIt first, ForwardIt last,
	BinaryPredicate p)
{
	if (first == last)
	{
		return last;
	}
	ForwardIt next = first;
	++next;
	for (; next != last; ++next, ++first)
	{
		if (p(*first, *next))
		{
			return first;
		}
	}
	return last;
}

template <class InputIterator, class T>
int count(InputIterator first, InputIterator last, const T& val)
{
	int ret = 0;
	while (first != last) 
	{
		if (*first == val) 
			++ret;
		++first;
	}
	return ret;
}

template <class InputIterator, class UnaryPredicate>
int count_if(InputIterator first, InputIterator last, UnaryPredicate pred)
{
	int ret = 0;
	while (first != last) {
		if (pred(*first)) ++ret;
		++first;
	}
	return ret;
}

template <class InputIterator1, class InputIterator2>
pair<InputIterator1, InputIterator2>
mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2)
{
	while ((first1 != last1) && (*first1 == *first2)) 
	{
		++first1; ++first2;
	}
	return make_pair(first1, first2);
}

template <class InputIterator1, class InputIterator2, class BinaryPredicate>
pair<InputIterator1, InputIterator2>
mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, BinaryPredicate pred)
{
	while ((first1 != last1) && pred(*first1, *first2))
	{
		++first1; ++first2;
	}
	return make_pair(first1, first2);
}

template <class InputIterator1, class InputIterator2, class BinaryPredicate>
bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, BinaryPredicate pred)
{
	while (first1 != last1) {
		if (!pred(*first1, *first2))
			return false;
		++first1; ++first2;
	}
	return true;
}

template<class ForwardIterator1, class ForwardIterator2>
ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1,
	ForwardIterator2 first2, ForwardIterator2 last2)
{
	if (first2 == last2) return first1;

	while (first1 != last1)
	{
		ForwardIterator1 it1 = first1;
		ForwardIterator2 it2 = first2;
		while (*it1 == *it2) {
			if (it2 == last2) return first1;
			if (it1 == last1) return last1;
			++it1; ++it2;
		}
		++first1;
	}
	return last1;
}

template<class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1,
	ForwardIterator2 first2, ForwardIterator2 last2, BinaryPredicate pred)
{
	if (first2 == last2) return first1;

	while (first1 != last1)
	{
		ForwardIterator1 it1 = first1;
		ForwardIterator2 it2 = first2;
		while (pred(*it1, *it2)) {
			if (it2 == last2) return first1;
			if (it1 == last1) return last1;
			++it1; ++it2;
		}
		++first1;
	}
	return last1;
}

template<class ForwardIterator, class Size, class T>
ForwardIterator search_n(ForwardIterator first, ForwardIterator last,
	Size count, const T& val)
{
	ForwardIterator it, limit;
	Size i;

	limit = first; advance(limit, distance(first, last) - count);

	while (first != limit)
	{
		it = first; i = 0;
		while (*it == val)
		{
			++it; if (++i == count) 
				return first;
		}
		++first;
	}
	return last;
}

template<class ForwardIterator, class Size, class T, class BinaryPredicate>
ForwardIterator search_n(ForwardIterator first, ForwardIterator last,
	Size count, const T& val, BinaryPredicate pred)
{
	ForwardIterator it, limit;
	Size i;

	limit = first; advance(limit, distance(first, last) - count);

	while (first != limit)
	{
		it = first; i = 0;
		while (pred(*it, val))
		{
			++it; if (++i == count)
				return first;
		}
		++first;
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
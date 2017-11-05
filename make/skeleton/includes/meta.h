#ifndef CPPAPE_META_H
#define CPPAPE_META_H

#include "baselib.h"


typedef char meta_yes[1];
typedef char meta_no [2];

template<class C> static meta_yes& meta_is_test(C * x, C * y); // selected if C is a class type
static meta_no&  meta_is_test(...);      // selected otherwise

template<class X, class Y>
class is_same
{
public:
static size_t const value;
};

template<class X, class Y>
size_t const is_same<X, Y>::value = sizeof(meta_is_test((X*)0, (Y*)0)) == sizeof(meta_yes);




#endif
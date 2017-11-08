#ifndef CPPAPE_ASSERT_H
#define CPPAPE_ASSERT_H

void abort(const char * reason);

#define assert(expr) if(!expr) abort("Assertion \"" #expr "\" failed.")


#endif
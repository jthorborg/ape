#ifndef CPPAPE_STRING_VIEW_H
#define CPPAPE_STRING_VIEW_H

#include "assert.h"
#include "tcc/string.h"

class string_view
{
public:
	string_view(const char * str)
	{
		assert(str != NULL && "nullptr in string view");

		pbegin = str;
		pend = str + strlen(str);
	}

	string_view(const char * begin, const char * end)
	{
		assert(begin != NULL && end != NULL && "nullptr in string view");

		pbegin = begin;
		pend = end;
	}

	string_view() : pbegin(""), pend(pbegin) {}

	string_view(const string_view& other) : pbegin(other.pbegin), pend(other.pend) {}

	static string_view empty() { return ""; }

	size_t size() const { return pend - pbegin; }
	const char * c_str() const { return pbegin; }
	char operator[] (size_t idx) const { return pbegin[idx]; }
	
	const char * begin() const { return pbegin; }
	const char * end() const { return pend; }
	const char * data() const { return begin(); }

private:
	const char * pbegin, * pend;
};

inline void abort(const string_view& reason)
{
	abort(reason.c_str());
}

#endif
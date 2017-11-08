#ifndef CPPAPE_STRING_H
#define CPPAPE_STRING_H

#include "baselib.h"
#include "vector.h"
#include "assert.h"
#include "string_view.h"

class string
{
public:

	string() { copy_assign(); }
	string(const char * other) { *this = string_view(other); }
	string(const string& other) { copy_assign(other.begin(), other.end()); }
	string(const string_view& view) { copy_assign(view.begin(), view.end()); }

	string& operator = (const string_view& view) { copy_assign(view.begin(), view.end()); return *this; }

	// invariant preserving methods
	char& back () { return contents[contents.size() - 1]; }
	char back () const { return contents[contents.size() - 1]; }

	char& front() { return contents[0]; }
	char front() const { return contents[0]; }

	char& at(size_t index) { assert(index < contents.size() - 1 && "access out of bounds"); return contents[index]; }
	char at(size_t index) const { assert(index < contents.size() - 1 && "access out of bounds"); return contents[index]; }

	char& operator[] (size_t index) { return contents[index]; }
	char operator[] (size_t index) const { return contents[index]; }
	
	char * begin() { return contents.begin(); }
	const char * begin() const { return contents.begin(); }
	char * end() { return contents.end() - 1; }
	const char * end() const { return contents.end() - 1; }

	const char * c_str() const { return begin(); }
	const char * data() const { return begin(); }
	char * data() { return begin(); }

	size_t size() const { return contents.size() - 1; }
	size_t length() const { return contents.size() - 1; }
	size_t capacity() const { return contents.capacity() -1; }

	void reserve(size_t size) { contents.reserve(size + 1); }
	void clear() { copy_assign(); }
	bool empty() const { return contents.size() < 2; }

	// mutators / modifiers

	string& operator += (const string_view& other)
	{
		append(other);
		return *this;
	}

	string& append(const string_view& other, size_t subpos = 0, size_t len = -1)
	{
		if (len == -1)
			len = other.size();

		copy_append(other.begin() + subpos, other.begin() + len);

		return *this;
	}

	string& append(char c, size_t n = 0)
	{
		for (size_t i = 0; i < n; ++i)
			copy_append(&c, &c + 1);

		return *this;
	}

	string& assign(const string_view& other)
	{
		return *this = other;
	}

	void push_back(char c) { append(c);	}
	void pop_back() { contents.pop_back(); invariants(); }
	void swap(string& other) { contents.swap(other.contents);}

	operator const string_view() const
	{
		return string_view(contents.begin(), contents.end() - 1);
	}

private:

	void copy_assign(const char * begin = NULL, const char * end = NULL)
	{
		size_t size = distance(begin, end);
		contents.resize(size + 1);
		memmove(contents.begin(), begin, size);
		invariants();
	}

	void copy_append(const char * begin, const char * end)
	{
		size_t old_size = contents.size() - 1;
		size_t size = distance(begin, end);
		contents.resize(old_size + size + 1);
		memmove(contents.begin() + old_size, begin, size);
		invariants();
	}

	void invariants()
	{
		contents[contents.size() - 1] = '\0';
	}

	vector<char> contents;
};


#endif
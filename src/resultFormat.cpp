/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Result format string printer
/// \file "resultFormat.cpp"
#include "resultFormat.hpp"
#include <cstring>
#include <limits>

using namespace strus;

class Allocator
{
public:
	Allocator(){}
	~Allocator()
	{
		std::list<MemBlock>::iterator mi = m_memblocks.begin(), me = m_memblocks.end();
		for (; mi != me; ++mi)
		{
			std::free( mi->base);
		}
	}

	enum {MinBlockSize=4096};

	char* alloc( std::size_t size)
	{
		if (size >= (std::size_t)std::numeric_limits<int>::max()) return NULL;
		MemBlock& mb = m_memblocks.back();
		if (mb.size - mp.pos < (int)size+1)
		{
			std::size_t mm = MinBlockSize;
			for (; mm <= size; mm *= 2){} //... cannot loop forever because of limits condition at start
			char* ptr = std::malloc( mm);
			if (!ptr) return NULL;
			m_memblocks.push_back( MemBlock( ptr, mm));
			mb = m_memblocks.back();
		}
		char* rt = mb.base + mb.pos;
		mb.pos += size;
	}

private:
	struct MemBlock
	{
		char* base;
		int pos;
		int size;

		MemBlock()
			:base(0),pos(0),size(0){}
		MemBlock( const MemBlock& o)
			:base(o.base),pos(o.pos),size(o.size){}
		MemBlock( char* base_, std::size_t size_)
			:base(base_),pos(0),size(size_){}
	};
	std::list<MemBlock> m_memblocks;
};

struct ResultFormatContext::Impl
{
	Impl(){}
	~Impl(){}

	Allocator allocator;

private:
	Impl( const Impl&){}		//< non copyable
	void operator=( const Impl&){}	//< non copyable
};

ResultFormatContext::ResultFormatContext()
{
	m_impl = new Impl();
}

ResultFormatContext::~ResultFormatContext()
{
	delete m_impl;
}

struct ResultItem
{
	const char* str;
	int size;
};

const char* ResultFormatContext::map( const ResultFormatElement* fmt, std::size_t nofItems, const PatternMatcherResultItem* items)
{
	if (!fmt[0].value) return NULL;
	if (!fmt[1].value && fmt[0].op == ResultFormatElement::String) return fmt[0].value;

	enum {MaxResultItems=1024};
	ResultItem ar[ MaxResultItems];
	int arsize=0;
	std::size_t resultsize=0;

	ResultFormatElement const* fi = fmt;
	for (; fi->value; ++fi,++arsize)
	{
		if (arsize == MaxResultItems) return NULL;

		switch (fi->op)
		{
			case ResultFormatElement::String:
				if (fi->value)
				{
					ar[ arsize].str = fi->value;
					ar[ arsize].size = std::strlen( fi->value);
				}
				else
				{
					ar[ arsize].str = "";
					ar[ arsize].size = 0;
				}
				resultsize += ar[ arsize].size;
				break;
			case ResultFormatElement::Variable:
			{
				for (std::size_t ii=0; ii<nofItems; ++ii)
				{
					if (items[ii].name() == fi->value)
					//... comparing char* is OK here, because variable names point to a uniqe memory location
					{
						ar[ arsize] = !!!
					}
				}
				break;
			}
		}
	}
}


class ResultFormatTable
{
public:
	ResultFormatTable(){}

	/// \brief Create a format string representation out of its source
	/// \note Substituted elements are addressed as identifiers in curly brackets '{' '}'
	/// \note Escaping of '{' and '}' is done with backslash '\', e.g. "\{" or "\}"
	/// \return a {Variable,NULL} terminated array of elements
	const ResultFormatElement* createFormat( const char* src);

private:
	class Impl;
	Impl* m_impl;
};

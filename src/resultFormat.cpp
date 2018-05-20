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
#include "strus/errorBufferInterface.hpp"
#include "strus/debugTraceInterface.hpp"
#include "strus/base/utf8.hpp"
#include "strus/errorCodes.hpp"
#include "errorUtils.hpp"
#include "internationalization.hpp"
#include <cstring>
#include <limits>
#include <list>
#include <map>

using namespace strus;

struct ResultFormatElement
{
	enum Op {Variable,String};
	Op op;
	const char* value;
	const char* separator;
};

namespace strus
{
struct ResultFormat
{
	const ResultFormatElement* ar;		///< array of elements
	std::size_t arsize;			///< number of elements in array 'ar'
	std::size_t estimated_allocsize;	///< estimated maximum length of the result printed in bytes including 0-byte terminator
};
}//namespace

class Allocator
{
public:
	enum {VariableResultElementSize = 32};

	Allocator()
		:m_last_alloc_pos(0),m_last_alloc_align(0){}

	~Allocator()
	{
		std::list<MemBlock>::iterator mi = m_memblocks.begin(), me = m_memblocks.end();
		for (; mi != me; ++mi)
		{
			std::free( mi->base);
		}
	}

	enum {MinBlockSize=32*1024/*32K*/};

	void* alloc( std::size_t size, int align=1)
	{
		if (size >= (std::size_t)std::numeric_limits<int>::max()) return NULL;
		if (m_memblocks.empty() || m_memblocks.back().size - m_memblocks.back().pos < (int)size + align)
		{
			std::size_t mm = MinBlockSize;
			for (; mm <= size; mm *= 2){} //... cannot loop forever because of limits condition at start
			char* ptr = (char*)std::malloc( mm);
			if (!ptr) return NULL;
			m_memblocks.push_back( MemBlock( ptr, mm));
		}
		MemBlock& mb = m_memblocks.back();
		while ((mb.pos & (align -1)) != 0) ++mb.pos;
		char* rt = mb.base + mb.pos;
		m_last_alloc_pos = mb.pos;
		m_last_alloc_align = align;
		mb.pos += size;
		return rt;
	}

	void* realloc_last_alloc( std::size_t size)
	{
		MemBlock& mb = m_memblocks.back();
		if (mb.pos - m_last_alloc_pos > (int)size)
		{
			mb.pos = m_last_alloc_pos + size;
			return mb.base + m_last_alloc_pos;
		}
		else if (mb.pos - m_last_alloc_pos < (int)size)
		{
			if (mb.size - m_last_alloc_pos > (int)size)
			{
				mb.pos = m_last_alloc_pos + size;
				return mb.base + m_last_alloc_pos;
			}
			else
			{
				char* rt = (char*)alloc( size, m_last_alloc_align);
				if (!rt) return NULL;
				std::memcpy( rt, mb.base + m_last_alloc_pos, mb.pos - m_last_alloc_pos);
				return rt;
			}
		}
		else
		{
			return mb.base + m_last_alloc_pos;
		}
	}

	void resize_last_alloc( std::size_t size)
	{
		MemBlock& mb = m_memblocks.back();
		if (mb.pos - m_last_alloc_pos > (int)size)
		{
			mb.pos = m_last_alloc_pos + size;
		}
	}

private:
	Allocator( const Allocator&){}		//< non copyable
	void operator=( const Allocator&){}	//< non copyable

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
	int m_last_alloc_pos;
	int m_last_alloc_align;
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

ResultFormatContext::ResultFormatContext( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_),m_debugtrace(0)
{
	m_impl = new Impl();
	DebugTraceInterface* dbgi = m_errorhnd->debugTrace();
	if (dbgi) m_debugtrace = dbgi->createTraceContext( "pattern");
}

ResultFormatContext::~ResultFormatContext()
{
	delete m_impl;
	if (m_debugtrace) delete m_debugtrace;
}

struct ResultFormatTable::Impl
{
	Impl(){}
	~Impl(){}

	Allocator allocator;
	std::map<std::string,const char*> separatormap;

private:
	Impl( const Impl&){}		//< non copyable
	void operator=( const Impl&){}	//< non copyable
};

ResultFormatTable::ResultFormatTable( ErrorBufferInterface* errorhnd_, ResultFormatVariableMap* variableMap_)
	:m_errorhnd(errorhnd_),m_variableMap(variableMap_)
{
	m_impl = new Impl();
}

ResultFormatTable::~ResultFormatTable()
{
	delete m_impl;
}


static inline char* printNumber( char* ri, const char* re, int num)
{
	std::size_t len;
	if (re - ri < 8) return 0;
	len = strus::utf8encode( ri, num);
	if (!len) return 0;
	return ri + len;
}

static char* printResultItemReference( char* ri, const char* re, const analyzer::PatternMatcherResultItem& item)
{
	if (ri == re) return NULL;
	if (item.start_origseg() == item.end_origseg() && item.end_origpos() > item.start_origpos())
	{
		*ri++ = '\1';
		if (0==(ri = printNumber( ri, re, item.start_origseg()))) return NULL;
		if (0==(ri = printNumber( ri, re, item.start_origpos()))) return NULL;
		if (0==(ri = printNumber( ri, re, item.end_origpos() - item.start_origpos()))) return NULL;
	}
	else
	{
		*ri++ = '\2';
		if (0==(ri = printNumber( ri, re, item.start_origseg()))) return NULL;
		if (0==(ri = printNumber( ri, re, item.start_origpos()))) return NULL;
		if (0==(ri = printNumber( ri, re, item.end_origseg()))) return NULL;
		if (0==(ri = printNumber( ri, re, item.end_origpos()))) return NULL;
	}
	return ri;
}

static inline char* printValue( char* ri, const char* re, const char* value)
{
	char const* vi = value;
	for (; ri < re && *vi; ++ri,++vi) *ri = *vi;
	return ri < re ? ri : NULL;
}

const char* ResultFormatContext::map( const ResultFormat* fmt, std::size_t nofItems, const analyzer::PatternMatcherResultItem* items)
{
	if (!fmt->ar[0].value) return NULL;
	if (!fmt->ar[1].value && fmt->ar[0].op == ResultFormatElement::String) return fmt->ar[0].value;

	int mm = fmt->estimated_allocsize + (nofItems * Allocator::VariableResultElementSize) + 128/*thumb estimate*/;
	char* result = (char*)m_impl->allocator.alloc( mm);
	if (!result)
	{
		m_errorhnd->report( ErrorCodeOutOfMem, _TXT("out of memory"));
		return NULL;
	}
	char* ri = result;
	const char* re = result + mm - 1;

	ResultFormatElement const* fi = fmt->ar;
	const ResultFormatElement* fe = fmt->ar + fmt->arsize;
	for (; fi!=fe; ++fi)
	{
		char* next_ri = ri;
		switch (fi->op)
		{
			case ResultFormatElement::String:
			{
				next_ri = printValue( ri, re, fi->value);
			}
			break;
			case ResultFormatElement::Variable:
			{
				int nn = 0;
				char* loop_ri = ri;
				for (std::size_t ii=0; ii<nofItems; ++ii)
				{
					const analyzer::PatternMatcherResultItem& item = items[ii];
					if (item.name() == fi->value)
					//... comparing char* is OK here, because variable names point to a uniqe memory location
					{
						if (nn++ > 0)
						{
							next_ri = printValue( loop_ri, re, fi->separator);
							if (!next_ri) break;
						}
						if (item.value())
						{
							next_ri = printValue( loop_ri, re, item.value());
						}
						else
						{
							next_ri = printResultItemReference( loop_ri, re, items[ii]);
						}
						if (!next_ri) break;
						loop_ri = next_ri;
					}
				}
				break;
			}
		}
		if (!next_ri)
		{
			std::size_t rsize = ri - result;
			mm = rsize * (rsize/4) + 1024;
			result = (char*)m_impl->allocator.realloc_last_alloc( mm);
			if (!result)
			{
				m_errorhnd->report( ErrorCodeOutOfMem, _TXT("out of memory"));
				return NULL;
			}
			ri = result + rsize;
			re = result + mm - 1;
			--fi; //... compensate increment of fi in for loop
			if (m_debugtrace) m_debugtrace->event( "realloc result format", "size=%d newsize=%d", (int)rsize, mm);
			continue;
		}
		ri = next_ri;
	}
	*ri++ = '\0';
	m_impl->allocator.resize_last_alloc( ri-result);
	return result;
}

struct _STDALIGN
{
	int val;
};

static int countNofElements( const char* src)
{
	int rt = 1;
	char const* si = src;
	while (*si)
	{
		const char* last_si = si;
		for (; *si && *si != '{'; ++si)
		{
			if (*si == '\\' && si[1] == '{')
			{
				si += 1;
			}
		}
		if (!*si) return rt;
		if (si > last_si)
		{
			rt += 1; //... const string
		}
		rt += 1; //... variable
		for (; *si && *si != '}'; ++si)
		{
			if (*si == '\\' || *si == '{') return -1;
		}
		if (!*si) return -1;
		++si;
	}
	return rt;
}

static void copyStringValue( char* dest, char const* si, const char* se)
{
	char* vi = dest;
	for (; si != se; ++si)
	{
		if (*si == '\\' && si[1] == '{')
		{
			*vi++ = '{';
			si++;
		}
		else
		{
			*vi++ = *si;
		}
	}
	*vi = '\0';
}

const ResultFormat* ResultFormatTable::createResultFormat( const char* src)
{
	int nofElements = countNofElements( src);
	if (nofElements <= 0)
	{
		m_errorhnd->report( ErrorCodeSyntax, _TXT("failed to parse pattern result format string '%s'"), src);
		return NULL;
	}
	ResultFormat* rt = (ResultFormat*)m_impl->allocator.alloc( sizeof( ResultFormat) + (nofElements * sizeof(ResultFormatElement)), sizeof(_STDALIGN));
	if (!rt)
	{
		m_errorhnd->report( ErrorCodeOutOfMem, _TXT("out of memory"));
		return NULL;
	}
	ResultFormatElement* ar = (ResultFormatElement*)(void*)(rt+1);
	rt->arsize = nofElements;
	rt->ar = ar;
	rt->estimated_allocsize = 0;
	std::memset( ar, 0, (nofElements * sizeof(ResultFormatElement)));
	int ei = 0, ee = nofElements;

	char const* si = src;
	while (*si)
	{
		const char* last_si = si;
		for (; *si && *si != '{'; ++si)
		{
			if (*si == '\\' && si[1] == '{')
			{
				++si;
			}
		}
		if (*si && si > last_si)
		{
			// Put content element:
			if (ei == ee)
			{
				m_errorhnd->report( ErrorCodeRuntimeError, _TXT("internal: failed to parse format source '%s'"), src);
				return NULL;
			}
			char* value = (char*)m_impl->allocator.alloc( si - last_si + 1);
			ResultFormatElement& elem = ar[ ei];
			elem.op = ResultFormatElement::String;
			elem.value = value;
			if (value == NULL)
			{
				m_errorhnd->report( ErrorCodeOutOfMem, _TXT("out of memory"));
				return NULL;
			}
			copyStringValue( value, last_si, si);
			elem.separator = NULL;
			rt->estimated_allocsize += si - last_si;
		}
		if (*si == '{')
		{
			std::string name;
			std::string separator;

			// Put variable:
			try
			{
				for (++si; *si != '}' && *si != '|'; ++si)
				{
					name.push_back( *si);
				}
				if (*si == '|')
				{
					for (++si; *si != '}'; ++si)
					{
						separator.push_back( *si);
					}
				}
			}
			catch (const std::bad_alloc&)
			{
				m_errorhnd->report( ErrorCodeOutOfMem, _TXT("out of memory"));
				return NULL;
			}
			++si;
			const char* variable = m_variableMap->getVariable( name);
			if (!variable)
			{
				m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown variable '%s'"), name.c_str());
				return NULL;
			}
			if (ei == ee)
			{
				m_errorhnd->report( ErrorCodeRuntimeError, _TXT("internal: failed to parse format source '%s'"), src);
				return NULL;
			}
			ResultFormatElement& elem = ar[ ei];
			elem.op = ResultFormatElement::Variable;
			elem.value = variable;
			if (separator.empty())
			{
				elem.separator = " ";
			}
			else
			{
				std::map<std::string,const char*>::const_iterator xi = m_impl->separatormap.find( separator);
				if (xi != m_impl->separatormap.end())
				{
					elem.separator = xi->second;
				}
				else
				{
					char* separatorptr = (char*)m_impl->allocator.alloc( separator.size()+1);
					if (!separatorptr)
					{
						m_errorhnd->report( ErrorCodeOutOfMem, _TXT("out of memory"));
						return NULL;
					}
					std::memcpy( separatorptr, separator.c_str(), separator.size());
					separatorptr[ separator.size()] = '\0';
					elem.separator = separatorptr;
					try
					{
						m_impl->separatormap[ separator] = elem.separator;
					}
					catch (const std::bad_alloc&)
					{
						m_errorhnd->report( ErrorCodeOutOfMem, _TXT("out of memory"));
						return NULL;
					}
				}
			}
			rt->estimated_allocsize += Allocator::VariableResultElementSize;
		}
	}
	if (ei != ee)
	{
		m_errorhnd->report( ErrorCodeRuntimeError, _TXT("internal: error counting result size for expression '%s' (%d != %d)"), src, ei, ee);
		return NULL;
	}
	return rt;
}


bool ResultChunk::parseNext( ResultChunk& result, char const*& src)
{
	std::memset( &result, 0, sizeof(ResultChunk));
	const char* start = src;
	int chlen;

	while ((unsigned char)*src > 2) ++src;
	if (start == src)
	{
		if (*src == '\0')
		{
			return false;
		}
		if (*src == '\1')
		{
			++src;
			chlen = strus::utf8charlen( *src);
			result.start_seg = strus::utf8decode( src, chlen);
			chlen = strus::utf8charlen( *src);
			result.start_pos = strus::utf8decode( src, chlen);
			chlen = strus::utf8charlen( *src);
			int len = strus::utf8decode( src, chlen);
			result.end_seg = result.start_seg;
			result.end_pos = result.start_pos + len;
		}
		else//if (*src == '\2')
		{
			++src;
			chlen = strus::utf8charlen( *src);
			result.start_seg = strus::utf8decode( src, chlen);
			chlen = strus::utf8charlen( *src);
			result.start_pos = strus::utf8decode( src, chlen);
			chlen = strus::utf8charlen( *src);
			result.end_seg = strus::utf8decode( src, chlen);
			chlen = strus::utf8charlen( *src);
			result.end_pos = strus::utf8decode( src, chlen);
		}
		return true;
	}
	else
	{
		result.value = start;
		result.valuesize = src - start;
		return true;
	}
}


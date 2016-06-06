/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of detecting terms defined as regular expressions
/// \file "streamTermMatch.hpp"
#include "streamTermMatch.hpp"
#include "strus/stream/patternMatchTerm.hpp"
#include "strus/stream/patternMatchToken.hpp"
#include "strus/streamTermMatchInstanceInterface.hpp"
#include "strus/streamTermMatchContextInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/reference.hpp"
#include "compactNodeTrie.hpp"
#include "symbolTable.hpp"
#include "utils.hpp"
#include "errorUtils.hpp"
#include "internationalization.hpp"
#include "hs.h"
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>

using namespace strus;
using namespace strus::stream;

static bool isLiteral( char ch)
{
	static const char* regexchr = ".|*?+(){}[]^$";
	return 0==std::strchr( regexchr, ch);
}

static bool isRegularExpression( const std::string& src)
{
	char const* si = src.c_str();
	for (; *si; ++si)
	{
		if (*si == '\\' && *(si+1)) ++si;
		else if (!isLiteral( *si)) return true;
	}
	return false;
}

enum {PatternTypeFlagBit=28};
static unsigned int getPatternHandle( uint32_t idx)
{
	if (idx >= (1<<PatternTypeFlagBit)) throw strus::runtime_error( _TXT("too many patterns defined"));
	return idx | (1<<PatternTypeFlagBit);
}
static unsigned int getSymbolHandle( uint32_t idx)
{
	if (idx >= (1<<PatternTypeFlagBit)) throw strus::runtime_error( _TXT("too many symbols defined"));
	return idx;
}

struct SymbolTableRef
{
	unsigned int patternid;
	Reference<SymbolTable> symbolTable;

	explicit SymbolTableRef( unsigned int patternid_)
		:patternid(patternid_),symbolTable(new SymbolTable){}
	SymbolTableRef( const SymbolTableRef& o)
		:patternid(o.patternid),symbolTable(o.symbolTable){}
	~SymbolTableRef()
	{}
};

struct TermMatchData
{
	std::vector<SymbolTableRef> symbolTableList;
	SymbolTable patternTable;
	hs_database_t* patterndb;

	TermMatchData()
		:patterndb(0){}
	~TermMatchData()
	{
		if (patterndb) hs_free_database(patterndb);
	}
};

class StreamTermMatchContext
	:public StreamTermMatchContextInterface
{
public:
	StreamTermMatchContext( const TermMatchData* data_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_data(data_),m_hs_scratch(0)
	{
		hs_error_t err = hs_alloc_scratch( m_data->patterndb, &m_hs_scratch);
		if (err != HS_SUCCESS)
		{
			throw std::bad_alloc();
		}
	}

	virtual ~StreamTermMatchContext()
	{
		hs_free_scratch( m_hs_scratch);
	}

	static int match_event_handler( unsigned int id, unsigned long int from, unsigned long int to, unsigned int, void *context)
	{
		StreamTermMatchContext* THIS = (StreamTermMatchContext*)context;
		std::vector<SymbolTableRef>::const_iterator
			si = THIS->symbolTableList.begin(),
			se = THIS->symbolTableList.end();
		for (; si != se; ++si)
		{
			if (id == si->patternid)
			{
				unsigned int symid = si->symbolTable->get( src+from, to-from);
				if (symid)
				{
					id = symid;
				}
				break;
			}
		}
		stream::PatternMatchTerm term( id, 0, from, to-from);
		THIS->m_match_termbuf.push_back( term);
		return 0;
	}

	virtual std::vector<stream::PatternMatchTerm> match( const char* src)
	{
		try
		{
			std::vector<stream::PatternMatchToken>::const_iterator
				ti = tokens.begin(), te = tokens.end();
			for (; ti != te; ++ti)
			{
				
			}
			std::vector<stream::PatternMatchTerm> rt;
			return rt;
		}
		CATCH_ERROR_MAP_RETURN( "failed to create pattern matching terms with regular expressions: %s", *m_errorhnd, std::vector<stream::PatternMatchTerm>());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	const TermMatchData* m_data;
	hs_scratch_t* m_hs_scratch;
	std::vector<stream::PatternMatchTerm> m_match_termbuf;
	std::vector<stream::PatternMatchTerm> m_tokenize_termbuf;
};

class StreamTermMatchInstance
	:public StreamTermMatchInstanceInterface
{
public:
	explicit StreamTermMatchInstance( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~StreamTermMatchInstance(){}

	virtual void definePattern( unsigned int id, const std::string& expression, unsigned int level, PositionBind posbind)
	{
		
		if (isRegularExpression(expression))
		{
			return getPatternHandle( m_data.patternTable.getOrCreate( expression));
		}
		else
		{
			return getSymbolHandle( m_data.symbolTable.getOrCreate( expression));
		}
	}

	virtual void defineSymbol( unsigned int id, unsigned int patternid, const std::string& name)
	{
		
	}

	virtual bool compile()
	{
		try
		{
			if (m_data.patterndb) hs_free_database( m_data.patterndb);
			m_data.patterndb = 0;

			std::size_t arsize = m_data.patternTable.size();
			char** xar = (char**)std::malloc( (arsize+1)*sizeof(*ear));
			unsigned int* iar = (unsigned int*)std::malloc( (arsize+1)*sizeof(*iar));
			unsigned int* far = (unsigned int*)std::malloc( (arsize+1)*sizeof(*iar));
			if (!xar | !iar | !far)
			{
				if (xar) std::free(xar);
				if (iar) std::free(iar);
				if (far) std::free(far);
				throw std::bad_alloc();
			}
			std::size_t aidx = 0;
			SymbolTable::const_iterator
				ai = m_data.patternTable.begin(),
				ae = m_data.patternTable.end();
			for (; aidx < arsize && ai != ae; ++ai,++aidx)
			{
				xar[ aidx] = ai->first;
				iar[ aidx] = ai->second;
				far[ aidx] = HS_FLAG_UTF8
						| HS_FLAG_UCP
						| HS_FLAG_DOTALL
						| HS_FLAG_SOM_LEFTMOST;
			}
			xar[ aidx] = 0;
			iar[ aidx] = 0;
			far[ aidx] = 0;
			hs_platform_info_t platform;
			std::memset( &platform, 0, sizeof(platform));
			platform.cpu_features = HS_TUNE_FAMILY_GENERIC;
			hs_compile_error_t* compile_err = 0;

			hs_error_t err =
				hs_compile_multi(
					xar, flags, iar, arsize, HS_MODE_BLOCK, &platform,
					&m_data.patterndb, &error);
			if (err != HS_SUCCESS)
			{
				if (compile_err)
				{
					const char* error_pattern = error->expression < 0 ?0:xar[error->expression];
					if (error_pattern)
					{
						m_errorhnd->report( _TXT( "failed to compile pattern \"%s\": %s\n"),
									  error_pattern, compile_err->message);
					}
					else
					{
						m_errorhnd->report( _TXT( "failed to build automaton from expressions: %s\n"),
									  compile_err->message);
					}
					hs_free_compile_error( compile_err);
				}
				else
				{
					m_errorhnd->report( _TXT( "unknown errpr building automaton from expressions\n"));
				}
			}
			
			if (xar) std::free(xar);
			if (iar) std::free(iar);
			if (far) std::free(far);
			return true;
		}
		CATCH_ERROR_MAP_RETURN( "failed to compile regular expression patterns: %s", *m_errorhnd, false);
	}

	virtual StreamTermMatchContextInterface* createContext() const
	{
		try
		{
			return new StreamTermMatchContext( &m_data, m_errorhnd);
		}
		CATCH_ERROR_MAP_RETURN( "failed to create term match context: %s", *m_errorhnd, 0);
	}
private:
	ErrorBufferInterface* m_errorhnd;
	TermMatchData m_data;
};

StreamTermMatchInstanceInterface* StreamTermMatch::createInstance() const
{
	try
	{
		return new StreamTermMatchInstance( m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( "failed to create term match instance: %s", *m_errorhnd, 0);
}




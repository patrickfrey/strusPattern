/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of detecting tokens defined as regular expressions on text
/// \file "charRegexMatch.hpp"
#include "charRegexMatch.hpp"
#include "strus/stream/patternMatchToken.hpp"
#include "strus/charRegexMatchInstanceInterface.hpp"
#include "strus/charRegexMatchContextInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/reference.hpp"
#include "strus/base/stdint.h"
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
#include <limits>
#include <boost/regex.hpp>

using namespace strus;
using namespace strus::stream;

typedef unsigned long long unsigned_long_long;

enum {MaxPatternId=(1 << 30)-1};

struct PatternDef
{
public:
	PatternDef()
		:m_expression()
		,m_subexpref(0)
		,m_id(0)
		,m_posbind(CharRegexMatchInstanceInterface::BindContent)
		,m_level(0)
		,m_symtabref(0){}
	PatternDef(
			const std::string& expression_,
			unsigned int subexpref_,
			unsigned int id_,
			CharRegexMatchInstanceInterface::PositionBind posbind_,
			unsigned int level_,
			unsigned int symtabref_=0)
		:m_expression(expression_)
		,m_subexpref(subexpref_)
		,m_id(id_)
		,m_posbind(posbind_)
		,m_level(level_)
		,m_symtabref((uint8_t)symtabref_)
	{
		if (subexpref_ >= std::numeric_limits<uint32_t>::max())
		{
			throw strus::runtime_error(_TXT("too many sub expression references defined: %u"), subexpref_);
		}
		if (id_ > MaxPatternId)
		{
			throw strus::runtime_error(_TXT("pattern id out of range, The id must be a positive integer in the range 1..%u"), MaxPatternId);
		}
		if (level_ > std::numeric_limits<uint8_t>::max())
		{
			throw strus::runtime_error(_TXT("level out of range, The level must be a positive integer in the range 1..%u"), (unsigned int)std::numeric_limits<uint8_t>::max());
		}
		if (symtabref_ > std::numeric_limits<uint8_t>::max())
		{
			throw strus::runtime_error(_TXT("symbol table reference out of range, The level must be a positive integer in the range 1..%u"), (unsigned int)std::numeric_limits<uint8_t>::max());
		}
	}
	PatternDef( const PatternDef& o)
		:m_expression(o.m_expression)
		,m_subexpref(o.m_subexpref)
		,m_id(o.m_id)
		,m_posbind(o.m_posbind)
		,m_level(o.m_level)
		,m_symtabref(o.m_symtabref){}

	unsigned int subexpref() const
	{
		return m_subexpref;
	}
	unsigned int id() const
	{
		return m_id;
	}
	CharRegexMatchInstanceInterface::PositionBind posbind() const
	{
		return (CharRegexMatchInstanceInterface::PositionBind)m_posbind;
	}
	unsigned int level() const
	{
		return m_level;
	}
	unsigned int symtabref() const
	{
		return m_symtabref;
	}
	const std::string& expression() const
	{
		return m_expression;
	}
	void setSymtabref( unsigned int symtabref_)
	{
		if (symtabref_ > std::numeric_limits<uint8_t>::max())
		{
			throw strus::runtime_error(_TXT("symbol table reference out of range, The level must be a positive integer in the range 1..%u"), (unsigned int)std::numeric_limits<uint8_t>::max());
		}
		m_symtabref = symtabref_;
	}

private:
	std::string m_expression;
	uint32_t m_subexpref;
	uint32_t m_id;
	uint8_t m_posbind;
	uint8_t m_level;
	uint8_t m_symtabref;
};

class HsPatternTable
{
public:
	std::size_t arsize;
	const char** patternar;
	unsigned int* idar;
	unsigned int* flagar;

	HsPatternTable()
		:arsize(0),patternar(0),idar(0),flagar(0)
	{}

	void init( std::size_t arsize_)
	{
		arsize = arsize_;
		clear();
		patternar = (const char**)std::malloc( (arsize+1)*sizeof(*patternar));
		idar = (unsigned int*)std::malloc( (arsize+1)*sizeof(*idar));
		flagar = (unsigned int*)std::malloc( (arsize+1)*sizeof(*flagar));
		if (!patternar | !idar | !flagar)
		{
			clear();
			throw std::bad_alloc();
		}
	}

	~HsPatternTable()
	{
		clear();
	}

	void clear()
	{
		if (patternar) {std::free(patternar); patternar = 0;}
		if (idar) {std::free(idar); idar = 0;}
		if (flagar) {std::free(flagar); flagar = 0;}
	}
};


class PatternTable
{
public:
	PatternTable(){}

	void definePattern(
			unsigned int id,
			const std::string& expression,
			unsigned int resultIndex,
			unsigned int level,
			CharRegexMatchInstanceInterface::PositionBind posbind)
	{
		unsigned int subexpref = 0;
		if (resultIndex)
		{
			m_subexprmap.push_back( SubExpressionReference( expression, resultIndex));
			subexpref = m_subexprmap.size();
		}
		m_defar.push_back( PatternDef( expression, subexpref, id, posbind, level));
		if (m_defar.size() > std::numeric_limits<uint32_t>::max())
		{
			throw strus::runtime_error(_TXT("too many patterns defined, maximum %u allowed"), (unsigned int)std::numeric_limits<uint32_t>::max());
		}
	}

	void defineSymbol( uint32_t symbolid, unsigned int patternid, const std::string& name)
	{
		if (patternid > MaxPatternId)
		{
			throw strus::runtime_error(_TXT("pattern id out of range, The id must be a positive integer in the range 1..%u"), MaxPatternId);
		}
		if (symbolid > MaxPatternId)
		{
			throw strus::runtime_error(_TXT("symbol id out of range, The id must be a positive integer in the range 1..%u"), MaxPatternId);
		}
		uint8_t symtabref;
		IdSymTabMap::const_iterator yi = m_idsymtabmap.find( patternid);
		if (yi == m_idsymtabmap.end())
		{
			symtabref = m_idsymtabmap[ patternid] = createSymbolTable();
		}
		else
		{
			symtabref = yi->second;
		}
		uint32_t symidx = m_symtabmap[ symtabref-1]->getOrCreate( name);
		if (symidx != m_symidmap.size()+1)
		{
			throw strus::runtime_error(_TXT("symbol defined twice: '%s'"), name.c_str());
		}
		m_symidmap.push_back( symbolid);
	}

	unsigned int symbolId( unsigned int id, const std::string& value) const
	{
		uint8_t symtabref = m_defar[ id].symtabref();
		if (symtabref)
		{
			uint32_t symidx = m_symtabmap[ symtabref-1]->get( value);
			if (symidx)
			{
				return m_symidmap[ symidx-1];
			}
		}
		return id;
	}

	const PatternDef& patternDef( unsigned int id) const
	{
		return m_defar[ id-1];
	}

	///\param[in] options options to stear matching
	void complete( HsPatternTable& hspt, unsigned int options)
	{
		hspt.init( m_defar.size());
		std::vector<PatternDef>::iterator di = m_defar.begin(), de = m_defar.end();
		for (std::size_t didx=0; di != de; ++di,++didx)
		{
			IdSymTabMap::const_iterator ti = m_idsymtabmap.find( di->id());
			if (ti != m_idsymtabmap.end())
			{
				di->setSymtabref( ti->second);
			}
			hspt.patternar[ didx] = di->expression().c_str();
			hspt.idar[ didx] = di->id();
			hspt.flagar[ didx] = options | HS_FLAG_UTF8 | HS_FLAG_SOM_LEFTMOST;
		}
		hspt.patternar[ m_defar.size()] = 0;
		hspt.idar[ m_defar.size()] = 0;
		hspt.flagar[ m_defar.size()] = 0;
	}

	bool matchSubExpression( uint32_t subexpref, const char* src, unsigned_long_long& from, unsigned_long_long& to) const
	{
		const SubExpressionReference& se = m_subexprmap[ subexpref-1];
		const char* start = src + from;
		boost::match_results<const char*> what;
		if (boost::regex_search( start, what, se.regex)) 
		{
			int rel_pos = what.position( se.index);
			int rel_len = what.length( se.index);
			to = from + rel_pos + rel_len;
			from += rel_pos;
			return true;
		}
		return false;
	}

private:
	uint8_t createSymbolTable()
	{
		if (m_symtabmap.size() >= std::numeric_limits<uint8_t>::max())
		{
			throw strus::runtime_error(_TXT("too many patter symbol tables defined, %u allowed"), m_defar.size());
		}
		m_symtabmap.push_back( new SymbolTable());
		return m_symtabmap.size();
	}

	struct SubExpressionReference
	{
		boost::regex regex;
		std::size_t index;

		SubExpressionReference( const std::string& expression, std::size_t index_)
			:regex(expression),index(index_){}
		SubExpressionReference( const SubExpressionReference& o)
			:regex(o.regex),index(o.index){}
	};

private:
	std::vector<PatternDef> m_defar;			///< list of expressions and their attributes defined for the automaton to recognize
	std::vector<Reference<SymbolTable> > m_symtabmap;	///< map PatternDef::symtabref -> symbol table
	std::vector<uint32_t> m_symidmap;			///< map symbol table id -> symbol identifier id given by defineSymbol
	typedef std::map<uint32_t,uint8_t> IdSymTabMap;
	IdSymTabMap m_idsymtabmap;				///< map pattern id -> index in m_symtabmap == PatternDef::symtabref
	std::vector<SubExpressionReference> m_subexprmap;	///< single regular expression patterns for extracting subexpressions if they are referenced.
};


struct TermMatchData
{
	PatternTable patternTable;
	hs_database_t* patterndb;

	TermMatchData()
		:patternTable(),patterndb(0){}
	~TermMatchData()
	{
		if (patterndb) hs_free_database(patterndb);
	}
};

struct MatchEvent
{
	uint32_t id;
	uint8_t level;
	uint8_t posbind;
	uint16_t origsize;
	uint32_t origpos;

	MatchEvent()
		:id(0),level(0),posbind(0),origsize(0),origpos(0){}
	MatchEvent( uint32_t id_, uint8_t level_, uint8_t posbind_, uint32_t origpos_, uint32_t origsize_)
		:id(id_),level(level_),posbind(posbind_),origsize(origsize_),origpos(origpos_){}
	MatchEvent( const MatchEvent& o)
		:id(o.id),level(o.level),posbind(o.posbind),origsize(o.origsize),origpos(o.origpos){}
};

class CharRegexMatchContext
	:public CharRegexMatchContextInterface
{
public:
	CharRegexMatchContext( const TermMatchData* data_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_data(data_),m_hs_scratch(0),m_src(0)
	{
		hs_error_t err = hs_alloc_scratch( m_data->patterndb, &m_hs_scratch);
		if (err != HS_SUCCESS)
		{
			throw std::bad_alloc();
		}
	}

	virtual ~CharRegexMatchContext()
	{
		hs_free_scratch( m_hs_scratch);
	}

	static int match_event_handler( unsigned int id, unsigned_long_long from, unsigned_long_long to, unsigned int, void *context)
	{
		CharRegexMatchContext* THIS = (CharRegexMatchContext*)context;
		try
		{
			if (to - from >= std::numeric_limits<uint16_t>::max())
			{
				throw strus::runtime_error( "size of matched term out of range");
			}
			const PatternDef& patternDef = THIS->m_data->patternTable.patternDef(id);
			if (patternDef.subexpref())
			{
				if (!THIS->m_data->patternTable.matchSubExpression( patternDef.subexpref(), THIS->m_src, from, to))
				{
					return 0;
				}
			}
			MatchEvent matchEvent( (uint32_t)id, patternDef.level(), patternDef.posbind(), (uint32_t)from, (uint32_t)(to-from));
			if (THIS->m_matchEventAr.empty())
			{
				THIS->m_matchEventAr.push_back( matchEvent);
			}
			else
			{
				// Handle superseeding of elements by elements with higher level:
				std::size_t nofDeletes = 0;
				uint32_t matchLastPos = matchEvent.origpos + matchEvent.origsize;
				std::vector<MatchEvent>::reverse_iterator
					mi = THIS->m_matchEventAr.rbegin(),
					me = THIS->m_matchEventAr.rend();
				for (; mi != me && mi->origpos >= matchEvent.origpos; ++mi)
				{
					if (mi->level < matchEvent.level
					&&  mi->origpos + mi->origsize <= matchLastPos)
					{
						// ... an old element is overwritten because it is completely covered by one element with a higher level already in the list
						std::vector<MatchEvent>::iterator oi = mi.base()-1;
						std::vector<MatchEvent>::iterator prev_oi = oi++;
						for (; oi != THIS->m_matchEventAr.end(); prev_oi = oi++)
						{
							*prev_oi = *oi;
						}
						++nofDeletes;
					}
				}
				THIS->m_matchEventAr.resize( THIS->m_matchEventAr.size()-nofDeletes);
				if (!nofDeletes)
				{
					// ... if we had delete then there cannot exist an element covering the
					//     new element. Otherwise deletes would have happened before
					mi = THIS->m_matchEventAr.rbegin();
					me = THIS->m_matchEventAr.rend();
					for (; mi != me && mi->origpos + mi->origsize >= matchLastPos; ++mi)
					{
						if (mi->level > matchEvent.level && mi->origpos <= matchEvent.origpos)
						{
							// ... the new element is ignored because it is completely covered by one element with a higher level already in the list
							return 0;
						}
					}
				}
				// Insert the new element with ascending order of origpos
				THIS->m_matchEventAr.resize( THIS->m_matchEventAr.size()+1);
				mi = THIS->m_matchEventAr.rbegin();
				me = THIS->m_matchEventAr.rend();
				std::vector<MatchEvent>::reverse_iterator prev_mi = mi;
				for (++mi; mi != me && mi->origpos > matchEvent.origpos; prev_mi=mi++)
				{
					*prev_mi = *mi;
				}
				--mi;
				*mi = matchEvent;
			}
			return 0;
		}
		CATCH_ERROR_MAP_RETURN( "error calling hyperscan match event handler: %s", *THIS->m_errorhnd, -1);
	}

	static const char* hsErrorName( int ec)
	{
		switch (ec) {
			case HS_SUCCESS:
				return "HS_SUCCESS";
			case HS_INVALID:
				return "HS_INVALID";
			case HS_NOMEM:
				return "HS_NOMEM";
			case HS_SCAN_TERMINATED:
				return "HS_SCAN_TERMINATED";
			case HS_COMPILER_ERROR:
				return "HS_COMPILER_ERROR";
			case HS_DB_VERSION_ERROR:
				return "HS_DB_VERSION_ERROR";
			case HS_DB_PLATFORM_ERROR:
				return "HS_DB_PLATFORM_ERROR";
			case HS_DB_MODE_ERROR:
				return "HS_DB_MODE_ERROR";
			case HS_BAD_ALIGN:
				return "HS_BAD_ALIGN";
			case HS_BAD_ALLOC:
				return "HS_BAD_ALLOC";
			case HS_SCRATCH_IN_USE:
				return "HS_SCRATCH_IN_USE";
			default:
				return "unknown";
		}
	}

	virtual std::vector<stream::PatternMatchToken> match( const char* src, std::size_t srclen)
	{
		try
		{
			std::vector<stream::PatternMatchToken> rt;
			unsigned int nofExpectedTokens = srclen / 4 + 10;
			m_matchEventAr.reserve( nofExpectedTokens);
			m_src = src;
			if (srclen >= (std::size_t)std::numeric_limits<uint32_t>::max())
			{
				throw strus::runtime_error( "size of string to scan out of range");
			}
			// Collect all matches calling the Hyperscan engine:
			hs_error_t err = hs_scan( m_data->patterndb, src, srclen, 0/*reserved*/, m_hs_scratch, match_event_handler, this);
			m_src = 0;
			if (err != HS_SUCCESS)
			{
				char srcbuf[ 128];
				if (srclen > sizeof(srcbuf)-1) srclen = sizeof(srcbuf)-1;
				std::memcpy( srcbuf, src, srclen);
				srcbuf[ srclen] = 0;
				m_matchEventAr.clear();
				throw strus::runtime_error(_TXT("error matching pattern (hyperscan error %s) on '%s'"), hsErrorName(err), srcbuf);
			}
			rt.reserve( m_matchEventAr.size());

			// Build the result term array, calculate ordinal positions of the result terms:
			std::vector<MatchEvent>::const_iterator
				mi = m_matchEventAr.begin(), me = m_matchEventAr.end();
			uint32_t ordpos = 0;
			uint32_t origpos = 0;
			for (; mi != me; ++mi)
			{
				switch ((CharRegexMatchInstanceInterface::PositionBind)mi->posbind)
				{
					case CharRegexMatchInstanceInterface::BindContent:
						ordpos = 1;
						origpos = mi->origpos;
						rt.push_back( stream::PatternMatchToken( mi->id, 1, mi->origpos, mi->origsize));
						++mi;
						goto EXITLOOP;
					case CharRegexMatchInstanceInterface::BindSuccessor:
						rt.push_back( stream::PatternMatchToken( mi->id, 1, mi->origpos, mi->origsize));
						break;
					case CharRegexMatchInstanceInterface::BindPredecessor:
						break;
				}
			}
			EXITLOOP:
			if (ordpos == 0)
			{
				rt.clear();
			}
			for (; mi != me; ++mi)
			{
				switch ((CharRegexMatchInstanceInterface::PositionBind)mi->posbind)
				{
					case CharRegexMatchInstanceInterface::BindContent:
						if (mi->origpos > origpos)
						{
							origpos = mi->origpos;
							++ordpos;
						}
						rt.push_back( stream::PatternMatchToken( mi->id, ordpos, mi->origpos, mi->origsize));
						break;
					case CharRegexMatchInstanceInterface::BindSuccessor:
						rt.push_back( stream::PatternMatchToken( mi->id, ordpos+1, mi->origpos, mi->origsize));
						break;
					case CharRegexMatchInstanceInterface::BindPredecessor:
						rt.push_back( stream::PatternMatchToken( mi->id, ordpos, mi->origpos, mi->origsize));
						break;
				}
			}
			m_matchEventAr.clear();
			return rt;
		}
		CATCH_ERROR_MAP_RETURN( "failed to run pattern matching terms with regular expressions: %s", *m_errorhnd, std::vector<stream::PatternMatchToken>());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	const TermMatchData* m_data;
	hs_scratch_t* m_hs_scratch;
	const char* m_src;
	std::vector<MatchEvent> m_matchEventAr;
};

class CharRegexMatchInstance
	:public CharRegexMatchInstanceInterface
{
public:
	explicit CharRegexMatchInstance( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_data(),m_state(DefinitionPhase)
	{}

	virtual ~CharRegexMatchInstance(){}

	virtual void definePattern(
			unsigned int id,
			const std::string& expression,
			unsigned int resultIndex,
			unsigned int level,
			PositionBind posbind)
	{
		try
		{
			if (m_state != DefinitionPhase)
			{
				throw strus::runtime_error(_TXT("called define pattern after calling 'compile'"));
			}
			m_data.patternTable.definePattern( id, expression, resultIndex, level, posbind);
		}
		CATCH_ERROR_MAP( "failed to define term match regular expression pattern: %s", *m_errorhnd);
	}

	virtual void defineSymbol( unsigned int symbolid, unsigned int patternid, const std::string& name)
	{
		try
		{
			if (m_state != DefinitionPhase)
			{
				throw strus::runtime_error(_TXT("called define pattern after calling 'compile'"));
			}
			m_data.patternTable.defineSymbol( symbolid, patternid, name);
		}
		CATCH_ERROR_MAP( "failed to define term match regular expression pattern: %s", *m_errorhnd);
	}

	virtual bool compile( const CharRegexMatchOptions& opts)
	{
		try
		{
			if (m_data.patterndb) hs_free_database( m_data.patterndb);
			m_data.patterndb = 0;

			HsPatternTable hspt;
			m_data.patternTable.complete( hspt, getFlags( opts));

			hs_platform_info_t platform;
			std::memset( &platform, 0, sizeof(platform));
			platform.cpu_features = HS_TUNE_FAMILY_GENERIC;
			hs_compile_error_t* compile_err = 0;

			hs_error_t err =
				hs_compile_multi(
					hspt.patternar, hspt.flagar, hspt.idar, hspt.arsize, HS_MODE_BLOCK, &platform,
					&m_data.patterndb, &compile_err);
			if (err != HS_SUCCESS)
			{
				if (compile_err)
				{
					const char* error_pattern = compile_err->expression < 0 ?0:hspt.patternar[ compile_err->expression];
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
				return false;
			}
			m_state = MatchPhase;
			return true;
		}
		CATCH_ERROR_MAP_RETURN( "failed to compile regular expression patterns: %s", *m_errorhnd, false);
	}

	virtual CharRegexMatchContextInterface* createContext() const
	{
		try
		{
			if (m_state != MatchPhase)
			{
				throw strus::runtime_error(_TXT("called create context without calling 'compile'"));
			}
			return new CharRegexMatchContext( &m_data, m_errorhnd);
		}
		CATCH_ERROR_MAP_RETURN( "failed to create term match context: %s", *m_errorhnd, 0);
	}

private:
	static unsigned int getFlags( const CharRegexMatchOptions& opts)
	{
		unsigned int rt = 0;
		CharRegexMatchOptions::const_iterator ai = opts.begin(), ae = opts.end();
		for (; ai != ae; ++ai)
		{
			if (utils::caseInsensitiveEquals( *ai, "CASELESS"))
			{
				rt |= HS_FLAG_CASELESS;
			}
			else if (utils::caseInsensitiveEquals( *ai, "DOTALL"))
			{
				rt |= HS_FLAG_DOTALL;
			}
			else if (utils::caseInsensitiveEquals( *ai, "MULTILINE"))
			{
				rt |= HS_FLAG_MULTILINE;
			}
			else if (utils::caseInsensitiveEquals( *ai, "ALLOWEMPTY"))
			{
				rt |= HS_FLAG_ALLOWEMPTY;
			}
			else if (utils::caseInsensitiveEquals( *ai, "UCP"))
			{
				rt |= HS_FLAG_UCP;
			}
			else
			{
				throw strus::runtime_error(_TXT("unknown option '%s'"), ai->c_str());
			}
		}
		return rt;
	}

private:
	ErrorBufferInterface* m_errorhnd;
	TermMatchData m_data;
	enum State {DefinitionPhase,MatchPhase};
	State m_state;
};

CharRegexMatchInstanceInterface* CharRegexMatch::createInstance() const
{
	try
	{
		return new CharRegexMatchInstance( m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( "failed to create term match instance: %s", *m_errorhnd, 0);
}




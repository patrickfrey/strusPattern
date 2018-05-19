/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of detecting tokens defined as regular expressions on text
/// \file "patternLexer.hpp"
#include "patternLexer.hpp"
#include "unicodeUtils.hpp"
#include "strus/analyzer/patternLexem.hpp"
#include "strus/analyzer/positionBind.hpp"
#include "strus/patternLexerInstanceInterface.hpp"
#include "strus/patternLexerContextInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/reference.hpp"
#include "strus/base/stdint.h"
#include "strus/base/symbolTable.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/debugTraceInterface.hpp"
#include "compactNodeTrie.hpp"
#include "errorUtils.hpp"
#include "internationalization.hpp"
#include "hyperscanErrorCode.hpp"
#include "hs_compile.h"
#include "hs.h"
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <limits>
#include <iostream>
#include <map>
#undef TRE_USE_SYSTEM_REGEX_H
#include <tre/tre.h>

using namespace strus;
using namespace strus::analyzer;

typedef unsigned long long unsigned_long_long;

enum {MaxPatternId=(1 << 30)-1};

struct PatternDef
{
public:
	PatternDef()
		:m_expression()
		,m_expression_onebyte()
		,m_subexpref(0)
		,m_id(0)
		,m_posbind(analyzer::BindContent)
		,m_level(0)
		,m_editdist(0)
		,m_symtabref(0){}
	PatternDef(
			const std::string& expression_,
			unsigned int subexpref_,
			unsigned int id_,
			analyzer::PositionBind posbind_,
			unsigned int level_,
			unsigned int resultidx_,
			unsigned int editdist_,
			unsigned int symtabref_=0)
		:m_expression(expression_)
		,m_expression_onebyte()
		,m_subexpref(subexpref_)
		,m_id(id_)
		,m_posbind(posbind_)
		,m_level(level_)
		,m_resultidx(resultidx_)
		,m_editdist(editdist_)
		,m_symtabref((uint8_t)symtabref_)
	{
		if (subexpref_ >= std::numeric_limits<uint32_t>::max())
		{
			throw strus::runtime_error(_TXT("too many sub expression references defined: %u"), subexpref_);
		}
		if (id_ > MaxPatternId)
		{
			throw strus::runtime_error(_TXT("%s out of range, It must be a positive integer in the range 1..%u"), "pattern id", MaxPatternId);
		}
		if (level_ > std::numeric_limits<uint8_t>::max())
		{
			throw strus::runtime_error(_TXT("%s out of range, It must be a positive integer in the range 1..%u"), "level", (unsigned int)std::numeric_limits<uint8_t>::max());
		}
		if (resultidx_ > std::numeric_limits<uint8_t>::max())
		{
			throw strus::runtime_error(_TXT("%s out of range, It must be a positive integer in the range 1..%u"), "result index", (unsigned int)std::numeric_limits<uint8_t>::max());
		}
		if (editdist_ > std::numeric_limits<uint8_t>::max())
		{
			throw strus::runtime_error(_TXT("%s out of range, It must be a positive integer in the range 1..%u"), "edit distance", (unsigned int)std::numeric_limits<uint8_t>::max());
		}
		if (symtabref_ > std::numeric_limits<uint8_t>::max())
		{
			throw strus::runtime_error(_TXT("%s out of range, It must be a positive integer in the range 1..%u"), "symbol table reference", (unsigned int)std::numeric_limits<uint8_t>::max());
		}
	}
	PatternDef( const PatternDef& o)
		:m_expression(o.m_expression)
		,m_expression_onebyte(o.m_expression_onebyte)
		,m_subexpref(o.m_subexpref)
		,m_id(o.m_id)
		,m_posbind(o.m_posbind)
		,m_level(o.m_level)
		,m_resultidx(o.m_resultidx)
		,m_editdist(o.m_editdist)
		,m_symtabref(o.m_symtabref){}

	unsigned int subexpref() const
	{
		return m_subexpref;
	}
	unsigned int id() const
	{
		return m_id;
	}
	analyzer::PositionBind posbind() const
	{
		return (analyzer::PositionBind)m_posbind;
	}
	unsigned int level() const
	{
		return m_level;
	}
	unsigned int resultidx() const
	{
		return m_resultidx;
	}
	unsigned int editdist() const
	{
		return m_editdist;
	}
	unsigned int symtabref() const
	{
		return m_symtabref;
	}
	const std::string& expression() const
	{
		return m_expression;
	}
	const std::string& expression_onebyte() const
	{
		return m_expression_onebyte;
	}
	void setSymtabref( unsigned int symtabref_)
	{
		if (symtabref_ > std::numeric_limits<uint8_t>::max())
		{
			throw strus::runtime_error(_TXT("symbol table reference out of range, The level must be a positive integer in the range 1..%u"), (unsigned int)std::numeric_limits<uint8_t>::max());
		}
		m_symtabref = symtabref_;
	}
	void setExpressionOneByteCharMap()
	{
		OneByteCharMap obcmap;
		obcmap.init( m_expression.c_str(), m_expression.size());
		m_expression_onebyte = obcmap.value;
	}
	void setSubExpressionRef( unsigned int subexpref_)
	{
		m_subexpref = subexpref_;
	}

private:
	std::string m_expression;		///< regular expression string
	std::string m_expression_onebyte;	///< regular expression string mapped down to one byte character set for prematching
	uint32_t m_subexpref;			///< index of sub expression in sub expression table, for 2nd matching to get the sub expression match
	uint32_t m_id;				///< id of the lexem as defined by definedLexem
	uint8_t m_posbind;			///< analyzer position bind specificaction
	uint8_t m_level;			///< priority level (bigger => higher priority)
	uint8_t m_resultidx;			///< index of subexpression result selected, 0 for the whole match
	uint8_t m_editdist;			///< edit distance (Levenstein) for matching patterns
	uint8_t m_symtabref;			///< symbol table to use
};

class HsPatternTable
{
public:
	std::size_t arsize;
	const char** patternar;
	unsigned int* idar;
	unsigned int* flagar;
	hs_expr_ext_t** extar;

	HsPatternTable()
		:arsize(0),patternar(0),idar(0),flagar(0),extar(0)
	{}

	void init( std::size_t arsize_)
	{
		arsize = arsize_;
		clear();
		patternar = (const char**)std::calloc( (arsize+1),sizeof(*patternar));
		idar = (unsigned int*)std::calloc( (arsize+1),sizeof(*idar));
		flagar = (unsigned int*)std::calloc( (arsize+1),sizeof(*flagar));
		extar = (hs_expr_ext_t**)std::calloc( (arsize+1),sizeof(*extar));
		if (!patternar | !idar | !flagar | !extar)
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
		if (extar)
		{
			std::size_t ai = 0, ae = arsize;
			for (; ai != ae; ++ai)
			{
				if (extar[ai]) std::free( extar[ai]);
			}
			std::free(extar);
			extar = 0;
		}
	}
};


class PatternTable
{
public:
	explicit PatternTable( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_debugtrace(0),m_withOneByteCharMap(false)
	{
		DebugTraceInterface* debugtrace = m_errorhnd->debugTrace();
		if (debugtrace) m_debugtrace = debugtrace->createTraceContext( "pattern");
	}
	~PatternTable()
	{
		if (m_debugtrace) delete m_debugtrace;
	}

	void definePattern(
			unsigned int id,
			const std::string& expression_,
			unsigned int resultIndex,
			unsigned int level,
			analyzer::PositionBind posbind)
	{
		std::string expression( expression_);
		unsigned int editdist = extractEditDistFromExpression( expression);
		if (m_debugtrace) m_debugtrace->event( "pattern", "idx=%d expr='%s' level=%d result=%d edist=%d posbind=%d",
						(int)(m_defar.size()+1), expression.c_str(), (int)level, (int)resultIndex,
						(int)editdist, ((int)(posbind+1) % 3 - 1));
		m_defar.push_back( PatternDef( expression, 0/*subexpref*/, id, posbind, level, resultIndex, editdist));
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
		PatternSymbolTable& pst = m_symtabmap[ symtabref-1];
		uint32_t symidx = pst.symtab->getOrCreate( name);
		if (symidx != pst.idmap.size()+1)
		{
			if (symidx == 0) throw strus::runtime_error( "%s", m_errorhnd->fetchError());
			throw strus::runtime_error(_TXT("symbol defined twice: '%s'"), name.c_str());
		}
		if (m_debugtrace) m_debugtrace->event( "symbol", "patternid=%d symid=%d name='%s'", (int)patternid, (int)symbolid, name.c_str());
		pst.idmap.push_back( symbolid);
	}

	unsigned int getSymbol( unsigned int patternid, const std::string& name) const
	{
		uint8_t symtabref;
		IdSymTabMap::const_iterator yi = m_idsymtabmap.find( patternid);
		if (yi == m_idsymtabmap.end())
		{
			return 0;
		}
		else
		{
			symtabref = yi->second;
			const PatternSymbolTable& pst = m_symtabmap[ symtabref-1];
			uint32_t symidx = pst.symtab->get( name);
			return symidx?pst.idmap[ symidx-1]:0;
		}
	}

	unsigned int symbolId( uint8_t symtabref, const char* keystr, std::size_t keylen) const
	{
		const PatternSymbolTable& pst = m_symtabmap[ symtabref-1];
		uint32_t symidx = pst.symtab->get( keystr, keylen);
		return symidx?pst.idmap[ symidx-1]:0;
	}

	const PatternDef& patternDef( unsigned int id) const
	{
		return m_defar[ id-1];
	}

	hs_expr_ext_t* createPatternExprExtFlags( unsigned int edit_distance)
	{
		hs_expr_ext_t* rt = (hs_expr_ext_t*)std::calloc( 1, sizeof( hs_expr_ext_t));
		if (rt == 0) throw std::bad_alloc();
		rt->flags = HS_EXT_FLAG_EDIT_DISTANCE;
		rt->edit_distance = edit_distance;
		return rt;
	}
	///\param[in] options options to stear matching
	void complete( HsPatternTable& hspt, unsigned int options)
	{
		{
			std::vector<PatternDef>::iterator di = m_defar.begin(), de = m_defar.end();
			for (; di != de; ++di)
			{
				if (di->editdist()) break;
			}
			m_withOneByteCharMap |= (di != de);
		}
		if (m_withOneByteCharMap)
		{
			//... always do rematch expression in case of using edit dist because a match is only a hint:
			std::vector<PatternDef>::iterator di = m_defar.begin(), de = m_defar.end();
			for (; di != de; ++di)
			{
				SubExpressionReference ref( new SubExpressionDef( di->expression(), di->resultidx(), di->editdist(), true/*wchar matching*/));
				m_subexprmap.push_back( ref);
				di->setSubExpressionRef( m_subexprmap.size());
			}
		}
		else
		{
			//... do rematch expression that select a subexpression:
			std::vector<PatternDef>::iterator di = m_defar.begin(), de = m_defar.end();
			for (; di != de; ++di)
			{
				if (di->resultidx() != 0)
				{
					SubExpressionReference ref( new SubExpressionDef( di->expression(), di->resultidx(), di->editdist(), false/*byte matching*/));
					m_subexprmap.push_back( ref);
					di->setSubExpressionRef( m_subexprmap.size());
				}
			}
		}
		{
			std::vector<PatternDef>::iterator di = m_defar.begin(), de = m_defar.end();
			hspt.init( m_defar.size());
			di = m_defar.begin();
			for (std::size_t didx=0; di != de; ++di,++didx)
			{
				const char* expr;
				if (m_withOneByteCharMap)
				{
					di->setExpressionOneByteCharMap();
					expr = di->expression_onebyte().c_str();
				}
				else
				{
					expr = di->expression().c_str();
				}
				IdSymTabMap::const_iterator ti = m_idsymtabmap.find( di->id());
				if (ti != m_idsymtabmap.end())
				{
					di->setSymtabref( ti->second);
				}
				hspt.patternar[ didx] = expr;
				hspt.idar[ didx] = didx+1;
				if (di->editdist())
				{
					hspt.flagar[ didx] = options | HS_FLAG_SOM_LEFTMOST;
					hspt.extar[ didx] = createPatternExprExtFlags( di->editdist());
				}
				else if (m_withOneByteCharMap)
				{
					hspt.flagar[ didx] = options | HS_FLAG_SOM_LEFTMOST;
					hspt.extar[ didx] = 0;
				}
				else
				{
					hspt.flagar[ didx] = options | HS_FLAG_UTF8 | HS_FLAG_SOM_LEFTMOST;
					hspt.extar[ didx] = 0;
				}
			}
		}
		hspt.patternar[ m_defar.size()] = 0;
		hspt.idar[ m_defar.size()] = 0;
		hspt.flagar[ m_defar.size()] = 0;
		hspt.extar[ m_defar.size()] = 0;
	}

	bool matchSubExpression( uint32_t subexpref, const char* src, unsigned_long_long& from, unsigned_long_long& to) const
	{
		const SubExpressionDef& subedef = *m_subexprmap[ subexpref-1];
		if (subedef.editdist)
		{
			int cost = 0;
			return subedef.approx_match( src, from, to, cost);
		}
		else
		{
			return subedef.match( src, from, to);
		}
	}

	/// \brief Check, if there exists a pattern with edit distance match
	bool withOneByteCharMap() const
	{
		return m_withOneByteCharMap;
	}
	/// \brief Force mapping to a virtual character set of one byte characters used as hash and post filtering
	void forceOneByteCharMap()
	{
		m_withOneByteCharMap = true;
	}

private:
	uint8_t createSymbolTable()
	{
		if (m_symtabmap.size() >= std::numeric_limits<uint8_t>::max())
		{
			throw strus::runtime_error(_TXT("too many patter symbol tables defined, %u allowed"), (unsigned int)m_defar.size());
		}
		m_symtabmap.push_back( PatternSymbolTable());
		return m_symtabmap.size();
	}

	struct SubExpressionDef
	{
		regex_t regex;
		std::size_t index;
		unsigned int editdist;
		bool usewchar;
		enum {MaxSubexpressionIndex=99};

		SubExpressionDef( const std::string& expression, std::size_t index_, unsigned int editdist_, bool usewchar_)
			:index(index_),editdist(editdist_),usewchar(usewchar_)
		{
			if (index > MaxSubexpressionIndex+1)
			{
				throw std::runtime_error( _TXT("error in sub expression selection index out of range"));
			}
			int errcode;
			if (usewchar)
			{
				WCharString wsrc( expression.c_str(), expression.size());
				const wchar_t* wexpr = wsrc.str();
				errcode = tre_regwcomp( &regex, wexpr, REG_EXTENDED | REG_APPROX_MATCHER);
			}
			else
			{
				errcode = tre_regcomp( &regex, expression.c_str(), REG_EXTENDED | REG_APPROX_MATCHER);
			}
			if (errcode)
			{
				char errbuf[ 1024];
				(void)tre_regerror( errcode, &regex, errbuf, sizeof(errbuf));
				throw strus::runtime_error(_TXT("error compiling regular expression: %s"), errbuf);
			}
		}
		~SubExpressionDef()
		{
			tre_regfree( &regex);
		}

		bool match( const char* src, unsigned_long_long& from, unsigned_long_long& to) const
		{
			const char* start = src + from;
			regmatch_t pmatch[ MaxSubexpressionIndex+1];
			int errcode = tre_regexec( &regex, start, index+1, pmatch, REG_NOTBOL | REG_NOTEOL);
			if (errcode)
			{
				if (errcode == REG_NOMATCH) return false;

				char errbuf[ 1024];
				(void)tre_regerror( errcode, &regex, errbuf, sizeof(errbuf));
				throw strus::runtime_error(_TXT("error matching of a regular expression: %s"), errbuf);
			}
			const regmatch_t& mt0 = pmatch[ 0];
			const regmatch_t& mt = pmatch[ index];
			if (mt0.rm_so < 0 || mt.rm_so < 0 || mt.rm_eo > (regoff_t)(to - from)) return false;
			from += mt.rm_so;
			to = from + mt.rm_eo - mt.rm_so;
			return true;
		}

		bool approx_match( const char* src, unsigned_long_long& from, unsigned_long_long& to, int& cost) const
		{
			if (usewchar)
			{
				return approx_match_wchar( src, from, to, cost);
			}
			else
			{
				return approx_match_utf8( src, from, to, cost);
			}
		}
		
private:
		bool approx_match_utf8( const char* src, unsigned_long_long& from, unsigned_long_long& to, int& cost) const
		{
			const char* start = src + from;
			regaparams_t params;
			params.cost_ins = 1;
			params.cost_del = 1;
			params.cost_subst = 1;
			params.max_cost = editdist + 3;
			params.max_del = editdist + 3;
			params.max_ins = editdist + 3;
			params.max_subst = editdist + 3;
			params.max_err = editdist + 3;

			regmatch_t pmatch[ MaxSubexpressionIndex+1];
			regamatch_t amatch;
			std::memset( &amatch, 0, sizeof(amatch));
			amatch.nmatch = index+1;
			amatch.pmatch = pmatch;
			int errcode = tre_regaexec( &regex, start, &amatch, params, REG_NOTBOL | REG_NOTEOL);
			if (errcode)
			{
				if (errcode == REG_NOMATCH) return false;

				char errbuf[ 1024];
				(void)tre_regerror( errcode, &regex, errbuf, sizeof(errbuf));
				throw strus::runtime_error(_TXT("error matching (approximative) of a regular expression: %s"), errbuf);
			}
			const regmatch_t& mt0 = pmatch[ 0];
			const regmatch_t& mt = pmatch[ index];
			if (mt0.rm_so < 0 || mt.rm_so < 0) return false;
			from += mt.rm_so;
			to = from + mt.rm_eo - mt.rm_so;
			cost = amatch.cost;
			return true;
		}

		bool approx_match_wchar( const char* src, unsigned_long_long& from, unsigned_long_long& to, int& cost) const
		{
			WCharString wsrc( src + from, to - from + editdist * sizeof(wchar_t));
			const wchar_t* wstart = wsrc.str();

			regaparams_t params;
			params.cost_ins = 1;
			params.cost_del = 1;
			params.cost_subst = 1;
			params.max_cost = editdist + 3;
			params.max_del = editdist + 3;
			params.max_ins = editdist + 3;
			params.max_subst = editdist + 3;
			params.max_err = editdist + 3;

			regmatch_t pmatch[ MaxSubexpressionIndex+1];
			regamatch_t amatch;
			std::memset( &amatch, 0, sizeof(amatch));
			amatch.nmatch = index+1;
			amatch.pmatch = pmatch;
			
			int errcode = tre_regawexec( &regex, wstart, &amatch, params, REG_NOTBOL | REG_NOTEOL);
			if (errcode)
			{
				if (errcode == REG_NOMATCH) return false;

				char errbuf[ 1024];
				(void)tre_regerror( errcode, &regex, errbuf, sizeof(errbuf));
				throw strus::runtime_error(_TXT("error matching (approximative) of a regular expression: %s"), errbuf);
			}
			const regmatch_t& mt0 = pmatch[ 0];
			const regmatch_t& mt = pmatch[ index];
			if (mt0.rm_so < 0 || mt.rm_so < 0) return false;
			std::size_t orig_match_start = wsrc.origpos( mt.rm_so);
			std::size_t orig_match_end = wsrc.origpos( mt.rm_eo);
			from += orig_match_start;
			to = from + orig_match_end - orig_match_start;
			cost = amatch.cost;
			return true;
		}
private:
		SubExpressionDef( const SubExpressionDef&){}	//... non copyable
		void operator=( const SubExpressionDef&){}	//... non copyable
	};

	static bool isDigit( char ch)	{return ch >= '0' && ch <= '9';}

	unsigned int extractEditDistFromExpression( std::string& expr)
	{
		const char* si = expr.c_str();
		char const* se = si + expr.size();
		if (si == se) return 0;
		unsigned int dcnt = 0;
		for (--se; se >= si && (unsigned char)*se <= 32; --se){}
		for (; se >= si && isDigit( *se); --se,++dcnt){}
		if (dcnt > 0)
		{
			const char* ediststr = se+1;
			for (; se >= si && (unsigned char)*se <= 32; --se){}
			if (se >= si && *se == '~')
			{
				unsigned int rt = atoi( ediststr);
				for (; se > si && (unsigned char)*(se-1) <= 32; --se){}
				expr.resize( se-si);
				return rt;
			}
		}
		return 0;
	}

private:
	struct PatternSymbolTable
	{
		Reference<SymbolTable> symtab;
		std::vector<unsigned int> idmap;

		PatternSymbolTable()
			:symtab( new SymbolTable()),idmap(){}
		PatternSymbolTable( const PatternSymbolTable& o)
			:symtab(o.symtab),idmap(o.idmap){}
	};

	ErrorBufferInterface* m_errorhnd;
	DebugTraceContextInterface* m_debugtrace;
	std::vector<PatternDef> m_defar;			///< list of expressions and their attributes defined for the automaton to recognize
	std::vector<PatternSymbolTable> m_symtabmap;		///< map PatternDef::symtabref -> symbol table
	typedef std::map<uint32_t,uint8_t> IdSymTabMap;
	IdSymTabMap m_idsymtabmap;				///< map pattern id -> index in m_symtabmap == PatternDef::symtabref
	typedef Reference<SubExpressionDef> SubExpressionReference;
	std::vector<SubExpressionReference> m_subexprmap;	///< single regular expression patterns for extracting subexpressions if they are referenced.
	bool m_withOneByteCharMap;				///< true if the automaton has to be mapped down to a one byte character set serving as hash
};


struct TermMatchData
{
	PatternTable patternTable;
	hs_database_t* patterndb;

	explicit TermMatchData( ErrorBufferInterface* errorhnd_)
		:patternTable( errorhnd_),patterndb(0){}
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

class PatternLexerContext
	:public PatternLexerContextInterface
{
public:
	PatternLexerContext( const TermMatchData* data_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_data(data_),m_hs_scratch(0),m_src(0),m_matchEventAr(),m_charmap()
	{
		hs_error_t err = hs_alloc_scratch( m_data->patterndb, &m_hs_scratch);
		if (err != HS_SUCCESS)
		{
			throw std::bad_alloc();
		}
	}

	virtual ~PatternLexerContext()
	{
		hs_free_scratch( m_hs_scratch);
	}

	virtual void reset()
	{
		try
		{
			hs_scratch_t* new_scratch = 0;
			hs_error_t err = hs_alloc_scratch( m_data->patterndb, &m_hs_scratch);
			if (err != HS_SUCCESS)
			{
				throw std::bad_alloc();
			}
			hs_free_scratch( m_hs_scratch);
			m_hs_scratch = new_scratch;
			m_src = 0;
		}
		CATCH_ERROR_MAP( _TXT("error calling hyperscan lexer reset: %s"), *m_errorhnd);
	}
	
	static int match_event_handler( unsigned int patternIdx, unsigned_long_long from, unsigned_long_long to, unsigned int, void *context)
	{
		PatternLexerContext* THIS = (PatternLexerContext*)context;
		try
		{
			if (THIS->m_data->patternTable.withOneByteCharMap())
			{
				from = THIS->m_charmap.posar[ from];
				to = THIS->m_charmap.posar[ to];
			}
			if (to - from >= std::numeric_limits<uint16_t>::max())
			{
				throw strus::runtime_error( "size of matched term out of range");
			}
			const PatternDef& patternDef = THIS->m_data->patternTable.patternDef( patternIdx);
			if (patternDef.subexpref())
			{
				if (!THIS->m_data->patternTable.matchSubExpression( patternDef.subexpref(), THIS->m_src, from, to))
				{
					return 0;
				}
			}
			unsigned int patternid = patternDef.id();
			if (patternDef.symtabref())
			{
				unsigned int symid = THIS->m_data->patternTable.symbolId( patternDef.symtabref(), THIS->m_src + from, (uint32_t)(to-from));
				if (symid) patternid = symid;
			}
			MatchEvent matchEvent( patternDef.id(), patternDef.level(), patternDef.posbind(), (uint32_t)from, (uint32_t)(to-from));
			if (THIS->m_matchEventAr.empty())
			{
				THIS->m_matchEventAr.push_back( matchEvent);
				if (patternid != patternDef.id())
				{
					THIS->m_matchEventAr.push_back( MatchEvent( patternid, patternDef.level(), patternDef.posbind(), (uint32_t)from, (uint32_t)(to-from)));
				}
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
					if ((matchEvent.id == mi->id && mi->origpos == matchEvent.origpos && mi->level == matchEvent.level)
					|| (mi->level < matchEvent.level && mi->origpos + mi->origsize <= matchLastPos))
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
				if (patternid == patternDef.id())
				{
					THIS->m_matchEventAr.resize( THIS->m_matchEventAr.size() + 1);
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
				else
				{
					THIS->m_matchEventAr.resize( THIS->m_matchEventAr.size() + 2);
					mi = THIS->m_matchEventAr.rbegin();
					me = THIS->m_matchEventAr.rend();
					std::vector<MatchEvent>::reverse_iterator prev_mi = mi;
					for (mi+=2; mi != me && mi->origpos > matchEvent.origpos; prev_mi=mi++)
					{
						*prev_mi = *mi;
					}
					--mi;
					*mi = matchEvent;
					--mi;
					*mi = MatchEvent( patternid, patternDef.level(), patternDef.posbind(), (uint32_t)from, (uint32_t)(to-from));
				}
			}
			return 0;
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error calling hyperscan match event handler: %s"), *THIS->m_errorhnd, -1);
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

	virtual std::vector<analyzer::PatternLexem> match( const char* src, std::size_t srclen)
	{
		try
		{
			std::vector<analyzer::PatternLexem> rt;
			unsigned int nofExpectedTokens = srclen / 4 + 10;
			m_matchEventAr.reserve( nofExpectedTokens);
			m_src = src;
			if (srclen >= (std::size_t)std::numeric_limits<uint32_t>::max())
			{
				throw strus::runtime_error( "size of string to scan out of range");
			}
			// Collect all matches calling the Hyperscan engine:
			hs_error_t err;
			if (m_data->patternTable.withOneByteCharMap())
			{
				m_charmap.init( src, srclen);
				err = hs_scan( m_data->patterndb, m_charmap.value.c_str(), m_charmap.value.size(), 0/*reserved*/, m_hs_scratch, match_event_handler, this);
			}
			else
			{
				err = hs_scan( m_data->patterndb, src, srclen, 0/*reserved*/, m_hs_scratch, match_event_handler, this);
			}
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
			uint8_t lastposbind = (uint8_t)analyzer::BindContent;
			for (; mi != me; ++mi)
			{
				lastposbind = mi->posbind;
				switch ((analyzer::PositionBind)mi->posbind)
				{
					case analyzer::BindUnique:
					case analyzer::BindContent:
						ordpos = 1;
						origpos = mi->origpos;
						rt.push_back( analyzer::PatternLexem( mi->id, 1, 0/*origseg*/, mi->origpos, mi->origsize));
						++mi;
						goto EXITLOOP;
					case analyzer::BindSuccessor:
						rt.push_back( analyzer::PatternLexem( mi->id, 1, 0/*origseg*/, mi->origpos, mi->origsize));
						break;
					case analyzer::BindPredecessor:
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
				switch ((analyzer::PositionBind)mi->posbind)
				{
					case analyzer::BindUnique:
						if (lastposbind == (uint8_t)analyzer::BindUnique) break;
					case analyzer::BindContent:
						if (mi->origpos > origpos)
						{
							origpos = mi->origpos;
							++ordpos;
						}
						rt.push_back( analyzer::PatternLexem( mi->id, ordpos, 0/*origseg*/, mi->origpos, mi->origsize));
						break;
					case analyzer::BindSuccessor:
						rt.push_back( analyzer::PatternLexem( mi->id, ordpos+1, 0/*origseg*/, mi->origpos, mi->origsize));
						break;
					case analyzer::BindPredecessor:
						rt.push_back( analyzer::PatternLexem( mi->id, ordpos, 0/*origseg*/, mi->origpos, mi->origsize));
						break;
				}
				lastposbind = mi->posbind;
			}
			m_matchEventAr.clear();
			return rt;
		}
		CATCH_ERROR_MAP_RETURN( _TXT("failed to run pattern matching terms with regular expressions: %s"), *m_errorhnd, std::vector<analyzer::PatternLexem>());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	const TermMatchData* m_data;
	hs_scratch_t* m_hs_scratch;
	const char* m_src;
	std::vector<MatchEvent> m_matchEventAr;
	OneByteCharMap m_charmap;
};

class PatternLexerInstance
	:public PatternLexerInstanceInterface
{
public:
	explicit PatternLexerInstance( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_data(errorhnd_),m_state(DefinitionPhase),m_flags(0),m_idnamemap(),m_idnamestrings()
	{}

	virtual ~PatternLexerInstance(){}

	virtual void defineLexemName( unsigned int id, const std::string& name)
	{
		try
		{
			if (m_idnamemap.find( id) != m_idnamemap.end()) throw std::runtime_error( _TXT("duplicate definition"));
			m_idnamemap[ id] = m_idnamestrings.size()+1;
			m_idnamestrings.push_back( '\0');
			m_idnamestrings.append( name);
		}
		CATCH_ERROR_MAP( _TXT("failed to assign lexem name to lexem or symbol identifier: %s"), *m_errorhnd);
	}

	virtual const char* getLexemName( unsigned int id) const
	{
		std::map<unsigned int,std::size_t>::const_iterator li = m_idnamemap.find( id);
		if (li == m_idnamemap.end()) return 0;
		return m_idnamestrings.c_str() + li->second;
	}

	virtual void defineLexem(
			unsigned int id,
			const std::string& expression,
			unsigned int resultIndex,
			unsigned int level,
			analyzer::PositionBind posbind)
	{
		try
		{
			if (m_state != DefinitionPhase)
			{
				throw std::runtime_error( _TXT("called define pattern after calling 'compile'"));
			}
			m_data.patternTable.definePattern( id, expression, resultIndex, level, posbind);
		}
		CATCH_ERROR_MAP( _TXT("failed to define term match regular expression pattern: %s"), *m_errorhnd);
	}

	virtual void defineSymbol( unsigned int symbolid, unsigned int patternid, const std::string& name)
	{
		try
		{
			if (m_state != DefinitionPhase)
			{
				throw std::runtime_error( _TXT("called define pattern after calling 'compile'"));
			}
			m_data.patternTable.defineSymbol( symbolid, patternid, name);
		}
		CATCH_ERROR_MAP( _TXT("failed to define regular expression pattern symbol: %s"), *m_errorhnd);
	}
	virtual unsigned int getSymbol(
			unsigned int patternid,
			const std::string& name) const
	{
		try
		{
			return m_data.patternTable.getSymbol( patternid, name);
		}
		CATCH_ERROR_MAP_RETURN( _TXT("failed to retrieve regular expression pattern symbol: %s"), *m_errorhnd, 0);
	}

	virtual void defineOption( const std::string& name, double)
	{
		try
		{
			if (strus::caseInsensitiveEquals( name, "CASELESS"))
			{
				m_flags |= HS_FLAG_CASELESS;
			}
			else if (strus::caseInsensitiveEquals( name, "DOTALL"))
			{
				m_flags |= HS_FLAG_DOTALL;
			}
			else if (strus::caseInsensitiveEquals( name, "MULTILINE"))
			{
				m_flags |= HS_FLAG_MULTILINE;
			}
			else if (strus::caseInsensitiveEquals( name, "ALLOWEMPTY"))
			{
				m_flags |= HS_FLAG_ALLOWEMPTY;
			}
			else if (strus::caseInsensitiveEquals( name, "UCP"))
			{
				m_flags |= HS_FLAG_UCP;
			}
			else if (strus::caseInsensitiveEquals( name, "BYTECHAR"))
			{
				m_data.patternTable.forceOneByteCharMap();
			}
			else
			{
				throw strus::runtime_error(_TXT("unknown option '%s'"), name.c_str());
			}
			
		}
		CATCH_ERROR_MAP( _TXT("define option failed for hyperscan pattern lexer: %s"), *m_errorhnd);
	}

	virtual bool compile()
	{
		try
		{
			if (m_data.patterndb) hs_free_database( m_data.patterndb);
			m_data.patterndb = 0;

			HsPatternTable hspt;
			m_data.patternTable.complete( hspt, m_flags);

			hs_platform_info_t platform;
			std::memset( &platform, 0, sizeof(platform));
			platform.cpu_features = HS_TUNE_FAMILY_GENERIC;
			hs_compile_error_t* compile_err = 0;

			hs_error_t err =
				hs_compile_ext_multi(
					hspt.patternar, hspt.flagar, hspt.idar, hspt.extar, hspt.arsize, HS_MODE_BLOCK, &platform,
					&m_data.patterndb, &compile_err);
			if (err != HS_SUCCESS)
			{
				if (compile_err)
				{
					const char* error_pattern = compile_err->expression < 0 ?0:hspt.patternar[ compile_err->expression];
					if (error_pattern)
					{
						m_errorhnd->report(
							hyperscanErrorCode( err),
							_TXT( "failed to compile pattern \"%s\": %s\n"), error_pattern, compile_err->message);
					}
					else
					{
						m_errorhnd->report(
							hyperscanErrorCode( err),
							_TXT( "failed to build automaton from expressions: %s\n"), compile_err->message);
					}
					hs_free_compile_error( compile_err);
				}
				else
				{
					m_errorhnd->report(
						hyperscanErrorCode( err),
						_TXT( "unknown errpr building automaton from expressions\n"));
				}
				return false;
			}
			m_state = MatchPhase;
			return true;
		}
		CATCH_ERROR_MAP_RETURN( _TXT("failed to compile regular expression patterns: %s"), *m_errorhnd, false);
	}

	virtual PatternLexerContextInterface* createContext() const
	{
		try
		{
			if (m_state != MatchPhase)
			{
				throw std::runtime_error( _TXT("called create context without calling 'compile'"));
			}
			return new PatternLexerContext( &m_data, m_errorhnd);
		}
		CATCH_ERROR_MAP_RETURN( _TXT("failed to create term match context: %s"), *m_errorhnd, 0);
	}

	virtual analyzer::FunctionView view() const
	{
		return analyzer::FunctionView();
	}

private:
	ErrorBufferInterface* m_errorhnd;
	TermMatchData m_data;
	enum State {DefinitionPhase,MatchPhase};
	State m_state;
	unsigned int m_flags;
	std::map<unsigned int,std::size_t> m_idnamemap;
	std::string m_idnamestrings;
};


std::vector<std::string> PatternLexer::getCompileOptionNames() const
{
	std::vector<std::string> rt;
	static const char* ar[] = {"CASELESS", "DOTALL", "MULTILINE", "ALLOWEMPTY", "UCP", 0};
	for (std::size_t ai=0; ar[ai]; ++ai)
	{
		rt.push_back( ar[ ai]);
	}
	return rt;
}

PatternLexerInstanceInterface* PatternLexer::createInstance() const
{
	try
	{
		return new PatternLexerInstance( m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("failed to create term match instance: %s"), *m_errorhnd, 0);
}

const char* PatternLexer::getDescription() const
{
	return _TXT( "pattern lexer based the Intel hyperscan library");
}



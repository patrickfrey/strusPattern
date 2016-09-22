/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief StrusStream program implementation for loading pattern definitions from source
/// \file "patternMatchProgram.cpp"
#include "patternMatchProgram.hpp"
#include "lexems.hpp"
#include "utils.hpp"
#include "errorUtils.hpp"
#include "symbolTable.hpp"
#include "internationalization.hpp"
#include "strus/errorBufferInterface.hpp"
#include <stdexcept>
#include <new>

using namespace strus;
using namespace strus::stream;
using namespace strus::parser;

PatternMatchProgramInstance::PatternMatchProgramInstance( const TokenPatternMatchInterface* tpm, const CharRegexMatchInterface* crm, ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_)
	,m_tokenPatternMatchOptionNames(tpm->getCompileOptions())
	,m_charRegexMatchOptionNames(crm->getCompileOptions())
	,m_tokenPatternMatch(tpm->createInstance())
	,m_charRegexMatch(crm->createInstance())
{
	if (!m_tokenPatternMatch.get() || !m_charRegexMatch.get()) throw std::bad_alloc();
}

struct SourcePosition
{
	std::size_t line;
	std::size_t column;

	SourcePosition()
		:line(1),column(1){}
	SourcePosition( std::size_t line_, std::size_t column_)
		:line(line_),column(column_){}
	SourcePosition( const SourcePosition& o)
		:line(o.line),column(o.column){}
};

static SourcePosition getSourcePosition( const std::string& source, const char* itr)
{
	SourcePosition rt;
	char const* si = source.c_str();
	for (; si < itr; ++si)
	{
		if (*si == '\n')
		{
			++rt.line;
			rt.column = 1;
		}
		else if (*si == '\r')
		{}
		else if (*si == '\t')
		{
			rt.column += 8;
		}
		else
		{
			++rt.column;
		}
	}
	return rt;
}

typedef strus::TokenPatternMatchInstanceInterface::JoinOperation JoinOperation;
static JoinOperation joinOperation( const std::string& name)
{
	static const char* ar[] = {"sequence","sequence_imm","sequence_struct","within","within_struct","any","and",0};
	std::size_t ai = 0;
	for (; ar[ai]; ++ai)
	{
		if (utils::caseInsensitiveEquals( name, ar[ai]))
		{
			return (JoinOperation)ai;
		}
	}
	throw strus::runtime_error( _TXT("unknown join operation: '%s'"), name.c_str());
}

uint32_t PatternMatchProgramInstance::getOrCreateSymbol( unsigned int regexid, const std::string& name)
{
	unsigned int id = m_charRegexMatch->getSymbol( regexid, name);
	if (!id)
	{
		m_symbolRegexIdList.push_back( regexid);
		id = m_symbolRegexIdList.size() + MaxRegularExpressionNameId;
		m_charRegexMatch->defineSymbol( id, regexid, name);
	}
	return id;
}

const char* PatternMatchProgramInstance::getSymbolRegexId( unsigned int id) const
{
	return m_regexNameSymbolTab.key( m_symbolRegexIdList[ id - MaxRegularExpressionNameId -1]);
}

void PatternMatchProgramInstance::loadExpressionNode( const std::string& name, char const*& si, SubExpressionInfo& exprinfo)
{
	exprinfo.minrange = 0;
	if (isOpenOvalBracket( *si))
	{
		JoinOperation operation = joinOperation( name);

		unsigned int cardinality = 0;
		unsigned int range = 0;
		unsigned int nofArguments = 0;
		char const* lookahead = si;
		(void)parse_OPERATOR( lookahead);

		if (isCloseOvalBracket( *lookahead))
		{
			si = lookahead;
		}
		else do
		{
			(void)parse_OPERATOR( si);
			SubExpressionInfo argexprinfo;
			loadExpression( si, argexprinfo);
			switch (operation)
			{
				case TokenPatternMatchInstanceInterface::OpSequence:
					exprinfo.minrange += argexprinfo.minrange;
					break;
				case TokenPatternMatchInstanceInterface::OpSequenceImm:
					exprinfo.minrange += argexprinfo.minrange;
					break;
				case TokenPatternMatchInstanceInterface::OpSequenceStruct:
					if (nofArguments)
					{
						exprinfo.minrange += argexprinfo.minrange;
					}
					break;
				case TokenPatternMatchInstanceInterface::OpWithin:
					exprinfo.minrange += argexprinfo.minrange;
					break;
				case TokenPatternMatchInstanceInterface::OpWithinStruct:
					if (nofArguments)
					{
						exprinfo.minrange += argexprinfo.minrange;
					}
					break;
				case TokenPatternMatchInstanceInterface::OpAny:
					if (nofArguments == 0 || exprinfo.minrange < argexprinfo.minrange)
					{
						exprinfo.minrange = argexprinfo.minrange;
					}
					break;
				case TokenPatternMatchInstanceInterface::OpAnd:
					if (exprinfo.minrange > argexprinfo.minrange)
					{
						exprinfo.minrange = argexprinfo.minrange;
					}
					break;
			}
			++nofArguments;
			if (isOr( *si) || isExp( *si))
			{
				unsigned int mask = 0;
				while (isOr( *si) || isExp( *si))
				{
					if (isOr( *si) && (mask & 0x01) == 0)
					{
						mask |= 0x01;
						(void)parse_OPERATOR( si);
						if (!is_UNSIGNED( si))
						{
							throw strus::runtime_error(_TXT("unsigned integer expected as proximity range value after '|' in expression parameter list"));
						}
						range = parse_UNSIGNED( si);
					}
					else if (isExp( *si) && (mask & 0x02) == 0)
					{
						mask |= 0x02;
						(void)parse_OPERATOR( si);
						if (!is_UNSIGNED( si))
						{
							throw strus::runtime_error(_TXT("unsigned integer expected as cardinality value after '^' in expression parameter list"));
						}
						cardinality = parse_UNSIGNED( si);
					}
					else if (isComma(*si))
					{
						throw strus::runtime_error(_TXT("unexpected comma ',' after proximity range and/or cardinality specification than must only appear at the end of the arguments list"));
					}
				}
			}
		}
		while (isComma( *si));
		if (!isCloseOvalBracket( *si))
		{
			throw strus::runtime_error(_TXT("close bracket ')' expexted at end of join operation expression"));
		}
		(void)parse_OPERATOR( si);
		switch (operation)
		{
			case TokenPatternMatchInstanceInterface::OpSequenceImm:
				if (range == 0)
				{
					range = exprinfo.minrange;
				}
				else if (range < exprinfo.minrange)
				{
					throw strus::runtime_error(_TXT("rule cannot match in such a within such a small position range span: %u (required %u)"), range, exprinfo.minrange);
				}
				break;
			case TokenPatternMatchInstanceInterface::OpSequence:
			case TokenPatternMatchInstanceInterface::OpSequenceStruct:
			case TokenPatternMatchInstanceInterface::OpWithin:
			case TokenPatternMatchInstanceInterface::OpWithinStruct:
			case TokenPatternMatchInstanceInterface::OpAny:
			case TokenPatternMatchInstanceInterface::OpAnd:
				if (range == 0)
				{
					throw strus::runtime_error(_TXT("position range span must be specified for one of the operators %s"), "{'any','and','within','within_struct','sequence','sequence_struct'}");
				}
				else if (range < exprinfo.minrange)
				{
					throw strus::runtime_error(_TXT("rule cannot match in such a small position range span specified: %u (required %u)"), range, exprinfo.minrange);
				}
				break;
		}
		m_tokenPatternMatch->pushExpression( operation, nofArguments, range, cardinality);
	}
	else if (isAssign(*si))
	{
		throw strus::runtime_error(_TXT("unexpected assignment operator '=', only one assignment allowed per node"));
	}
	else
	{
		unsigned int id = m_regexNameSymbolTab.get( name);
		if (id)
		{
			if (isStringQuote(*si))
			{
				std::string symbol( parse_STRING( si));
				id = getOrCreateSymbol( id, symbol);
			}
			m_tokenPatternMatch->pushTerm( id);
		}
		else
		{
			id = m_patternNameSymbolTab.get( name);
			if (!id)
			{
				id = m_patternNameSymbolTab.getOrCreate( name);
				m_unresolvedPatternNameSet.insert( id);
			}
			m_tokenPatternMatch->pushPattern( name);
		}
		exprinfo.minrange = 1;
	}
}

void PatternMatchProgramInstance::loadExpression( char const*& si, SubExpressionInfo& exprinfo)
{
	std::string name = parse_IDENTIFIER( si);
	if (isAssign(*si))
	{
		(void)parse_OPERATOR( si);
		float weight = 1.0f;
		if (isOpenSquareBracket(*si))
		{
			(void)parse_OPERATOR( si);
			if (!is_FLOAT(si))
			{
				throw strus::runtime_error(_TXT("floating point number expected for variable assignment weight in square brackets '[' ']' after assignment operator"));
			}
			weight = parse_FLOAT( si);
			if (!isCloseSquareBracket(*si))
			{
				throw strus::runtime_error(_TXT("close square bracket expected after floating point number in variable assignment weight specification"));
			}
			(void)parse_OPERATOR( si);
		}
		std::string op = parse_IDENTIFIER( si);
		loadExpressionNode( op, si, exprinfo);
		m_tokenPatternMatch->attachVariable( name, weight);
	}
	else
	{
		loadExpressionNode( name, si, exprinfo);
	}
}

void PatternMatchProgramInstance::loadOption( char const*& si)
{
	if (isAlpha(*si))
	{
		std::string name = parse_IDENTIFIER( si);
		std::vector<std::string>::const_iterator
			oi = m_tokenPatternMatchOptionNames.begin(),
			oe = m_tokenPatternMatchOptionNames.end();
		for (; oi != oe && !utils::caseInsensitiveEquals( name, *oi); ++oi){}

		if (oi != oe)
		{
			if (isAssign(*si))
			{
				(void)parse_OPERATOR( si);
				if (is_FLOAT(si))
				{
					double value = parse_FLOAT( si);
					m_tokenPatternMatchOptions( name, value);
				}
				else
				{
					throw strus::runtime_error(_TXT("expected number as value of token pattern match option declaration"));
				}
			}
			else
			{
				throw strus::runtime_error(_TXT("expected assignment operator in token pattern match option declaration"));
			}
			return;
		}
		oi = m_charRegexMatchOptionNames.begin(),
		oe = m_charRegexMatchOptionNames.end();
		for (; oi != oe && !utils::caseInsensitiveEquals( name, *oi); ++oi){}

		if (oi != oe)
		{
			m_charRegexMatchOptions( name);
		}
		else
		{
			throw strus::runtime_error(_TXT("unknown option: '%s'"), name.c_str());
		}
	}
	else
	{
		throw strus::runtime_error(_TXT("identifier expected at start of option declaration"));
	}
}

bool PatternMatchProgramInstance::load( const std::string& source)
{
	char const* si = source.c_str();
	try
	{
		while (*si)
		{
			if (isPercent(*si))
			{
				//... we got a char regex match option or a token pattern match option
				(void)parse_OPERATOR( si);
				loadOption( si);
				continue;
			}
			bool visible = true;
			if (isDot(*si))
			{
				//... declare rule as invisible (private)
				(void)parse_OPERATOR( si);
				visible = false;
			}
			if (isAlpha(*si))
			{
				std::string name = parse_IDENTIFIER( si);
				unsigned int level = 0;
				bool has_level = false;
				if (isExp(*si))
				{
					(void)parse_OPERATOR(si);
					level = parse_UNSIGNED( si);
					has_level = true;
				}
				if (isColon( *si))
				{
					//... char regex expression declaration
					if (!visible)
					{
						throw strus::runtime_error(_TXT("unexpected colon ':' after dot '.' followed by an identifier, that starts an token pattern declaration marked as private (invisible in output)"));
					}
					unsigned int nameid = m_regexNameSymbolTab.getOrCreate( name);
					std::string regex;
					do
					{
						if (nameid > MaxRegularExpressionNameId)
						{
							throw strus::runtime_error(_TXT("too many regular expression tokens defined: %u"), nameid);
						}
						(void)parse_OPERATOR(si);

						//... Regex pattern def -> name : regex ;
						if ((unsigned char)*si > 32)
						{
							regex = parse_REGEX( si);
						}
						else
						{
							throw strus::runtime_error(_TXT("regular expression definition (inside chosen characters) expected after colon ':'"));
						}
						unsigned int resultIndex = 0;
						if (isOpenSquareBracket(*si))
						{
							(void)parse_OPERATOR(si);
							resultIndex = parse_UNSIGNED( si);
							if (!isOpenSquareBracket(*si))
							{
								throw strus::runtime_error(_TXT("close square bracket ']' expected at end of result index definition"));
							}
							(void)parse_OPERATOR(si);
						}
						CharRegexMatchInstanceInterface::PositionBind posbind = CharRegexMatchInstanceInterface::BindContent;
						if (isLeftArrow(si))
						{
							++si;
							(void)parse_OPERATOR(si);
							posbind = CharRegexMatchInstanceInterface::BindPredecessor;
						}
						else if (isRightArrow(si))
						{
							++si;
							(void)parse_OPERATOR(si);
							posbind = CharRegexMatchInstanceInterface::BindSuccessor;
						}
						m_charRegexMatch->definePattern(
							nameid, regex, resultIndex, level, posbind);
					}
					while (isOr(*si));
				}
				else if (isAssign(*si))
				{
					if (has_level)
					{
						throw strus::runtime_error(_TXT("unsupported definition of level \"^N\" in token pattern definition"));
					}
					//... token pattern expression declaration
					unsigned int nameid = m_patternNameSymbolTab.getOrCreate( name);
					do
					{
						//... Token pattern def -> name = expression ;
						(void)parse_OPERATOR(si);
						SubExpressionInfo exprinfo;
						loadExpression( si, exprinfo);
						std::set<uint32_t>::iterator ui = m_unresolvedPatternNameSet.find( nameid);
						if (ui != m_unresolvedPatternNameSet.end())
						{
							m_unresolvedPatternNameSet.erase( ui);
						}
						m_tokenPatternMatch->definePattern( name, visible);
					}
					while (isOr(*si));
				}
				else
				{
					throw strus::runtime_error(_TXT("assign '=' (token pattern definition) or colon ':' (regex pattern definition) expected after name starting a pattern declaration"));
				}
				if (!isSemiColon(*si))
				{
					throw strus::runtime_error(_TXT("semicolon ';' expected at end of rule"));
				}
				(void)parse_OPERATOR(si);
			}
			else
			{
				throw strus::runtime_error(_TXT("identifier expected at start of rule"));
			}
		}
		return true;
	}
	catch (const std::runtime_error& err)
	{
		enum {MaxErrorSnippetLen=20};
		const char* se = (const char*)std::memchr( si, '\0', MaxErrorSnippetLen);
		std::size_t snippetSize = (!se)?MaxErrorSnippetLen:(se - si);
		char snippet[ MaxErrorSnippetLen+1];
		std::memcpy( snippet, si, snippetSize);
		snippet[ snippetSize] = 0;
		std::size_t ii=0;
		for (; snippet[ii]; ++ii)
		{
			if ((unsigned char)snippet[ii] < 32)
			{
				snippet[ii] = ' ';
			}
		}
		SourcePosition errpos = getSourcePosition( source, si);
		m_errorhnd->report( _TXT("error in pattern match program at line %u, column %u: \"%s\" [at '%s']"), errpos.line, errpos.column, err.what(), snippet);
		return false;
	}
	catch (const std::bad_alloc&)
	{
		m_errorhnd->report( _TXT("out of memory when loading program source"));
		return false;
	}
}

bool PatternMatchProgramInstance::compile()
{
	try
	{
		if (m_errorhnd->hasError())
		{
			m_errorhnd->explain( _TXT("error before compile (while building program): %s"));
			return false;
		}
		if (!m_unresolvedPatternNameSet.empty())
		{
			std::ostringstream unresolved;
			std::set<uint32_t>::iterator
				ui = m_unresolvedPatternNameSet.begin(),
				ue = m_unresolvedPatternNameSet.end();
			for (std::size_t uidx=0; ui != ue && uidx<10; ++ui,++uidx)
			{
				if (!uidx) unresolved << ", ";
				unresolved << "'" << m_patternNameSymbolTab.key(*ui) << "'";
			}
			std::string unresolvedstr( unresolved.str());
			throw strus::runtime_error(_TXT("unresolved pattern references: %s"), unresolvedstr.c_str());
		}
		bool rt = true;
		rt &= m_tokenPatternMatch->compile( m_tokenPatternMatchOptions);
		rt &= m_charRegexMatch->compile( m_charRegexMatchOptions);
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("failed to compile pattern match program source: %s"), *m_errorhnd, false);
}

const CharRegexMatchInstanceInterface* PatternMatchProgramInstance::getCharRegexMatchInstance() const
{
	return m_charRegexMatch.get();
}

const TokenPatternMatchInstanceInterface* PatternMatchProgramInstance::getTokenPatternMatchInstance() const
{
	return m_tokenPatternMatch.get();
}

const char* PatternMatchProgramInstance::tokenName( unsigned int id) const
{
	if (id < MaxRegularExpressionNameId)
	{
		return m_regexNameSymbolTab.key( id);
	}
	else
	{
		return getSymbolRegexId( id);
	}
}

PatternMatchProgramInstanceInterface* PatternMatchProgram::createInstance() const
{
	try
	{
		return new PatternMatchProgramInstance( m_tpm, m_crm, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("failed to create pattern match program instance: %s"), *m_errorhnd, 0);
}


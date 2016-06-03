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
#include "compactNodeTrie.hpp"
#include "symbolTable.hpp"
#include "utils.hpp"
#include "errorUtils.hpp"
#include "internationalization.hpp"
#include "textwolf/charset_utf8.hpp"
#include <vector>
#include <string>
#include <stdexcept>

using namespace strus;
using namespace strus::stream;

static textwolf::charset::UTF8::CharLengthTab g_charLengthTab;

struct KeyAnalysis
{
	bool matchesAny;
	std::vector<std::string> prefixes;
	std::vector<std::string> suffixes;
};

struct TermMatchData
{
	conotrie::CompactNodeTrie prefixtree;
	conotrie::CompactNodeTrie suffixtree;
	conotrie::CompactNodeTrie midtree;
	SymbolTable symboltable;
};

class StreamTermMatchContext
	:public StreamTermMatchContextInterface
{
public:
	StreamTermMatchContext( const TermMatchData* data_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_data(data_){}

	virtual ~StreamTermMatchContext(){}

	virtual std::vector<stream::PatternMatchTerm> match( const std::vector<stream::PatternMatchToken>& tokens, const char* src)
	{
		try
		{
			std::vector<stream::PatternMatchTerm> rt;
			return rt;
		}
		CATCH_ERROR_MAP_RETURN( "failed to create pattern matching terms with regular expressions: %s", *m_errorhnd, std::vector<stream::PatternMatchTerm>());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	const TermMatchData* m_data;
};

class StreamTermMatchInstance
	:public StreamTermMatchInstanceInterface
{
public:
	explicit StreamTermMatchInstance( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~StreamTermMatchInstance(){}

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




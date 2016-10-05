/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/base/stdint.h"
#include "strus/lib/pattern.hpp"
#include "strus/lib/error.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/patternMatcherInterface.hpp"
#include "strus/patternMatcherInstanceInterface.hpp"
#include "strus/patternMatcherContextInterface.hpp"
#include "strus/analyzer/patternLexem.hpp"
#include "testUtils.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <memory>
#include <limits>
#include <ctime>
#include <cmath>
#include <cstring>
#include <iomanip>

#undef STRUS_LOWLEVEL_DEBUG

static void initRand()
{
	time_t nowtime;
	struct tm* now;

	::time( &nowtime);
	now = ::localtime( &nowtime);

	::srand( ((now->tm_year+1) * (now->tm_mon+100) * (now->tm_mday+1)));
}
#define RANDINT(MIN,MAX) ((std::rand()%(MAX-MIN))+MIN)

strus::ErrorBufferInterface* g_errorBuffer = 0;

struct DocumentItem
{
	unsigned int pos;
	unsigned int termid;

	DocumentItem( unsigned int pos_, unsigned int termid_)
		:pos(pos_),termid(termid_){}
	DocumentItem( const DocumentItem& o)
		:pos(o.pos),termid(o.termid){}
};

struct Document
{
	std::string id;
	std::vector<DocumentItem> itemar;

	explicit Document( const std::string& id_)
		:id(id_){}
	Document( const Document& o)
		:id(o.id),itemar(o.itemar){}
};

enum TermType {Token, SentenceDelim};
static unsigned int termId( TermType tp, unsigned int no)
{
	return ((unsigned int)tp << 24) + no;
}
#define TOKEN(no)  (((unsigned int)Token << 24) + no)
#define DELIM      (((unsigned int)SentenceDelim << 24) + 0)

static Document createDocument( unsigned int no, unsigned int size)
{
	char docid[ 32];
	snprintf( docid, sizeof(docid), "doc_%u", no);
	Document rt( docid);
	unsigned int ii = 0, ie = size;
	for (; ii < ie; ++ii)
	{
		unsigned int tok = ii+1;
		rt.itemar.push_back( DocumentItem( ii+1, termId( Token, tok)));
		if ((ii+1) % 10 == 0)
		{
			rt.itemar.push_back( DocumentItem( ii+2, termId( SentenceDelim, 0)));
		}
	}
	rt.itemar.push_back( DocumentItem( ++ii, termId( Token, 1)));
	rt.itemar.push_back( DocumentItem( ++ii, termId( Token, 1)));
	return rt;
}

typedef strus::PatternMatcherInstanceInterface::JoinOperation JoinOperation;
struct Operation
{
	enum Type {None,Term,Expression};
	Type type;
	unsigned int termid;
	unsigned int variable;
	JoinOperation joinop;
	unsigned int range;
	unsigned int cardinality;
	unsigned int argc;
};

struct Pattern
{
	const char* name;
	Operation operations[32];
	unsigned int results[32];
};

static void createPattern( strus::PatternMatcherInstanceInterface* ptinst, const char* ptname, const Operation* oplist)
{
	std::size_t oi = 0;
	for (; oplist[oi].type != Operation::None; ++oi)
	{
		const Operation& op = oplist[oi];
		switch (op.type)
		{
			case Operation::None:
				break;
			case Operation::Term:
				ptinst->pushTerm( op.termid);
				break;
			case Operation::Expression:
				ptinst->pushExpression( op.joinop, op.argc, op.range, op.cardinality);
				break;
		}
		if (op.variable)
		{
			char variablename[ 32];
			snprintf( variablename, sizeof(variablename), "A%u", (unsigned int)op.variable);
			ptinst->attachVariable( variablename, 1.0f);
		}
	}
	ptinst->definePattern( ptname, ptname[0] != '_');
}

static void createPatterns( strus::PatternMatcherInstanceInterface* ptinst, const Pattern* patterns)
{
	unsigned int pi=0;
	for (; patterns[pi].name; ++pi)
	{
		createPattern( ptinst, patterns[pi].name, patterns[pi].operations);
	}
}

static std::vector<strus::analyzer::PatternMatcherResult>
	processDocument( strus::PatternMatcherInstanceInterface* ptinst, const Document& doc)
{
	std::vector<strus::analyzer::PatternMatcherResult> results;
	std::auto_ptr<strus::PatternMatcherContextInterface> mt( ptinst->createContext());
	std::vector<DocumentItem>::const_iterator di = doc.itemar.begin(), de = doc.itemar.end();
	unsigned int didx = 0;
	for (; di != de; ++di,++didx)
	{
		mt->putInput( strus::analyzer::PatternLexem( di->termid, di->pos, 0/*origseg*/, didx, 1));
		if (g_errorBuffer->hasError()) throw std::runtime_error("error matching rules");
	}
	results = mt->fetchResults();

#ifdef STRUS_LOWLEVEL_DEBUG
	strus::utils::printResults( std::cout, std::vector<strus::SegmenterPosition>(), results);
	std::cout << "nof matches " << results.size() << std::endl;
	strus::analyzer::PatternMatcherStatistics stats = mt->getStatistics();
	strus::utils::printStatistics( std::cerr, stats);
#endif
	return results;
}

typedef strus::PatternMatcherInstanceInterface PT;
static const Pattern testPatterns[32] =
{
	{"seq[3]_1_2",
		{{Operation::Term,TOKEN(1),1},
		 {Operation::Term,TOKEN(2),2},
		 {Operation::Expression,0,0,PT::OpSequence,3,0,2}},
		{1,0}
	},
	{"seq[2]_1_2",
		{{Operation::Term,TOKEN(1),1},
		 {Operation::Term,TOKEN(2),2},
		 {Operation::Expression,0,0,PT::OpSequence,2,0,2}},
		{1,0}
	},
	{"seq[1]_1_2",
		{{Operation::Term,TOKEN(1),1},
		 {Operation::Term,TOKEN(2),2},
		 {Operation::Expression,0,0,PT::OpSequence,1,0,2}},
		{1,0}
	},
	{"seq[2]_1_3",
		{{Operation::Term,TOKEN(1),1},
		 {Operation::Term,TOKEN(3),2},
		 {Operation::Expression,0,0,PT::OpSequence,2,0,2}},
		{1,0}
	},
	{"seq[1]_1_3",
		{{Operation::Term,TOKEN(1),1},
		 {Operation::Term,TOKEN(3),2},
		 {Operation::Expression,0,0,PT::OpSequence,1,0,2}},
		{0}
	},
	{"seq[1]_1_1",
		{{Operation::Term,TOKEN(1),1},
		 {Operation::Term,TOKEN(1),2},
		 {Operation::Expression,0,0,PT::OpSequence,1,0,2}},
		{101,0}
	},
	{0,{Operation::None}}
};

int main( int argc, const char** argv)
{
	try
	{
		initRand();

		g_errorBuffer = strus::createErrorBuffer_standard( 0, 1);
		if (!g_errorBuffer)
		{
			std::cerr << "construction of error buffer failed" << std::endl;
			return -1;
		}
		else if (argc > 1)
		{
			std::cerr << "too many arguments" << std::endl;
			return 1;
		}
		unsigned int documentSize = 100;

		std::auto_ptr<strus::PatternMatcherInterface> pt( strus::createPatternMatcher_stream( g_errorBuffer));
		if (!pt.get()) throw std::runtime_error("failed to create pattern matcher");
		std::auto_ptr<strus::PatternMatcherInstanceInterface> ptinst( pt->createInstance());
		if (!ptinst.get()) throw std::runtime_error("failed to create pattern matcher instance");
		createPatterns( ptinst.get(), testPatterns);
		ptinst->compile( strus::analyzer::PatternMatcherOptions());

		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error( "error creating automaton for evaluating rules");
		}
		Document doc = createDocument( 1, documentSize);
		std::cerr << "starting rule evaluation ..." << std::endl;

		// Evaluate results:
		std::vector<strus::analyzer::PatternMatcherResult> 
			results = processDocument( ptinst.get(), doc);

		// Verify results:
		std::vector<strus::analyzer::PatternMatcherResult>::const_iterator
			ri = results.begin(), re = results.end();

		typedef std::pair<std::string,unsigned int> Match;
		std::set<Match> matches;
		for (;ri != re; ++ri)
		{
			matches.insert( Match( ri->name(), ri->ordpos()));
		}
		unsigned int ti=0;
		for (; testPatterns[ti].name; ++ti)
		{
			unsigned int ei=0;
			for (; testPatterns[ti].results[ei]; ++ei)
			{
				std::set<Match>::iterator
					mi = matches.find( Match( testPatterns[ti].name, testPatterns[ti].results[ei]));
				if (mi == matches.end())
				{
					char numbuf[ 64];
					::snprintf( numbuf, sizeof(numbuf), "%u", testPatterns[ti].results[ei]);
					throw std::runtime_error( std::string("expected match not found '") + testPatterns[ti].name + "' at ordpos " + numbuf);
				}
				else
				{
					matches.erase( mi);
				}
			}
		}
		if (!matches.empty())
		{
			std::set<Match>::const_iterator mi = matches.begin(), me = matches.end();
			for (; mi != me; ++mi)
			{
				std::cerr << "unexpected match of '" << mi->first << "' at ordpos " << mi->second << std::endl;
			}
			throw std::runtime_error( "more matches found than expected");
		}
		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error("error matching rule");
		}
		std::cerr << "OK" << std::endl;
		delete g_errorBuffer;
		return 0;
	}
	catch (const std::runtime_error& err)
	{
		if (g_errorBuffer->hasError())
		{
			std::cerr << "error processing pattern matching: "
					<< g_errorBuffer->fetchError() << " (" << err.what()
					<< ")" << std::endl;
		}
		else
		{
			std::cerr << "error processing pattern matching: "
					<< err.what() << std::endl;
		}
	}
	catch (const std::bad_alloc&)
	{
		std::cerr << "out of memory processing pattern matching" << std::endl;
	}
	delete g_errorBuffer;
	return -1;
}



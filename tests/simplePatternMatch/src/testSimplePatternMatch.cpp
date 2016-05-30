/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/base/stdint.h"
#include "strus/lib/stream.hpp"
#include "strus/lib/error.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/streamPatternMatchInterface.hpp"
#include "strus/streamPatternMatchInstanceInterface.hpp"
#include "strus/streamPatternMatchContextInterface.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <limits>
#include <ctime>
#include <cmath>
#include <cstring>
#include <iomanip>

#undef STRUS_LOWLEVEL_DEBUG
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
	return rt;
}

typedef strus::StreamPatternMatchInstanceInterface::JoinOperation JoinOperation;
struct Operation
{
	enum Type {None,Term,Expression};
	Type type;
	unsigned int termid;
	JoinOperation joinop;
	unsigned int range;
	unsigned int cardinality;
	unsigned int argc;
	unsigned int variable;
};

struct Pattern
{
	const char* name;
	Operation operations[32];
};

static void createPattern( strus::StreamPatternMatchInstanceInterface* ptinst, const char* ptname, const Operation* oplist)
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
	ptinst->closePattern( ptname, ptname[0] != '_');
}

static void createPatterns( strus::StreamPatternMatchInstanceInterface* ptinst, const Pattern* patterns)
{
	unsigned int pi=0;
	for (; patterns[pi].name; ++pi)
	{
		createPattern( ptinst, patterns[pi].name, patterns[pi].operations);
	}
}

static unsigned int matchRules( strus::StreamPatternMatchInstanceInterface* ptinst, const Document& doc)
{
	std::auto_ptr<strus::StreamPatternMatchContextInterface> mt( ptinst->createContext());
	std::vector<DocumentItem>::const_iterator di = doc.itemar.begin(), de = doc.itemar.end();
	unsigned int didx = 0;
	for (; di != de; ++di,++didx)
	{
		mt->putInput( di->termid, di->pos, didx, 1);
		if (g_errorBuffer->hasError()) throw std::runtime_error("error matching rules");
	}
	std::vector<strus::stream::PatternMatchResult> results = mt->fetchResults();
	unsigned int nofMatches = results.size();

#ifdef STRUS_LOWLEVEL_DEBUG
	std::vector<strus::stream::PatternMatchResult>::const_iterator
		ri = results.begin(), re = results.end();
	for (; ri != re; ++ri)
	{
		std::cout << "match '" << ri->name() << "':";
		std::vector<strus::stream::PatternMatchResultItem>::const_iterator
			ei = ri->itemlist().begin(), ee = ri->itemlist().end();
	
		for (; ei != ee; ++ei)
		{
			std::cout << " " << ei->name() << " [" << ei->ordpos()
					<< ", " << ei->origpos() << ", " << ei->origsize() << "]";
		}
		std::cout << std::endl;
	}
	strus::StreamPatternMatchContextInterface::Statistics stats = mt->getStatistics();
	std::cout << "nof matches " << nofMatches;
	std::vector<strus::StreamPatternMatchContextInterface::Statistics::Item>::const_iterator
		li = stats.items().begin(), le = stats.items().end();
	for (; li != le; ++li)
	{
		std::cout << ", " << li->name() << " " << (int)(li->value()+0.5);
	}
	std::cout << std::endl;
#endif
	return nofMatches;
}

typedef strus::StreamPatternMatchInstanceInterface PT;
static const Pattern testPatterns[32] =
{
	{"seq[3]_1_2",
		{{Operation::Term,TOKEN(1)},
		 {Operation::Term,TOKEN(2)},
		 {Operation::Expression,0,PT::OpSequence,3,0,2,1}}
	},
	{"seq[2]_1_2",
		{{Operation::Term,TOKEN(1)},
		 {Operation::Term,TOKEN(2)},
		 {Operation::Expression,0,PT::OpSequence,2,0,2,1}}
	},
	{"seq[1]_1_2",
		{{Operation::Term,TOKEN(1)},
		 {Operation::Term,TOKEN(2)},
		 {Operation::Expression,0,PT::OpSequence,1,0,2,1}}
	},
	{"seq[2]_1_3",
		{{Operation::Term,TOKEN(1)},
		 {Operation::Term,TOKEN(3)},
		 {Operation::Expression,0,PT::OpSequence,2,0,2,1}}
	},
	{"seq[1]_1_3",
		{{Operation::Term,TOKEN(1)},
		 {Operation::Term,TOKEN(3)},
		 {Operation::Expression,0,PT::OpSequence,1,0,2,1}}
	},
	{0,{Operation::None}}
};

int main( int argc, const char** argv)
{
	try
	{
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

		std::auto_ptr<strus::StreamPatternMatchInterface> pt( strus::createStreamPatternMatch_standard( g_errorBuffer));
		if (!pt.get()) throw std::runtime_error("failed to create pattern matcher");
		std::auto_ptr<strus::StreamPatternMatchInstanceInterface> ptinst( pt->createInstance());
		if (!ptinst.get()) throw std::runtime_error("failed to create pattern matcher instance");
		createPatterns( ptinst.get(), testPatterns);
		ptinst->optimize( strus::StreamPatternMatchInstanceInterface::OptimizeOptions(0.5f,1.1f,5));

		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error( "error creating automaton for evaluating rules");
		}
		Document doc = createDocument( 1, documentSize);
		unsigned int totalNofmatches = 0;
		std::cerr << "starting rule evaluation ..." << std::endl;

		unsigned int nofmatches = matchRules( ptinst.get(), doc);
		totalNofmatches += nofmatches;
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



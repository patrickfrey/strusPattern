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
#include <ctime>

#define STRUS_LOWLEVEL_DEBUG
#define RANDINT(MIN,MAX) ((rand()%(MAX-MIN))+MIN)

strus::ErrorBufferInterface* g_errorBuffer = 0;

struct DocumentItem
{
	unsigned int pos;
	std::string type;
	std::string value;

	DocumentItem( unsigned int pos_, std::string type_, std::string value_)
		:pos(pos_),type(type_),value(value_){}
	DocumentItem( const DocumentItem& o)
		:pos(o.pos),type(o.type),value(o.value){}
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

static std::string termValue( unsigned int val)
{
	char buf[ 32];
	snprintf( buf, 32, "%u", val);
	return std::string( buf);
}

static Document createDocument( unsigned int size, unsigned int incr, unsigned int mod)
{
	Document rt( std::string("doc_")
			+ termValue(size) + "_" + termValue(incr) + "_" + termValue(mod));
	unsigned int ii = 0, ie = size, delimcnt = incr;
	unsigned int tok = incr;
	for (; ii < ie; ++ii)
	{
		unsigned int val = mod ? (tok % mod) : tok;
		tok = val + incr;
		rt.itemar.push_back( DocumentItem( ii+1, "num", termValue( tok)));
		if (--delimcnt == 0)
		{
			delimcnt = incr;
			rt.itemar.push_back( DocumentItem( ii+1, "sent", ""));
		}
	}
	return rt;
}

static void createTermOpRule( strus::StreamPatternMatchInstanceInterface* ptinst, const char* operation, unsigned int range, unsigned int cardinality, unsigned int* param, std::size_t paramsize)
{
	std::string type( "num");
	std::size_t pi = 0, pe = paramsize;
	for (; pi != pe; ++pi)
	{
		unsigned int termid = ptinst->getTermId( type, termValue(param[pi]));
		ptinst->pushTerm( termid);
		ptinst->attachVariable( std::string("A") + termValue(pi));
	}
	ptinst->pushExpression( operation, paramsize, range, cardinality);
}

static void createTermOpPattern( strus::StreamPatternMatchInstanceInterface* ptinst, const char* operation, unsigned int range, unsigned int cardinality, unsigned int* param, std::size_t paramsize)
{
	std::string rulename = operation;
	std::size_t pi = 0, pe = paramsize;
	for (; pi != pe; ++pi)
	{
		rulename.push_back( '_');
		rulename.append( termValue( param[pi]));
	}
	createTermOpRule( ptinst, operation, range, cardinality, param, paramsize);
	ptinst->closePattern( rulename, true);
}

static void createRules( strus::StreamPatternMatchInstanceInterface* ptinst)
{
	unsigned int ti=0, te=100;
	unsigned int mi=0, me=100;
	for (; ti < te; ++ti)
	{
		for (; mi < me; ++mi)
		{
			unsigned int range = 2;
			unsigned int cardinality = 0;
			unsigned int param[3] = { ti+1, mi+1, 0 };

			createTermOpPattern( ptinst, "sequence", range, cardinality, param, 2);
			unsigned int ai=0, ae=10;
			for (; ai < ae; ++ai)
			{
				param[2] = ai+1;
				createTermOpPattern( ptinst, "sequence", range, cardinality, param, 3);
			}
		}
	}
}

static std::vector<Document> createDocuments()
{
	std::vector<Document> rt;
	unsigned int ti=0, te=100;
	unsigned int mi=0, me=100;
	unsigned int size = 1000;

	for (; ti < te; ++ti)
	{
		for (; mi < me; ++mi)
		{
			Document doc( createDocument( size, ti, mi));
			rt.push_back( doc);
		}
	}
	return rt;
}

static unsigned int matchRules( strus::StreamPatternMatchInstanceInterface* ptinst, const Document& doc)
{
	std::auto_ptr<strus::StreamPatternMatchContextInterface> mt( ptinst->createContext());
	std::vector<DocumentItem>::const_iterator di = doc.itemar.begin(), de = doc.itemar.end();
	unsigned int didx = 0;
	for (; di != de; ++di,++didx)
	{
		unsigned int termid = mt->termId( di->type, di->value);
		mt->putInput( termid, di->pos, didx, 1);
	}
	std::vector<strus::stream::PatternMatchResult> results = mt->fetchResult();
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
	}
	std::cout << "nof matches " << nofMatches << std::endl;
#endif
	return nofMatches;
}


int main( int, const char**)
{
	try
	{
		g_errorBuffer = strus::createErrorBuffer_standard( 0, 1);
		if (!g_errorBuffer)
		{
			std::cerr << "construction of error buffer failed" << std::endl;
			return -1;
		}
		std::auto_ptr<strus::StreamPatternMatchInterface> pt( strus::createStreamPatternMatch_standard( g_errorBuffer));
		if (!pt.get()) throw std::runtime_error("failed to create pattern matcher");
		std::auto_ptr<strus::StreamPatternMatchInstanceInterface> ptinst( pt->createInstance());
		if (!ptinst.get()) throw std::runtime_error("failed to create pattern matcher instance");

		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error("uncaugth exception");
		}
		std::vector<Document> docs = createDocuments();
		unsigned int totalNofmatches = 0;

		std::clock_t start = std::clock();
		std::vector<Document>::const_iterator di = docs.begin(), de = docs.end();
		for (; di != de; ++di)
		{
			unsigned int nofmatches = matchRules( ptinst.get(), *di);
			totalNofmatches += nofmatches;
		}
		double duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
		std::cerr << "OK" << std::endl;
		std::cerr << "processed " << docs.size() << " documents with total " << totalNofmatches << " in " << doubleToString(duration) << " seconds" << std::endl;
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



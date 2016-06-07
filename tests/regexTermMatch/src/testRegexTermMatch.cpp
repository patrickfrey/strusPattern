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
#include "strus/streamTermMatchInterface.hpp"
#include "strus/streamTermMatchInstanceInterface.hpp"
#include "strus/streamTermMatchContextInterface.hpp"
#include "strus/stream/patternMatchTerm.hpp"
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

#define STRUS_LOWLEVEL_DEBUG

strus::ErrorBufferInterface* g_errorBuffer = 0;

struct PatternDef
{
	unsigned int id;
	const char* expression;
	unsigned int resultIndex;
	unsigned int level;
	bool haspos;
};

struct SymbolDef
{
	unsigned int id;
	unsigned int patternid;
	const char* name;
};

struct TestDef
{
	const PatternDef patterns[64];
	const SymbolDef symbols[64];
	const char* src;
};

static void compile( strus::StreamTermMatchInstanceInterface* ptinst, const PatternDef* par, const SymbolDef* sar)
{
	std::size_t pi = 0;
	for (; par[pi].expression; ++pi)
	{
		strus::StreamTermMatchInstanceInterface::PositionBind posbind = par[pi].haspos
			? strus::StreamTermMatchInstanceInterface::BindContent
			: strus::StreamTermMatchInstanceInterface::BindPredecessor;
		ptinst->definePattern( par[pi].id, par[pi].expression, par[pi].resultIndex, par[pi].level, posbind);
	}
	for (pi=0; sar[pi].name; ++pi)
	{
		ptinst->defineSymbol( sar[pi].id, sar[pi].patternid, sar[pi].name);
	}
	if (!ptinst->compile())
	{
		throw std::runtime_error("error building term match automaton");
	}
}

static std::vector<strus::stream::PatternMatchTerm>
	match( strus::StreamTermMatchInstanceInterface* ptinst, const std::string& src)
{
	std::auto_ptr<strus::StreamTermMatchContextInterface> mt( ptinst->createContext());
	std::vector<strus::stream::PatternMatchTerm> rt = mt->match( src.c_str(), src.size());

#ifdef STRUS_LOWLEVEL_DEBUG
	std::vector<strus::stream::PatternMatchTerm>::const_iterator
		ri = rt.begin(), re = rt.end();
	for (; ri != re; ++ri)
	{
		std::string content( src.c_str() + ri->origpos(), ri->origsize());
		std::cout << "term " << ri->id() << " '" << content << "' at " << ri->ordpos() << " [" << ri->origpos() << ":" << ri->origsize() << "]" << std::endl;
	}
#endif
	return rt;
}

static const TestDef g_tests[32] =
{
	{
		{
			{1,"[0-9]+\\b",0,2,true},
			{2,"[a-zA-Z0-9]+\\b",0,1,true},
			{3,"([0-9]+)th\\b",1,2,false},
			{4,"\\w+\\s\\w+\\s\\w+\\s\\w+\\b",0,1,false},
			{0,0,0,0,false}
		},
		{
			{10,2,"The"},
			{10,2,"believe"},
			{0,0,0}
		},
		"The world was not created about 5000 years ago as some creationists still believe in the 21th century."
	},
	{
		{
			{0,0,0,0,false}
		},
		{
			{0,0,0}
		},
		0
	}
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
		std::auto_ptr<strus::StreamTermMatchInterface> pt( strus::createStreamTermMatch_standard( g_errorBuffer));
		if (!pt.get()) throw std::runtime_error("failed to create regular expression term matcher");
		std::size_t ti = 0;
		for (; g_tests[ti].src; ++ti)
		{
			std::cerr << "executing test " << ti << ":" << std::endl;
			strus::StreamTermMatchInterface::Options opt;
			opt("DOTALL");
			std::auto_ptr<strus::StreamTermMatchInstanceInterface> ptinst( pt->createInstance( opt));
			if (!ptinst.get()) throw std::runtime_error("failed to create regular expression term matcher instance");
			compile( ptinst.get(), g_tests[ti].patterns, g_tests[ti].symbols);
			if (g_errorBuffer->hasError())
			{
				throw std::runtime_error( "error building automaton for test");
			}
			match( ptinst.get(), g_tests[ti].src);
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



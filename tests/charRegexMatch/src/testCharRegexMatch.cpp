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
#include "strus/patternLexerInterface.hpp"
#include "strus/patternLexerInstanceInterface.hpp"
#include "strus/patternLexerContextInterface.hpp"
#include "strus/analyzer/patternLexem.hpp"
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

struct ResultDef
{
	unsigned int id;
	unsigned int ordpos;
	unsigned int origpos;
	unsigned int origsize;
};

struct TestDef
{
	const PatternDef patterns[64];
	const SymbolDef symbols[64];
	const char* src;
	ResultDef result[128];
};

static void compile( strus::PatternLexerInstanceInterface* ptinst, const PatternDef* par, const SymbolDef* sar)
{
	std::size_t pi = 0;
	for (; par[pi].expression; ++pi)
	{
		strus::analyzer::PositionBind posbind = par[pi].haspos
			? strus::analyzer::BindContent
			: strus::analyzer::BindPredecessor;
		ptinst->defineLexem( par[pi].id, par[pi].expression, par[pi].resultIndex, par[pi].level, posbind);
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

static std::vector<strus::analyzer::PatternLexem>
	match( strus::PatternLexerInstanceInterface* ptinst, const std::string& src)
{
	std::auto_ptr<strus::PatternLexerContextInterface> mt( ptinst->createContext());
	std::vector<strus::analyzer::PatternLexem> rt = mt->match( src.c_str(), src.size());

#ifdef STRUS_LOWLEVEL_DEBUG
	std::vector<strus::analyzer::PatternLexem>::const_iterator
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
		"The world was not created about 5000 years ago as some creationists still believe in the 21th century.",
		{
			{2,1,0,3},
			{10,1,0,3},
			{4,1,0,17},
			{2,2,4,5},
			{4,2,4,21},
			{2,3,10,3},
			{4,3,10,21},
			{2,4,14,3},
			{4,4,14,22},
			{2,5,18,7},
			{4,5,18,24},
			{2,6,26,5},
			{4,6,26,20},
			{1,7,32,4},
			{4,7,32,17},
			{2,8,37,5},
			{4,8,37,17},
			{2,9,43,3},
			{4,9,43,24},
			{2,10,47,2},
			{4,10,47,26},
			{2,11,50,4},
			{4,11,50,31},
			{2,12,55,12},
			{4,12,55,29},
			{2,13,68,5},
			{4,13,68,20},
			{2,14,74,7},
			{10,14,74,7},
			{4,14,74,19},
			{2,15,82,2},
			{4,15,82,19},
			{2,16,85,3},
			{2,17,89,4},
			{3,17,89,2},
			{2,18,94,7},
			{0,0,0,0}
		}
	},
	{
		{
			{1,"abc ~1",0,1,true},
			{0,0,0,0,false}
		},
		{
			{0,0,0}
		},
		"aabc abc abbc bbb ac aaa ab",
		{
			{1,1,1,3},
			{1,2,5,3},
			{1,3,9,3},
			{1,4,18,2},
			{1,5,25,2},
			{0,0,0,0}
		}
	},
	{
		{
			{1,"a\xC3\xB6\xC3\xBC ~1",0,1,true}, //... "aöü"
			{0,0,0,0,false}
		},
		{
			{0,0,0}
		},
		"aa\xC3\xB6\xC3\xBC a\xC3\xB6\xC3\xBC a\xC3\xB6\xC3\xB6\xC3\xBC \xC3\xB6\xC3\xB6\xC3\xB6 a\xC3\xBC aaa a\xC3\xB6",
		{
			{1,1,1,5},
			{1,2,7,5},
			{1,3,13,5},
			{1,4,28,3},
			{1,5,36,3}, 
			{0,0,0,0}
		}
	},
	{
		{
			{0,0,0,0,false}
		},
		{
			{0,0,0}
		},
		0,
		{
			{0,0,0,0}
		}
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
		std::auto_ptr<strus::PatternLexerInterface> pt( strus::createPatternLexer_stream( g_errorBuffer));
		if (!pt.get()) throw std::runtime_error("failed to create regular expression term matcher");
		std::size_t ti = 0;
		for (; g_tests[ti].src; ++ti)
		{
			std::cerr << "executing test " << (ti+1) << std::endl;
			std::auto_ptr<strus::PatternLexerInstanceInterface> ptinst( pt->createInstance());
			if (!ptinst.get()) throw std::runtime_error("failed to create regular expression term matcher instance");

			ptinst->defineOption( "DOTALL", 0);
			compile( ptinst.get(), g_tests[ti].patterns, g_tests[ti].symbols);
			if (g_errorBuffer->hasError())
			{
				throw std::runtime_error( "error building automaton for test");
			}
			std::vector<strus::analyzer::PatternLexem> result = match( ptinst.get(), g_tests[ti].src);
			if (result.empty() && g_errorBuffer->hasError())
			{
				throw std::runtime_error( "error matching");
			}
			std::vector<strus::analyzer::PatternLexem>::const_iterator ri = result.begin(), re = result.end();
			std::size_t ridx=0;
			const ResultDef* expected = g_tests[ti].result;
			for (; ri != re && expected[ridx].origsize != 0; ++ridx,++ri)
			{
				const ResultDef& exp = expected[ridx];
				if (exp.id != ri->id()) break;
				if (exp.ordpos != ri->ordpos()) break;
				if (exp.origpos != ri->origpos()) break;
				if (exp.origsize != ri->origsize()) break;
			}
			if (ri != re || expected[ridx].origsize != 0)
			{
				throw std::runtime_error( "test failed");
			}
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



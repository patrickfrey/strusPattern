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
#include "strus/patternMatcherInterface.hpp"
#include "strus/patternMatcherInstanceInterface.hpp"
#include "strus/patternMatcherContextInterface.hpp"
#include "strus/analyzer/patternMatcherResult.hpp"
#include "strus/analyzer/patternMatcherStatistics.hpp"
#include "strus/patternLexerInterface.hpp"
#include "strus/patternLexerInstanceInterface.hpp"
#include "strus/patternLexerContextInterface.hpp"
#include "strus/patternMatcherProgramInterface.hpp"
#include "strus/patternMatcherProgramInstanceInterface.hpp"
#include "strus/base/fileio.hpp"
#include "testUtils.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <memory>
#include <limits>
#include <cstring>
#include <iomanip>

#undef STRUS_LOWLEVEL_DEBUG

strus::ErrorBufferInterface* g_errorBuffer = 0;

static void printUsage( int argc, const char* argv[])
{
	std::cerr << "usage: " << argv[0] << " <program> <inputfile>" << std::endl;
	std::cerr << "<program>   = program to load" << std::endl;
	std::cerr << "<inputfile> = input file to process" << std::endl;
}

static bool diffContent( const std::string& res, const std::string& exp)
{
	std::string::const_iterator ri = res.begin(), re = res.end();
	std::string::const_iterator ei = exp.begin(), ee = exp.end();
	while (ei != ee && ri != re)
	{
		while (ei != ee && (*ei == '\n' || *ei == '\r')) ++ei;
		while (ri != re && (*ri == '\n' || *ri == '\r')) ++ri;
		if (ei == ee || ri == re || *ei != *ri) break;
		++ei;
		++ri;
	}
	while (ei != ee && (*ei == '\n' || *ei == '\r')) ++ei;
	while (ri != re && (*ri == '\n' || *ri == '\r')) ++ri;
	return (ei == ee && ri == re);
}

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
		else if (argc > 3)
		{
			printUsage( argc, argv);
			std::cerr << "too many arguments" << std::endl;
			return 1;
		}
		else if (argc < 3)
		{
			printUsage( argc, argv);
			std::cerr << "too few arguments" << std::endl;
			return 1;
		}
		// Create objects:
		std::auto_ptr<strus::PatternMatcherInterface> pti( strus::createPatternMatcher_standard( g_errorBuffer));
		if (!pti.get()) throw std::runtime_error("failed to create pattern matcher");
		std::auto_ptr<strus::PatternLexerInterface> cri( strus::createPatternLexer_standard( g_errorBuffer));
		if (!cri.get()) throw std::runtime_error("failed to create char regex matcher");
		std::auto_ptr<strus::PatternMatcherProgramInterface> ppi( strus::createPatternMatcherProgram_standard( pti.get(), cri.get(), g_errorBuffer));
		if (!ppi.get()) throw std::runtime_error("failed to create pattern program loader");
		std::auto_ptr<strus::PatternMatcherProgramInstanceInterface> pii( ppi->createInstance());
		if (!pii.get()) throw std::runtime_error("failed to create pattern program loader instance");

		// Load program:
		std::string programfile( argv[ 1]);
		std::string programsrc;
		unsigned int ec;
		ec = strus::readFile( programfile, programsrc);
		if (ec)
		{
			char errmsg[ 256];
			::snprintf( errmsg, sizeof(errmsg), "error (%u) reading program source '%s': %s", ec, programfile.c_str(), ::strerror(ec));
			throw std::runtime_error( errmsg);
		}
		if (!pii->load( programsrc))
		{
			throw std::runtime_error( "error loading pattern match program source");
		}
		if (!pii->compile())
		{
			throw std::runtime_error( "error compiling pattern match program");
		}

		// Load input:
		std::string inputfile( argv[ 2]);
		std::string inputsrc;
		ec = strus::readFile( inputfile, inputsrc);
		if (ec)
		{
			char errmsg[ 256];
			::snprintf( errmsg, sizeof(errmsg), "error (%u) reading input file '%s': %s", ec, inputfile.c_str(), ::strerror(ec));
			throw std::runtime_error( errmsg);
		}

		// Load expected output:
		char const* argv2ext = std::strchr( argv[ 2], '.');
		if (argv2ext)
		{
			char const* aa = std::strchr( argv2ext+1, '.');
			while (aa)
			{
				argv2ext = aa;
				aa = std::strchr( argv2ext+1, '.');
			}
		}
		else
		{
			argv2ext = std::strchr( argv[ 2], '\0');
		}
		std::string expectedfile( argv[ 2], argv2ext - argv[ 2]);
		expectedfile.append( ".res");
		std::string expectedsrc;
		ec = strus::readFile( expectedfile, expectedsrc);
		if (ec)
		{
			char errmsg[ 256];
			::snprintf( errmsg, sizeof(errmsg), "error (%u) reading expected file '%s': %s", ec, expectedfile.c_str(), ::strerror(ec));
			throw std::runtime_error( errmsg);
		}

		// Scan source with char regex automaton for tokens:
		const strus::PatternLexerInstanceInterface* crinst = pii->getPatternLexerInstance();
		std::auto_ptr<strus::PatternLexerContextInterface> crctx( crinst->createContext());

		std::vector<strus::analyzer::PatternLexem> crmatches = crctx->match( inputsrc.c_str(), inputsrc.size());
		if (crmatches.size() == 0 && g_errorBuffer->hasError())
		{
			throw std::runtime_error( "failed to scan for tokens with char regex match automaton");
		}
		std::ostringstream resultstrbuf;

		// Scan tokens with token pattern match automaton and print results:
		const strus::PatternMatcherInstanceInterface* ptinst = pii->getPatternMatcherInstance();
		std::auto_ptr<strus::PatternMatcherContextInterface> ptctx( ptinst->createContext());
		std::vector<strus::analyzer::PatternLexem>::const_iterator
			ci = crmatches.begin(), ce = crmatches.end();
		for (; ci != ce; ++ci)
		{
			std::string tokstr( std::string( inputsrc.c_str() + ci->origpos(), ci->origsize()));
			resultstrbuf << "token " << pii->tokenName(ci->id()) << "(" << ci->id() << ") at " << ci->ordpos() 
					<< "[" << ci->origpos() << ":" << ci->origsize() << "] '"
					<< tokstr << "'" << std::endl;
			ptctx->putInput( *ci);
		}
		std::vector<strus::analyzer::PatternMatcherResult> results = ptctx->fetchResults();
		if (results.size() == 0 && g_errorBuffer->hasError())
		{
			throw std::runtime_error( "failed to scan for patterns with token pattern match automaton");
		}

		// Print results to buffer:
		strus::utils::printResults( resultstrbuf, std::vector<strus::SegmenterPosition>(), results, inputsrc.c_str());
		strus::analyzer::PatternMatcherStatistics stats = ptctx->getStatistics();
		strus::utils::printStatistics( resultstrbuf, stats);

		// Print result to stdout and verify result by comparing it with the expected output:
		std::string resultstr = resultstrbuf.str();
		std::cout << resultstr << std::endl;
		if (!diffContent( expectedsrc, resultstr))
		{
			throw std::runtime_error( "output and expected result differ");
		}

		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error( "uncaught error");
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



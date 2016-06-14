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
#include "strus/tokenPatternMatchInterface.hpp"
#include "strus/tokenPatternMatchInstanceInterface.hpp"
#include "strus/tokenPatternMatchContextInterface.hpp"
#include "strus/stream/tokenPatternMatchResult.hpp"
#include "strus/stream/tokenPatternMatchStatistics.hpp"
#include "strus/stream/patternMatchToken.hpp"
#include "strus/charRegexMatchInterface.hpp"
#include "strus/charRegexMatchInstanceInterface.hpp"
#include "strus/charRegexMatchContextInterface.hpp"
#include "strus/patternMatchProgramInterface.hpp"
#include "strus/patternMatchProgramInstanceInterface.hpp"
#include "strus/base/fileio.hpp"
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
		std::auto_ptr<strus::TokenPatternMatchInterface> pti( strus::createTokenPatternMatch_standard( g_errorBuffer));
		if (!pti.get()) throw std::runtime_error("failed to create pattern matcher");
		std::auto_ptr<strus::CharRegexMatchInterface> cri( strus::createCharRegexMatch_standard( g_errorBuffer));
		if (!cri.get()) throw std::runtime_error("failed to create char regex matcher");
		std::auto_ptr<strus::PatternMatchProgramInterface> ppi( strus::createPatternMatchProgram_standard( pti.get(), cri.get(), g_errorBuffer));
		if (!ppi.get()) throw std::runtime_error("failed to create pattern program loader");
		std::auto_ptr<strus::PatternMatchProgramInstanceInterface> pii( ppi->createInstance());
		if (!pii.get()) throw std::runtime_error("failed to create pattern program loader instance");

		// Load program:
		std::string programfile( argv[ 1]);
		std::string programsrc;
		unsigned int ec;
		ec = strus::readFile( programfile, programsrc);
		if (ec)
		{
			char errmsg[ 256];
			::snprintf( errmsg, sizeof(errmsg), "error (%u) reading program source: %s", ec, ::strerror(ec));
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
			::snprintf( errmsg, sizeof(errmsg), "error (%u) reading input file: %s", ec, ::strerror(ec));
			throw std::runtime_error( errmsg);
		}

		// Scan source with char regex automaton for tokens:
		const strus::CharRegexMatchInstanceInterface* crinst = pii->getCharRegexMatchInstance();
		std::auto_ptr<strus::CharRegexMatchContextInterface> crctx( crinst->createContext());

		std::vector<strus::stream::PatternMatchToken> crmatches = crctx->match( inputsrc.c_str(), inputsrc.size());
		if (crmatches.size() == 0 && g_errorBuffer->hasError())
		{
			throw std::runtime_error( "failed to scan for tokens with char regex match automaton");
		}

		// Scan tokens with token pattern match automaton:
		const strus::TokenPatternMatchInstanceInterface* ptinst = pii->getTokenPatternMatchInstance();
		std::auto_ptr<strus::TokenPatternMatchContextInterface> ptctx( ptinst->createContext());
		std::vector<strus::stream::PatternMatchToken>::const_iterator
			ci = crmatches.begin(), ce = crmatches.end();
		for (; ci != ce; ++ci)
		{
			std::string tokstr( std::string( inputsrc.c_str() + ci->origpos(), ci->origsize()));
			std::cout << "token " << pii->tokenName(ci->id()) << "(" << ci->id() << ") at " << ci->ordpos() 
					<< "[" << ci->origpos() << ":" << ci->origsize() << "] '"
					<< tokstr << "'" << std::endl;
			ptctx->putInput( *ci);
		}
		std::vector<strus::stream::TokenPatternMatchResult> results = ptctx->fetchResults();
		if (results.size() == 0 && g_errorBuffer->hasError())
		{
			throw std::runtime_error( "failed to scan for patterns with token pattern match automaton");
		}

		// Print and verify result:
		std::vector<strus::stream::TokenPatternMatchResult>::const_iterator
			ri = results.begin(), re = results.end();
		for (; ri != re; ++ri)
		{
			std::cout << "result " << ri->name() << " at " << ri->ordpos() << ":" << std::endl;
			std::vector<strus::stream::TokenPatternMatchResultItem>::const_iterator
				ti = ri->items().begin(), te = ri->items().end();
			for (; ti != te; ++ti)
			{
				std::string itemstr( std::string( inputsrc.c_str() + ti->origpos(), ti->origsize()));
				std::cout << "\titem " << ti->name()
						<< " at " << ti->ordpos()
						<< " [" << ti->origpos() << ":" << ti->origsize() << "] "
						<< ti->weight() << " '" << itemstr.c_str() << "'" << std::endl;
			}
			std::cout << std::endl;
		}

		// Print and verify statistics:
		std::cerr << "Statistics:" << std::endl;
		strus::stream::TokenPatternMatchStatistics stats = ptctx->getStatistics();
		std::vector<strus::stream::TokenPatternMatchStatistics::Item>::const_iterator
			si = stats.items().begin(), se = stats.items().end();
		for (; si != se; ++si)
		{
			std::cerr << std::fixed << si->name() << " = " << si->value() << std::endl;
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



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
		else if (argc > 2)
		{
			std::cerr << "too many arguments" << std::endl;
			return 1;
		}
		else if (argc < 2)
		{
			std::cerr << "too few arguments" << std::endl;
			return 1;
		}
		std::auto_ptr<strus::TokenPatternMatchInterface> pti( strus::createTokenPatternMatch_standard( g_errorBuffer));
		if (!pti.get()) throw std::runtime_error("failed to create pattern matcher");
		std::auto_ptr<strus::CharRegexMatchInterface> cri( strus::createCharRegexMatch_standard( g_errorBuffer));
		if (!cri.get()) throw std::runtime_error("failed to create char regex matcher");
		std::auto_ptr<strus::PatternMatchProgramInterface> ppi( strus::createPatternMatchProgram_standard( pti.get(), cri.get(), g_errorBuffer));
		if (!ppi.get()) throw std::runtime_error("failed to create pattern program loader");
		std::auto_ptr<strus::PatternMatchProgramInstanceInterface> pii( ppi->createInstance());
		if (!pii.get()) throw std::runtime_error("failed to create pattern program loader instance");

		std::string programfile( argv[ 1]);
		std::string programsrc;
		unsigned int ec = strus::readFile( programfile, programsrc);
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
		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error( "error creating pattern match program interface");
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



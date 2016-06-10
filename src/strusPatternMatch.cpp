/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Program running pattern matching with a rule file on an input file
#include "strus/lib/error.hpp"
#include "strus/lib/stream.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/fileio.hpp"
#include "strus/versionBase.hpp"
#include "strus/versionAnalyzer.hpp"
#include "errorUtils.hpp"
#include "internationalization.hpp"
#include "utils.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <memory>
#include "hs_version.h"

#undef STRUS_LOWLEVEL_DEBUG

static void printIntelBsdLicense()
{
	std::cout << " Copyright (c) 2015, Intel Corporation" << std::endl;
	std::cout << std::endl;
	std::cout << " Redistribution and use in source and binary forms, with or without" << std::endl;
	std::cout << " modification, are permitted provided that the following conditions are met:" << std::endl;
	std::cout << std::endl;
	std::cout << "  * Redistributions of source code must retain the above copyright notice," << std::endl;
	std::cout << "    this list of conditions and the following disclaimer." << std::endl;
	std::cout << "  * Redistributions in binary form must reproduce the above copyright" << std::endl;
	std::cout << "    notice, this list of conditions and the following disclaimer in the" << std::endl;
	std::cout << "    documentation and/or other materials provided with the distribution." << std::endl;
	std::cout << "  * Neither the name of Intel Corporation nor the names of its contributors" << std::endl;
	std::cout << "    may be used to endorse or promote products derived from this software" << std::endl;
	std::cout << "    without specific prior written permission." << std::endl;
	std::cout << std::endl;
	std::cout << " THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"" << std::endl;
	std::cout << " AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE" << std::endl;
	std::cout << " IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE" << std::endl;
	std::cout << " ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE" << std::endl;
	std::cout << " LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR" << std::endl;
	std::cout << " CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF" << std::endl;
	std::cout << " SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS" << std::endl;
	std::cout << " INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN" << std::endl;
	std::cout << " CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)" << std::endl;
	std::cout << " ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE" << std::endl;
	std::cout << " POSSIBILITY OF SUCH DAMAGE." << std::endl;
}

static void printUsage()
{
	std::cout << "strusPatternMatch [options] <rulefile> <inputpath>" << std::endl;
	std::cout << "options:" << std::endl;
	std::cout << "-h|--help" << std::endl;
	std::cout << "    " << _TXT("Print this usage and do nothing else") << std::endl;
	std::cout << "-v|--version" << std::endl;
	std::cout << "    " << _TXT("Print the program version and do nothing else") << std::endl;
	std::cout << "--intel-bsd-license" << std::endl;
	std::cout << "    " << _TXT("Print the BSD license text of the Intel hyperscan library") << std::endl;
	std::cout << "-e|--ext <FILEEXT>" << std::endl;
	std::cout << "    " << _TXT("Do only process files with extension <FILEEXT>") << std::endl;
	std::cout << "<rulefile>   : " << _TXT("file with patterns to load") << std::endl;
	std::cout << "<inputfile>  : " << _TXT("input file or directory to process") << std::endl;
}

static strus::ErrorBufferInterface* g_errorBuffer = 0;	// error buffer

int main( int argc, const char* argv[])
{
	std::auto_ptr<strus::ErrorBufferInterface> errorBuffer( strus::createErrorBuffer_standard( 0, 2));
	if (!errorBuffer.get())
	{
		std::cerr << _TXT("failed to create error buffer") << std::endl;
		return -1;
	}
	g_errorBuffer = errorBuffer.get();

	try
	{
		bool doExit = false;
		int argi = 1;
		std::string fileext;

		// Parsing arguments:
		for (; argi < argc; ++argi)
		{
			if (0==std::strcmp( argv[argi], "-h") || 0==std::strcmp( argv[argi], "--help"))
			{
				printUsage();
				doExit = true;
			}
			else if (0==std::strcmp( argv[argi], "-v") || 0==std::strcmp( argv[argi], "--version"))
			{
				std::cerr << "strus analyzer version " << STRUS_ANALYZER_VERSION_STRING << std::endl;
				std::cerr << "strus base version " << STRUS_BASE_VERSION_STRING << std::endl;
				std::cerr << std::endl;
				std::cerr << "hyperscan version " << HS_VERSION_STRING << std::endl;
				std::cerr << "\tCopyright (c) 2015, Intel Corporation" << std::endl;
				std::cerr << "\tCall this program with --intel-bsd-license" << std::endl;
				std::cerr << "\tto print the the Intel hyperscan library license text." << std::endl; 
				doExit = true;
			}
			else if (0==std::strcmp( argv[argi], "--intel-bsd-license"))
			{
				std::cerr << "Intel hyperscan library license:" << std::endl;
				printIntelBsdLicense();
				std::cerr << std::endl;
				doExit = true;
			}
			else if (0==std::strcmp( argv[argi], "-e") || 0==std::strcmp( argv[argi], "--ext"))
			{
				if (argi == argc || argv[argi+1][0] == '-')
				{
					throw strus::runtime_error( _TXT("no argument given to option --ext"));
				}
				++argi;
				fileext = argv[argi];
			}
			else if (argv[argi][0] == '-')
			{
				throw strus::runtime_error(_TXT("unknown option %s"), argv[ argi]);
			}
			else
			{
				break;
			}
		}
		if (doExit) return 0;
		if (argc - argi < 2)
		{
			printUsage();
			throw strus::runtime_error( _TXT("too few arguments (given %u, required %u)"), argc - argi, 2);
		}
		if (argc - argi > 2)
		{
			printUsage();
			throw strus::runtime_error( _TXT("too many arguments (given %u, required %u)"), argc - argi, 2);
		}

		std::string rulefile( argv[ argi+0]);
		std::string inputpath( argv[ argi+1]);

		if (g_errorBuffer->hasError())
		{
			throw strus::runtime_error(_TXT("error in initialization"));
		}

		// Check for reported error an terminate regularly:
		if (g_errorBuffer->hasError())
		{
			throw strus::runtime_error( _TXT("error processing resize blocks"));
		}
		std::cerr << _TXT("OK") << std::endl;
		return 0;
	}
	catch (const std::exception& e)
	{
		const char* errormsg = g_errorBuffer?g_errorBuffer->fetchError():0;
		if (errormsg)
		{
			std::cerr << e.what() << ": " << errormsg << std::endl;
		}
		else
		{
			std::cerr << e.what() << std::endl;
		}
	}
	return -1;
}


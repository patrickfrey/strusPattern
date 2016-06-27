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
#include "strus/lib/segmenter_cjson.hpp"
#include "strus/lib/segmenter_textwolf.hpp"
#include "strus/lib/detector_std.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/fileio.hpp"
#include "strus/versionBase.hpp"
#include "strus/versionAnalyzer.hpp"
#include "strus/patternMatchProgramInstanceInterface.hpp"
#include "strus/patternMatchProgramInterface.hpp"
#include "strus/stream/patternMatchToken.hpp"
#include "strus/tokenPatternMatchInstanceInterface.hpp"
#include "strus/tokenPatternMatchContextInterface.hpp"
#include "strus/tokenPatternMatchInterface.hpp"
#include "strus/charRegexMatchInstanceInterface.hpp"
#include "strus/charRegexMatchContextInterface.hpp"
#include "strus/charRegexMatchInterface.hpp"
#include "strus/segmenterContextInterface.hpp"
#include "strus/segmenterInstanceInterface.hpp"
#include "strus/segmenterInterface.hpp"
#include "strus/documentClassDetectorInterface.hpp"
#include "strus/documentClass.hpp"
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
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

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
	std::cout << "strusPatternMatch [options] <inputpath>" << std::endl;
	std::cout << "options:" << std::endl;
	std::cout << "-h|--help" << std::endl;
	std::cout << "    " << _TXT("Print this usage and do nothing else") << std::endl;
	std::cout << "-v|--version" << std::endl;
	std::cout << "    " << _TXT("Print the program version and do nothing else") << std::endl;
	std::cout << "--intel-bsd-license" << std::endl;
	std::cout << "    " << _TXT("Print the BSD license text of the Intel hyperscan library") << std::endl;
	std::cout << "-t|--threads <N>" << std::endl;
	std::cout << "    " << _TXT("Set <N> as number of inserter threads to use") << std::endl;
	std::cout << "-X|--ext <FILEEXT>" << std::endl;
	std::cout << "    " << _TXT("Do only process files with extension <FILEEXT>") << std::endl;
	std::cout << "-Y|--mimetype <TYPE>" << std::endl;
	std::cout << "    " << _TXT("Specify the MIME type of all files to process as <TYPE>") << std::endl;
	std::cout << "-C|--encoding <ENC>" << std::endl;
	std::cout << "    " << _TXT("Specify the character set encoding of all files to process as <ENC>") << std::endl;
	std::cout << "-e|--expression <EXP>" << std::endl;
	std::cout << "    " << _TXT("Define a selection expression <EXP> for the content to process") << std::endl;
	std::cout << "-p|--program <PRG>" << std::endl;
	std::cout << "    " << _TXT("Load program <PRG> with patterns to process") << std::endl;
	std::cout << "<inputfile>  : " << _TXT("input file or directory to process") << std::endl;
}

static strus::ErrorBufferInterface* g_errorBuffer = 0;	// error buffer

unsigned int getUintValue( const char* arg)
{
	unsigned int rt = 0, prev = 0;
	char const* cc = arg;
	for (; *cc; ++cc)
	{
		if (*cc < '0' || *cc > '9') throw std::runtime_error( std::string( "parameter is not a non negative integer number: ") + arg);
		rt = (rt * 10) + (*cc - '0');
		if (rt < prev) throw std::runtime_error( std::string( "parameter out of range: ") + arg);
	}
	return rt;
}

static void loadFileNames( std::vector<std::string>& result, const std::string& path, const std::string& fileext)
{
	if (strus::isDir( path))
	{
		unsigned int ec = strus::readDirFiles( path, fileext, result);
		if (ec)
		{
			throw strus::runtime_error( _TXT( "could not read directory to process '%s' (errno %u)"), path.c_str(), ec);
		}
		std::vector<std::string> subdirs;
		ec = strus::readDirSubDirs( path, subdirs);
		if (ec)
		{
			throw strus::runtime_error( _TXT( "could not read subdirectories to process '%s' (errno %u)"), path.c_str(), ec);
		}
		std::vector<std::string>::const_iterator si = subdirs.begin(), se = subdirs.end();
		for (; si != se; ++si)
		{
			loadFileNames( result, *si, fileext);
		}
	}
	else
	{
		result.push_back( path);
	}
}

class ThreadContext;

class GlobalContext
{
public:
	explicit GlobalContext(
			const strus::TokenPatternMatchInstanceInterface* ptinst_,
			const strus::CharRegexMatchInstanceInterface* crinst_,
			const std::vector<std::string>& selectexpr,
			const std::string& path,
			const std::string& fileext,
			const std::string& mimetype,
			const std::string& encoding)
		:m_ptinst(ptinst_)
		,m_crinst(crinst_)
		,m_segmenter(0)
	{
		loadFileNames( m_files, path, fileext);
		m_fileitr = m_files.begin();

		if (strus::utils::caseInsensitiveEquals( mimetype, "xml")
		||  strus::utils::caseInsensitiveEquals( mimetype, "application/xml"))
		{
			m_documentClass.setMimeType( "application/xml");
			if (encoding.empty())
			{
				m_documentClass.setEncoding( encoding);
			}
			else
			{
				m_documentClass.setEncoding( "UTF-8");
			}
			std::auto_ptr<strus::SegmenterInterface> segmenter( strus::createSegmenter_textwolf( g_errorBuffer));
			if (!segmenter.get())
			{
				throw strus::runtime_error(_TXT("failed to create document segmenter"));
			}
			m_segmenter_textwolf.reset( segmenter->createInstance());
			if (!m_segmenter_textwolf.get())
			{
				throw strus::runtime_error(_TXT("failed to create document segmenter instance"));
			}
			m_segmenter = m_segmenter_textwolf.get();
		}
		else if (strus::utils::caseInsensitiveEquals( mimetype, "json")
			|| strus::utils::caseInsensitiveEquals( mimetype, "application/json"))
		{
			m_documentClass.setMimeType( "application/json");
			if (encoding.empty())
			{
				m_documentClass.setEncoding( encoding);
			}
			else
			{
				m_documentClass.setEncoding( "UTF-8");
			}
			std::auto_ptr<strus::SegmenterInterface> segmenter( strus::createSegmenter_cjson( g_errorBuffer));
			if (!segmenter.get())
			{
				throw strus::runtime_error(_TXT("failed to create document segmenter"));
			}
			m_segmenter_cjson.reset( segmenter->createInstance());
			if (!m_segmenter_cjson.get())
			{
				throw strus::runtime_error(_TXT("failed to create document segmenter instance"));
			}
			m_segmenter = m_segmenter_cjson.get();
		}
		else if (mimetype.empty())
		{
			m_documentClassDetector.reset( strus::createDetector_std( g_errorBuffer));
			if (!m_documentClassDetector.get())
			{
				throw strus::runtime_error(_TXT("failed to create document class detector"));
			}
			std::auto_ptr<strus::SegmenterInterface> segmenter_textwolf( strus::createSegmenter_textwolf( g_errorBuffer));
			if (!segmenter_textwolf.get())
			{
				throw strus::runtime_error(_TXT("failed to create document segmenter"));
			}
			m_segmenter_textwolf.reset( segmenter_textwolf->createInstance());
			if (!m_segmenter_textwolf.get())
			{
				throw strus::runtime_error(_TXT("failed to create document segmenter instance"));
			}
			std::auto_ptr<strus::SegmenterInterface> segmenter_cjson( strus::createSegmenter_cjson( g_errorBuffer));
			if (!segmenter_cjson.get())
			{
				throw strus::runtime_error(_TXT("failed to create document segmenter"));
			}
			m_segmenter_cjson.reset( segmenter_cjson->createInstance());
			if (!m_segmenter_cjson.get())
			{
				throw strus::runtime_error(_TXT("failed to create document segmenter instance"));
			}
		}
		else
		{
			throw strus::runtime_error(_TXT("unknown document type specified: '%s'"));
		}
		std::vector<std::string>::const_iterator ei = selectexpr.begin(), ee = selectexpr.end();
		for (int eidx=1; ei != ee; ++ei,++eidx)
		{
			if (m_segmenter_cjson.get()) m_segmenter_cjson->defineSelectorExpression( eidx, *ei);
			if (m_segmenter_textwolf.get()) m_segmenter_textwolf->defineSelectorExpression( eidx, *ei);
		}
		if (g_errorBuffer->hasError())
		{
			throw strus::runtime_error(_TXT("global context initialization failed"));
		}
	}

	strus::SegmenterContextInterface* createSegmenterContext( const std::string& content) const
	{
		if (m_segmenter)
		{
			return m_segmenter->createContext( m_documentClass);
		}
		else
		{
			strus::DocumentClass documentClass;
			if (!m_documentClassDetector->detect( documentClass, content.c_str(), content.size()))
			{
				throw strus::runtime_error(_TXT("failed to detect document class"));
			}
			if (documentClass.mimeType() == "application/xml")
			{
				return m_segmenter_textwolf->createContext( documentClass);
			}
			else if (documentClass.mimeType() == "application/json")
			{
				return m_segmenter_cjson->createContext( documentClass);
			}
			else
			{
				throw strus::runtime_error(_TXT("cannot process document class '%s"), documentClass.mimeType().c_str());
			}
		}
	}

	const strus::TokenPatternMatchInstanceInterface* tokenPatternMatchInstance() const	{return m_ptinst;}
	const strus::CharRegexMatchInstanceInterface* charRegexMatchInstance() const		{return m_crinst;}

	void fetchError()
	{
		boost::mutex::scoped_lock lock( m_mutex);
		if (g_errorBuffer->hasError())
		{
			m_errors.push_back( g_errorBuffer->fetchError());
		}
	}

	std::string fetchFile()
	{
		boost::mutex::scoped_lock lock( m_mutex);
		if (m_fileitr != m_files.end())
		{
			return *m_fileitr++;
		}
		return std::string();
	}

	const std::vector<std::string>& errors() const
	{
		return m_errors;
	}

public:
	ThreadContext* createThreadContext() const;

private:
	boost::mutex m_mutex;
	const strus::TokenPatternMatchInstanceInterface* m_ptinst;
	const strus::CharRegexMatchInstanceInterface* m_crinst;
	strus::SegmenterInstanceInterface* m_segmenter;
	std::auto_ptr<strus::SegmenterInstanceInterface> m_segmenter_textwolf;
	std::auto_ptr<strus::SegmenterInstanceInterface> m_segmenter_cjson;
	std::auto_ptr<strus::DocumentClassDetectorInterface> m_documentClassDetector;
	
	strus::DocumentClass m_documentClass;
	std::vector<std::string> m_errors;
	std::vector<std::string> m_files;
	std::vector<std::string>::const_iterator m_fileitr;
};

class ThreadContext
{
public:
	explicit ThreadContext( GlobalContext* globalContext_)
		:m_globalContext(globalContext_){}

	void processDocument( const std::string& filename)
	{
		std::string content;
		std::string segmented;

		unsigned int ec;
		if (filename == "-")
		{
			ec = strus::readStdin( content);
		}
		else
		{
			ec = strus::readFile( filename, content);
		}
		if (ec)
		{
			throw strus::runtime_error(_TXT("error (%u) reading rule file: %s"), ec, ::strerror(ec));
		}
		std::auto_ptr<strus::TokenPatternMatchContextInterface> mt( m_globalContext->tokenPatternMatchInstance()->createContext());
		std::auto_ptr<strus::CharRegexMatchContextInterface> crctx( m_globalContext->charRegexMatchInstance()->createContext());
		std::auto_ptr<strus::SegmenterContextInterface> segmenter( m_globalContext->createSegmenterContext( content));

		segmenter->putInput( content.c_str(), content.size(), true);
		int id;
		strus::SegmenterPosition segmentpos;
		const char* segment;
		std::size_t segmentsize;
		while (segmenter->getNext( id, segmentpos, segment, segmentsize))
		{
			if ((std::size_t)segmentpos > segmented.size())
			{
				segmented.resize( segmentpos, ' ');
				segmented.append( segment, segmentsize);
			}
			else if (segmentpos + segmentsize > segmented.size())
			{
				std::size_t part = segmented.size() - segmentpos;
				segmented.append( segment + part, segmentsize - part);
			}
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "processing segment " << id << " [" << std::string(segment,segmentsize) << "] at " << segmentpos << std::endl;
#endif
			std::vector<strus::stream::PatternMatchToken> crmatches = crctx->match( segment, segmentsize);
			if (crmatches.size() == 0 && g_errorBuffer->hasError())
			{
				throw std::runtime_error( "failed to scan for tokens with char regex match automaton");
			}
			std::vector<strus::stream::PatternMatchToken>::const_iterator ti = crmatches.begin(), te = crmatches.end();
			for (; ti != te; ++ti)
			{
				strus::stream::PatternMatchToken tok( strus::stream::PatternMatchToken( ti->id(), ti->ordpos(), ti->origpos()+segmentpos, ti->origsize()));
				mt->putInput( tok);
			}
		}
		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error("error matching rules");
		}
		std::vector<strus::stream::TokenPatternMatchResult> result = mt->fetchResults();
		output( filename, result, segmented.c_str());
	}

	void printResults( std::ostream& out, const std::vector<strus::stream::TokenPatternMatchResult>& results, const char* src)
	{
		std::vector<strus::stream::TokenPatternMatchResult>::const_iterator
			ri = results.begin(), re = results.end();
		for (; ri != re; ++ri)
		{
			out << ri->name() << " [" << ri->ordpos() << ", " << ri->origpos() << "]:";
			std::vector<strus::stream::TokenPatternMatchResultItem>::const_iterator
				ei = ri->items().begin(), ee = ri->items().end();
		
			for (; ei != ee; ++ei)
			{
				out << " " << ei->name() << " [" << ei->ordpos()
						<< ", " << ei->origpos() << ", " << ei->origsize() << "]";
				if (src)
				{
					out << " '" << std::string( src+ei->origpos(), ei->origsize()) << "'";
				}
			}
			out << std::endl;
		}
	}

	void output( const std::string& filename, const std::vector<strus::stream::TokenPatternMatchResult>& results, const char* src)
	{
		std::cout << filename << ":" << std::endl;
		printResults( std::cout, results, src);
	}

	void run()
	{
		try
		{
		for (;;)
		{
			std::string filename = m_globalContext->fetchFile();
			if (filename.empty()) break;

#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << _TXT("processing file '") << filename << "'" << std::endl;
#endif
			processDocument( filename);
			if (g_errorBuffer->hasError())
			{
				std::cerr << _TXT( "got error, terminating thread ...");
				break;
			}
		}
		}
		catch (const std::runtime_error& err)
		{
			g_errorBuffer->report( "error processing documents: %s", err.what());
		}
		catch (const std::bad_alloc&)
		{
			g_errorBuffer->report( "out of memory processing documents");
		}
		m_globalContext->fetchError();
	}

private:
	GlobalContext* m_globalContext;
};


int main( int argc, const char* argv[])
{
	std::auto_ptr<strus::ErrorBufferInterface> errorBuffer;
	try
	{
		bool doExit = false;
		int argi = 1;
		std::string fileext;
		std::string mimetype;
		std::string encoding;
		std::vector<std::string> programfiles;
		std::vector<std::string> selectexpr;
		unsigned int nofThreads = 0;

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
				std::cerr << _TXT("strus analyzer version ") << STRUS_ANALYZER_VERSION_STRING << std::endl;
				std::cerr << _TXT("strus base version ") << STRUS_BASE_VERSION_STRING << std::endl;
				std::cerr << std::endl;
				std::cerr << _TXT("hyperscan version ") << HS_VERSION_STRING << std::endl;
				std::cerr << "\tCopyright (c) 2015, Intel Corporation" << std::endl;
				std::cerr << "\tCall this program with --intel-bsd-license" << std::endl;
				std::cerr << "\tto print the the Intel hyperscan library license text." << std::endl; 
				doExit = true;
			}
			else if (0==std::strcmp( argv[argi], "--intel-bsd-license"))
			{
				std::cerr << _TXT("Intel hyperscan library license:") << std::endl;
				printIntelBsdLicense();
				std::cerr << std::endl;
				doExit = true;
			}
			else if (0==std::strcmp( argv[argi], "-X") || 0==std::strcmp( argv[argi], "--ext"))
			{
				if (argi == argc || argv[argi+1][0] == '-')
				{
					throw strus::runtime_error( _TXT("no argument given to option --ext"));
				}
				++argi;
				if (!fileext.empty())
				{
					throw strus::runtime_error( _TXT("file extension option --ext specified twice"));
				}
				fileext = argv[argi];
				if (fileext.empty())
				{
					throw strus::runtime_error( _TXT("file extension option --ext argument is empty"));
				}
			}
			else if (0==std::strcmp( argv[argi], "-Y") || 0==std::strcmp( argv[argi], "--mimetype")  || 0==std::strcmp( argv[argi], "--content-type"))
			{
				if (argi == argc || argv[argi+1][0] == '-')
				{
					throw strus::runtime_error( _TXT("no argument given to option --content-type"));
				}
				++argi;
				if (!mimetype.empty())
				{
					throw strus::runtime_error( _TXT("mime type option --mimetype specified twice"));
				}
				mimetype = argv[argi];
				if (mimetype.empty())
				{
					throw strus::runtime_error( _TXT("mime type option --mimetype argument is empty"));
				}
			}
			else if (0==std::strcmp( argv[argi], "-C") || 0==std::strcmp( argv[argi], "--encoding")  || 0==std::strcmp( argv[argi], "--content-type"))
			{
				if (argi == argc || argv[argi+1][0] == '-')
				{
					throw strus::runtime_error( _TXT("no argument given to option --content-type"));
				}
				++argi;
				if (!encoding.empty())
				{
					throw strus::runtime_error( _TXT("mime type option --encoding specified twice"));
				}
				encoding = argv[argi];
				if (encoding.empty())
				{
					throw strus::runtime_error( _TXT("mime type option --encoding argument is empty"));
				}
			}
			else if (0==std::strcmp( argv[argi], "-e") || 0==std::strcmp( argv[argi], "--expression"))
			{
				if (argi == argc || argv[argi+1][0] == '-')
				{
					throw strus::runtime_error( _TXT("no argument given to option --expression"));
				}
				++argi;
				selectexpr.push_back( argv[argi]);
			}
			else if (0==std::strcmp( argv[argi], "-p") || 0==std::strcmp( argv[argi], "--program"))
			{
				if (argi == argc || argv[argi+1][0] == '-')
				{
					throw strus::runtime_error( _TXT("no argument given to option --program"));
				}
				++argi;
				programfiles.push_back( argv[argi]);
			}
			else if (0==std::strcmp( argv[argi], "-t") || 0==std::strcmp( argv[argi], "--threads"))
			{
				if (argi == argc || argv[argi+1][0] == '-')
				{
					throw strus::runtime_error( _TXT("no argument given to option --threads"));
				}
				++argi;
				if (!nofThreads)
				{
					throw strus::runtime_error( _TXT("number of threads option --threads specified twice"));
				}
				nofThreads = getUintValue( argv[argi]);
				if (!nofThreads)
				{
					throw strus::runtime_error( _TXT("number of threafs option --threads is 0"));
				}
			}
			else if (argv[argi][0] == '-' && argv[argi][1] == '-' && !argv[argi][2])
			{
				break;
			}
			else if (argv[argi][0] == '-' && !argv[argi][1])
			{
				break;
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
		if (argc - argi < 1)
		{
			printUsage();
			throw strus::runtime_error( _TXT("too few arguments (given %u, required %u)"), argc - argi, 2);
		}
		if (argc - argi > 1)
		{
			printUsage();
			throw strus::runtime_error( _TXT("too many arguments (given %u, required %u)"), argc - argi, 2);
		}
		errorBuffer.reset( strus::createErrorBuffer_standard( 0, nofThreads+1));
		if (!errorBuffer.get())
		{
			throw strus::runtime_error( _TXT("failed to create error buffer"));
		}
		g_errorBuffer = errorBuffer.get();

		// Create objects:
		std::auto_ptr<strus::TokenPatternMatchInterface> pti( strus::createTokenPatternMatch_standard( g_errorBuffer));
		if (!pti.get()) throw std::runtime_error("failed to create pattern matcher");
		std::auto_ptr<strus::CharRegexMatchInterface> cri( strus::createCharRegexMatch_standard( g_errorBuffer));
		if (!cri.get()) throw std::runtime_error("failed to create char regex matcher");
		std::auto_ptr<strus::PatternMatchProgramInterface> ppi( strus::createPatternMatchProgram_standard( pti.get(), cri.get(), g_errorBuffer));
		if (!ppi.get()) throw std::runtime_error("failed to create pattern program loader");
		std::auto_ptr<strus::PatternMatchProgramInstanceInterface> pii( ppi->createInstance());
		if (!pii.get()) throw std::runtime_error("failed to create pattern program loader instance");

		std::cerr << "loading programs ..." << std::endl;
		std::vector<std::string>::const_iterator pi = programfiles.begin(), pe = programfiles.end();
		for (; pi != pe; ++pi)
		{
			std::string programsrc;
			unsigned int ec = strus::readFile( *pi, programsrc);
			if (ec)
			{
				throw strus::runtime_error(_TXT("error (%u) reading rule file: %s"), ec, ::strerror(ec));
			}
			if (!pii->load( programsrc))
			{
				throw std::runtime_error( "error loading pattern match program source");
			}
		}
		if (!pii->compile())
		{
			throw std::runtime_error( "error compiling pattern match programs");
		}
		
		if (g_errorBuffer->hasError())
		{
			throw strus::runtime_error(_TXT("error in initialization"));
		}
		std::string inputpath( argv[ argi]);
		if (selectexpr.empty())
		{
			selectexpr.push_back( "//()");
		}
		GlobalContext globalContext(
				pii->getTokenPatternMatchInstance(),
				pii->getCharRegexMatchInstance(),
				selectexpr, inputpath, fileext, mimetype, encoding);

		std::cerr << "start matching ..." << std::endl;
		if (nofThreads)
		{
			fprintf( stderr, _TXT("starting %u threads for evaluation ...\n"), nofThreads);

			std::vector<ThreadContext> ctxar;
			for (unsigned int ti=0; ti<nofThreads; ++ti)
			{
				ctxar.push_back( ThreadContext( &globalContext));
			}
			{
				boost::thread_group tgroup;
				for (unsigned int ti=0; ti<nofThreads; ++ti)
				{
					tgroup.create_thread( boost::bind( &ThreadContext::run, &ctxar[ti]));
				}
				tgroup.join_all();
			}
		}
		else
		{
			ThreadContext ctx( &globalContext);
			ctx.run();
		}
		if (!globalContext.errors().empty())
		{
			std::vector<std::string>::const_iterator 
				ei = globalContext.errors().begin(), ee = globalContext.errors().end();
			for (; ei != ee; ++ei)
			{
				std::cerr << _TXT("error in thread: ") << *ei << std::endl;
			}
		}

		// Check for reported error an terminate regularly:
		if (g_errorBuffer->hasError())
		{
			throw strus::runtime_error( _TXT("error processing resize blocks"));
		}
		std::cerr << _TXT("OK done") << std::endl;
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


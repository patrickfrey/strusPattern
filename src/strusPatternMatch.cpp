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
#include "strus/tokenMarkupInstanceInterface.hpp"
#include "strus/tokenMarkupContextInterface.hpp"
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
#include <map>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <limits>
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
	std::cout << "-K|--tokens" << std::endl;
	std::cout << "    " << _TXT("Print the tokenization used for pattern matching too") << std::endl;
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
	std::cout << "    " << _TXT("  (default if nothing specified is \"//()\"") << std::endl;
	std::cout << "-H|--markup <NAME>" << std::endl;
	std::cout << "    " << _TXT("Output the content with markups of the rules or variables with name <NAME>") << std::endl;
	std::cout << "-Z|--marker <MRK>" << std::endl;
	std::cout << "    " << _TXT("Define a character sequence inserted before every result declaration") << std::endl;
	std::cout << "-p|--program <PRG>" << std::endl;
	std::cout << "    " << _TXT("Load program <PRG> with patterns to process") << std::endl;
	std::cout << "-o|--output <FILE>" << std::endl;
	std::cout << "    " << _TXT("Write output to file (thread id is inserted before '.', if threads specified)") << std::endl;
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
		std::vector<std::string> filenames;
		unsigned int ec = strus::readDirFiles( path, fileext, filenames);
		if (ec)
		{
			throw strus::runtime_error( _TXT( "could not read directory to process '%s' (errno %u)"), path.c_str(), ec);
		}
		std::vector<std::string>::const_iterator fi = filenames.begin(), fe = filenames.end();
		for (; fi != fe; ++fi)
		{
			result.push_back( path + strus::dirSeparator() + *fi);
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
			loadFileNames( result, path + strus::dirSeparator() + *si, fileext);
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
			const strus::PatternMatchProgramInstanceInterface* program_,
			const strus::TokenPatternMatchInstanceInterface* ptinst_,
			const strus::CharRegexMatchInstanceInterface* crinst_,
			const std::vector<std::string>& selectexpr,
			const std::string& path,
			const std::string& fileext,
			const std::string& mimetype,
			const std::string& encoding,
			const std::map<std::string,int>& markups_,
			const std::string& resultMarker_,
			bool printTokens_)
		:m_program(program_)
		,m_ptinst(ptinst_)
		,m_crinst(crinst_)
		,m_segmenter(0)
		,m_markups(markups_)
		,m_resultMarker(resultMarker_)
		,m_printTokens(printTokens_)
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
		m_tokenMarkup.reset( strus::createTokenMarkupInstance_standard( g_errorBuffer));
		if (g_errorBuffer->hasError())
		{
			throw strus::runtime_error(_TXT("global context initialization failed"));
		}
	}

	strus::SegmenterContextInterface* createSegmenterContext( const std::string& content, strus::SegmenterInstanceInterface*& segmenter, strus::DocumentClass& documentClass) const
	{
		if (m_segmenter)
		{
			segmenter = m_segmenter;
			documentClass = m_documentClass;
			return m_segmenter->createContext( m_documentClass);
		}
		else
		{
			if (!m_documentClassDetector->detect( documentClass, content.c_str(), content.size()))
			{
				throw strus::runtime_error(_TXT("failed to detect document class"));
			}
			if (documentClass.mimeType() == "application/xml")
			{
				segmenter = m_segmenter_textwolf.get();
				return m_segmenter_textwolf->createContext( documentClass);
			}
			else if (documentClass.mimeType() == "application/json")
			{
				segmenter = m_segmenter_cjson.get();
				return m_segmenter_cjson->createContext( documentClass);
			}
			else
			{
				throw strus::runtime_error(_TXT("cannot process document class '%s"), documentClass.mimeType().c_str());
			}
		}
	}

	strus::TokenMarkupContextInterface* createTokenMarkupContext() const
	{
		strus::TokenMarkupContextInterface* rt = m_tokenMarkup->createContext();
		if (!rt)
		{
			throw strus::runtime_error(_TXT("failed to create token markup context"));
		}
		return rt;
	}

	const strus::TokenPatternMatchInstanceInterface* tokenPatternMatchInstance() const	{return m_ptinst;}
	const strus::CharRegexMatchInstanceInterface* charRegexMatchInstance() const		{return m_crinst;}

	bool printTokens() const								{return m_printTokens;}
	const char* tokenName( unsigned int id) const						{return m_program->tokenName( id);}

	void fetchError()
	{
		if (g_errorBuffer->hasError())
		{
			boost::mutex::scoped_lock lock( m_mutex);
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

	const std::map<std::string,int>& markups() const
	{
		return m_markups;
	}
	const std::string& resultMarker() const
	{
		return m_resultMarker;
	}

public:
	ThreadContext* createThreadContext() const;

private:
	boost::mutex m_mutex;
	const strus::PatternMatchProgramInstanceInterface* m_program;
	const strus::TokenPatternMatchInstanceInterface* m_ptinst;
	const strus::CharRegexMatchInstanceInterface* m_crinst;
	strus::SegmenterInstanceInterface* m_segmenter;
	std::auto_ptr<strus::SegmenterInstanceInterface> m_segmenter_textwolf;
	std::auto_ptr<strus::SegmenterInstanceInterface> m_segmenter_cjson;
	std::auto_ptr<strus::DocumentClassDetectorInterface> m_documentClassDetector;
	std::auto_ptr<strus::TokenMarkupInstanceInterface> m_tokenMarkup;
	std::map<std::string,int> m_markups;
	std::string m_resultMarker;
	strus::DocumentClass m_documentClass;
	std::vector<std::string> m_errors;
	std::vector<std::string> m_files;
	std::vector<std::string>::const_iterator m_fileitr;
	bool m_printTokens;
};

class ThreadContext
{
public:
	~ThreadContext(){}

	ThreadContext( const ThreadContext& o)
		:m_globalContext(o.m_globalContext),m_threadid(o.m_threadid),m_outputfile(o.m_outputfile),m_outputfilestream(o.m_outputfilestream),m_output(o.m_output)
	{}

	ThreadContext( GlobalContext* globalContext_, unsigned int threadid_, const std::string& outputfile_="")
		:m_globalContext(globalContext_),m_threadid(threadid_),m_outputfilestream(),m_output(0)
	{
		if (outputfile_.empty())
		{
			m_output = &std::cout;
		}
		else
		{
			if (m_threadid > 0)
			{
				std::ostringstream namebuf;
				char const* substpos = std::strchr( outputfile_.c_str(), '.');
				if (substpos)
				{
					namebuf << std::string( outputfile_.c_str(), substpos-outputfile_.c_str())
						<< m_threadid << substpos;
				}
				else
				{
					namebuf << outputfile_ << m_threadid;
				}
				m_outputfile = namebuf.str();
			}
			else
			{
				m_outputfile = outputfile_;
			}
			try
			{
				m_outputfilestream.reset( new std::ofstream( m_outputfile.c_str()));
			}
			catch (const std::exception& err)
			{
				throw strus::runtime_error(_TXT("failed to open file '%s' for output: %s"), m_outputfile.c_str(), err.what());
			}
			m_output = m_outputfilestream.get();
		}
	}

	struct PositionInfo
	{
		strus::SegmenterPosition segpos;
		std::size_t srcpos;

		PositionInfo( strus::SegmenterPosition segpos_, std::size_t srcpos_)
			:segpos(segpos_),srcpos(srcpos_){}
		PositionInfo( const PositionInfo& o)
			:segpos(o.segpos),srcpos(o.srcpos){}
	};

	void processDocument( const std::string& filename)
	{
		std::string content;

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
			throw strus::runtime_error(_TXT("error (%u) reading document %s: %s"), ec, filename.c_str(), ::strerror(ec));
		}
		std::auto_ptr<strus::TokenPatternMatchContextInterface> mt( m_globalContext->tokenPatternMatchInstance()->createContext());
		std::auto_ptr<strus::CharRegexMatchContextInterface> crctx( m_globalContext->charRegexMatchInstance()->createContext());
		strus::SegmenterInstanceInterface* segmenterInstance;
		strus::DocumentClass documentClass;
		std::auto_ptr<strus::SegmenterContextInterface> segmenter( m_globalContext->createSegmenterContext( content, segmenterInstance, documentClass));

		segmenter->putInput( content.c_str(), content.size(), true);
		int id;
		strus::SegmenterPosition segmentpos;
		const char* segment;
		std::size_t segmentsize;
		std::vector<PositionInfo> segmentposmap;
		std::string source;
		std::size_t segmentidx;
		unsigned int ordposOffset = 0;
		strus::SegmenterPosition prev_segmentpos = (strus::SegmenterPosition)std::numeric_limits<std::size_t>::max();
		while (segmenter->getNext( id, segmentpos, segment, segmentsize))
		{
			segmentidx = segmentposmap.size();
			if (prev_segmentpos != segmentpos)
			{
				segmentposmap.push_back( PositionInfo( segmentpos, source.size()));
				source.append( segment, segmentsize);
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "processing segment " << id << " [" << std::string(segment,segmentsize) << "] at " << segmentpos << std::endl;
#endif
				std::vector<strus::stream::PatternMatchToken> crmatches = crctx->match( segment, segmentsize);
				if (crmatches.size() == 0 && g_errorBuffer->hasError())
				{
					throw std::runtime_error( "failed to scan for tokens with char regex match automaton");
				}
				std::vector<strus::stream::PatternMatchToken>::iterator ti = crmatches.begin(), te = crmatches.end();
				for (; ti != te; ++ti)
				{
					ti->setOrigseg( segmentidx);
					ti->setOrdpos( ti->ordpos() + ordposOffset);
					if (m_globalContext->printTokens())
					{
						std::cout << ti->ordpos() << ": " << m_globalContext->tokenName(ti->id()) << " " << std::string( segment+ti->origpos(), ti->origsize()) << std::endl;
					}
					mt->putInput( *ti);
				}
				if (crmatches.size() > 0)
				{
					ordposOffset = crmatches.back().ordpos();
				}
			}
		}
		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error("error matching rules");
		}
		std::cout << m_globalContext->resultMarker() << filename << ":" << std::endl;
		std::vector<strus::stream::TokenPatternMatchResult> results = mt->fetchResults();
		if (m_globalContext->markups().empty())
		{
			(*m_output) << "# " << filename << ":" << std::endl;
			printResults( *m_output, segmentposmap, results, source);
		}
		else
		{
			markupResults( *m_output, results, documentClass, content, segmenterInstance);
		}
	}

	void printResults( std::ostream& out, const std::vector<PositionInfo>& segmentposmap, const std::vector<strus::stream::TokenPatternMatchResult>& results, const std::string& src)
	{
		std::vector<strus::stream::TokenPatternMatchResult>::const_iterator
			ri = results.begin(), re = results.end();
		for (; ri != re; ++ri)
		{
			std::size_t start_segpos = segmentposmap[ ri->start_origseg()].segpos;
			std::size_t end_segpos = segmentposmap[ ri->end_origseg()].segpos;
			out << ri->name() << " [" << ri->ordpos() << ", " << start_segpos << "|" << ri->start_origpos() << " .. " << end_segpos << "|" << ri->end_origpos() << "]:";
			std::vector<strus::stream::TokenPatternMatchResultItem>::const_iterator
				ei = ri->items().begin(), ee = ri->items().end();

			for (; ei != ee; ++ei)
			{
				start_segpos = segmentposmap[ ei->start_origseg()].segpos;
				end_segpos = segmentposmap[ ei->end_origseg()].segpos;
				out << " " << ei->name() << " [" << ei->ordpos()
						<< ", " << start_segpos << "|" << ei->start_origpos() << " .. " << end_segpos << "|" << ei->end_origpos() << "]";
				std::size_t start_srcpos = segmentposmap[ ei->start_origseg()].srcpos + ei->start_origpos();
				std::size_t end_srcpos = segmentposmap[ ei->start_origseg()].srcpos + ei->end_origpos();
				out << " '" << std::string( src.c_str() + start_srcpos, end_srcpos - start_srcpos) << "'";
			}
			out << std::endl;
		}
	}

	void markupResults( std::ostream& out,
				const std::vector<strus::stream::TokenPatternMatchResult>& results,
				const strus::DocumentClass& documentClass, const std::string& src,
				const strus::SegmenterInstanceInterface* segmenterInstance)
	{
		std::auto_ptr<strus::TokenMarkupContextInterface> markupContext( m_globalContext->createTokenMarkupContext());

		std::vector<strus::stream::TokenPatternMatchResult>::const_iterator
			ri = results.begin(), re = results.end();
		for (; ri != re; ++ri)
		{
			std::map<std::string,int>::const_iterator mi = m_globalContext->markups().find( ri->name());
			if (mi != m_globalContext->markups().end())
			{
				markupContext->putMarkup(
					ri->start_origseg(), ri->start_origpos(),
					ri->end_origseg(), ri->end_origpos(),
					strus::TokenMarkup( ri->name()), mi->second);
			}
			std::vector<strus::stream::TokenPatternMatchResultItem>::const_iterator
				ei = ri->items().begin(), ee = ri->items().end();

			for (; ei != ee; ++ei)
			{
				std::map<std::string,int>::const_iterator mi = m_globalContext->markups().find( ei->name());
				if (mi != m_globalContext->markups().end())
				{
					markupContext->putMarkup(
						ei->start_origseg(), ei->start_origpos(),
						ei->end_origseg(), ei->end_origpos(),
						strus::TokenMarkup( ri->name()), mi->second);
				}
			}
		}
		std::string content = markupContext->markupDocument( segmenterInstance, documentClass, src);
		out << content << std::endl;
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
	unsigned int m_threadid;
	std::string m_outputfile;
	strus::utils::SharedPtr<std::ofstream> m_outputfilestream;
	std::ostream* m_output;
};


int main( int argc, const char* argv[])
{
	std::auto_ptr<strus::ErrorBufferInterface> errorBuffer;
	try
	{
		bool doExit = false;
		int argi = 1;
		std::string outputfile;
		std::string fileext;
		std::string mimetype;
		std::string encoding;
		std::vector<std::string> programfiles;
		std::vector<std::string> selectexpr;
		std::map<std::string,int> markups;
		std::string resultmarker;
		unsigned int nofThreads = 0;
		bool printTokens = false;

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
			else if (0==std::strcmp( argv[argi], "-K") || 0==std::strcmp( argv[argi], "--tokens"))
			{
				printTokens = true;
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
			else if (0==std::strcmp( argv[argi], "-H") || 0==std::strcmp( argv[argi], "--markup"))
			{
				if (argi == argc || argv[argi+1][0] == '-')
				{
					throw strus::runtime_error( _TXT("no argument given to option --markup"));
				}
				++argi;
				markups.insert( std::pair<std::string, int>( argv[argi], markups.size()+1));
			}
			else if (0==std::strcmp( argv[argi], "-Z") || 0==std::strcmp( argv[argi], "--marker"))
			{
				if (argi == argc || argv[argi+1][0] == '-')
				{
					throw strus::runtime_error( _TXT("no argument given to option --marker"));
				}
				++argi;
				if (!resultmarker.empty())
				{
					throw strus::runtime_error( _TXT("option --marker specified twice"));
				}
				resultmarker = argv[argi];
			}
			else if (0==std::strcmp( argv[argi], "-t") || 0==std::strcmp( argv[argi], "--threads"))
			{
				if (argi == argc || argv[argi+1][0] == '-')
				{
					throw strus::runtime_error( _TXT("no argument given to option --threads"));
				}
				++argi;
				if (nofThreads)
				{
					throw strus::runtime_error( _TXT("number of threads option --threads specified twice"));
				}
				nofThreads = getUintValue( argv[argi]);
				if (!nofThreads)
				{
					throw strus::runtime_error( _TXT("number of threafs option --threads is 0"));
				}
			}
			else if (0==std::strcmp( argv[argi], "-o") || 0==std::strcmp( argv[argi], "--output"))
			{
				if (argi == argc || argv[argi+1][0] == '-')
				{
					throw strus::runtime_error( _TXT("no argument given to option --output"));
				}
				++argi;
				if (!outputfile.empty())
				{
					throw strus::runtime_error( _TXT("output file option --output specified twice"));
				}
				outputfile = argv[argi];
				if (outputfile.empty())
				{
					throw strus::runtime_error( _TXT("output file option --output argument is empty"));
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
				throw strus::runtime_error(_TXT("error (%u) reading rule file %s: %s"), ec, pi->c_str(), ::strerror(ec));
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
				pii.get(),
				pii->getTokenPatternMatchInstance(),
				pii->getCharRegexMatchInstance(),
				selectexpr, inputpath, fileext, mimetype, encoding,
				markups, resultmarker, printTokens);

		std::cerr << "start matching ..." << std::endl;
		if (nofThreads)
		{
			fprintf( stderr, _TXT("starting %u threads for evaluation ...\n"), nofThreads);

			std::vector<ThreadContext> ctxar;
			for (unsigned int ti=0; ti<nofThreads; ++ti)
			{
				ctxar.push_back( ThreadContext( &globalContext, ti+1, outputfile));
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
			ThreadContext ctx( &globalContext, 0, outputfile);
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
			throw std::runtime_error( "error processing documents");
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


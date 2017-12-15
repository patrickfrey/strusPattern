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
#include "strus/reference.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/patternMatcherInterface.hpp"
#include "strus/patternMatcherInstanceInterface.hpp"
#include "strus/patternMatcherContextInterface.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/thread.hpp"
#include "testUtils.hpp"
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

static void initRand()
{
	time_t nowtime;
	struct tm* now;

	::time( &nowtime);
	now = ::localtime( &nowtime);

	::srand( ((now->tm_year+1) * (now->tm_mon+100) * (now->tm_mday+1)));
}
#define RANDINT(MIN,MAX) ((std::rand()%(MAX-MIN))+MIN)

strus::ErrorBufferInterface* g_errorBuffer = 0;

static void createTermOpRule( strus::PatternMatcherInstanceInterface* ptinst, const char* joinopstr, unsigned int range, unsigned int cardinality, unsigned int* param, std::size_t paramsize)
{
	std::size_t pi = 0, pe = paramsize;
	bool with_delim = false;
	if (std::strcmp( joinopstr, "sequence_struct") == 0
	|| std::strcmp( joinopstr, "within_struct") == 0)
	{
		with_delim = true;
	}
	if (with_delim)
	{
		unsigned int delim_termid = strus::utils::termId( strus::utils::SentenceDelim, 0);
		ptinst->pushTerm( delim_termid);
		++paramsize;
	}
	for (; pi != pe; ++pi)
	{
		unsigned int termid = strus::utils::termId( strus::utils::Token, param[pi]);
		ptinst->pushTerm( termid);
		char variablename[ 32];
		snprintf( variablename, sizeof(variablename), "A%u", (unsigned int)pi);
		ptinst->attachVariable( variablename);
	}
	strus::utils::JoinOperation joinop = strus::utils::joinOperation( joinopstr);
	ptinst->pushExpression( joinop, paramsize, range, cardinality);
}

static void createTermOpPattern( strus::PatternMatcherInstanceInterface* ptinst, const char* operation, unsigned int range, unsigned int cardinality, unsigned int* param, std::size_t paramsize)
{
	std::string rulename = operation;
	std::size_t pi = 0, pe = paramsize;
	for (; pi != pe; ++pi)
	{
		char strbuf[ 32];
		snprintf( strbuf, sizeof( strbuf), "_%u", (unsigned int)pi);
		rulename.append( strbuf);
	}
	createTermOpRule( ptinst, operation, range, cardinality, param, paramsize);
	ptinst->definePattern( rulename, true);
}

static void createRules( strus::PatternMatcherInstanceInterface* ptinst, const char* joinop, unsigned int nofFeatures, unsigned int nofRules)
{
	strus::utils::ZipfDistribution featdist( nofFeatures, 0.8);
	strus::utils::ZipfDistribution rangedist( 10, 1.7);
	strus::utils::ZipfDistribution selopdist( 5);
	unsigned int ni=0, ne=nofRules;
	for (; ni < ne; ++ni)
	{
		unsigned int range = rangedist.random()+1;
		unsigned int cardinality = 0;
		unsigned int param[2];
		param[0] = featdist.random();
		param[1] = featdist.random();

		if (joinop)
		{
			createTermOpPattern( ptinst, joinop, range, cardinality, param, 2);
		}
		else
		{
			unsigned int selectOp = selopdist.random()-1;
			static const char* opar[] =
			{
				"sequence",
				"within",
				"sequence_struct",
				"within_struct",
				"any"
			};
			const char* op = opar[ selectOp];
			createTermOpPattern( ptinst, op, range, cardinality, param, 2);
		}
	}
}

static std::vector<strus::utils::Document> createRandomDocuments( unsigned int collSize, unsigned int docSize, unsigned int nofFeatures)
{
	std::vector<strus::utils::Document> rt;
	unsigned int di=0, de=collSize;

	for (di=0; di < de; ++di)
	{
		strus::utils::Document doc( strus::utils::createRandomDocument( di+1, docSize, nofFeatures));
		rt.push_back( doc);
	}
	return rt;
}

static unsigned int processDocument( const strus::PatternMatcherInstanceInterface* ptinst, const strus::utils::Document& doc, std::map<std::string,double>& globalstats)
{
	strus::local_ptr<strus::PatternMatcherContextInterface> mt( ptinst->createContext());
	std::vector<strus::utils::DocumentItem>::const_iterator di = doc.itemar.begin(), de = doc.itemar.end();
	unsigned int didx = 0;
	for (; di != de; ++di,++didx)
	{
		mt->putInput( strus::analyzer::PatternLexem( di->termid, di->pos, 0/*segpos*/, didx, 1));
	}
	if (g_errorBuffer->hasError())
	{
		throw std::runtime_error("error matching rules");
	}
	std::vector<strus::analyzer::PatternMatcherResult> results = mt->fetchResults();
	unsigned int nofMatches = results.size();

	strus::analyzer::PatternMatcherStatistics stats = mt->getStatistics();
	std::vector<strus::analyzer::PatternMatcherStatistics::Item>::const_iterator
		li = stats.items().begin(), le = stats.items().end();
	for (; li != le; ++li)
	{
		if (li->value() < 0.0) throw std::runtime_error("statistics got negative");
		globalstats[ li->name()] += li->value();
	}

#ifdef STRUS_LOWLEVEL_DEBUG
	strus::utils::printResults( std::cout, results);
	std::cout << "nof matches " << results.size();
	strus::analyzer::PatternMatcherStatistics stats = mt->getStatistics();
	strus::utils::printStatistics( std::cerr, stats);
#endif
	return nofMatches;
}

static void printUsage( int argc, const char* argv[])
{
	std::cerr << "usage: " << argv[0] << " [<options>] <features> <nofdocs> <docsize> <nofpatterns> [<joinop>]" << std::endl;
	std::cerr << "<options>= -h print this usage, -o do optimize automaton, -t <N> number of threads" << std::endl;
	std::cerr << "<features>= number of distinct features" << std::endl;
	std::cerr << "<nofdocs> = number of documents to insert" << std::endl;
	std::cerr << "<docsize> = size of a document" << std::endl;
	std::cerr << "<nofpatterns> = number of patterns to use" << std::endl;
	std::cerr << "<joinop> = operator to use for patterns (default all)" << std::endl;
}

static unsigned int processDocuments( const strus::PatternMatcherInstanceInterface* ptinst, const std::vector<strus::utils::Document>& docs, std::map<std::string,double>& stats)
{
	unsigned int totalNofmatches = 0;
	std::vector<strus::utils::Document>::const_iterator di = docs.begin(), de = docs.end();
	for (; di != de; ++di)
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "document " << di->id << ":" << std::endl;
#endif
		unsigned int nofmatches = processDocument( ptinst, *di, stats);
		totalNofmatches += nofmatches;
		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error("error matching rule");
		}
	}
	return totalNofmatches;
}

class Globals
{
public:
	explicit Globals( const strus::PatternMatcherInstanceInterface* ptinst_)
		:ptinst(ptinst_),totalNofMatches(0),totalNofDocs(0){}

	const strus::PatternMatcherInstanceInterface* ptinst;
	std::map<std::string,double> stats;
	unsigned int totalNofMatches;
	unsigned int totalNofDocs;
	std::vector<std::string> errors;

public:
	void accumulateStats( std::map<std::string,double> addstats, unsigned int nofMatches, unsigned int nofDocs)
	{
		strus::scoped_lock lock( mutex);
		std::map<std::string,double>::const_iterator
			li = addstats.begin(), le = addstats.end();
		for (; li != le; ++li)
		{
			if (li->second < 0.0) throw std::runtime_error("statistics got negative [part]");
			double vv = stats[ li->first] += li->second;
			if (vv < 0.0) throw std::runtime_error("statistics got negative [total]");
		}
		totalNofMatches += nofMatches;
		totalNofDocs += nofDocs;
	}

private:
	strus::mutex mutex;
};

class Task
{
public:
	Task( Globals* globals_, const std::vector<strus::utils::Document>& docs_)
		:m_globals(globals_),m_docs(docs_){}
	Task( const Task& o)
		:m_globals(o.m_globals),m_docs(o.m_docs){}
	~Task(){}

	void run()
	{
		std::map<std::string,double> stats;
		unsigned int nofMatches = processDocuments( m_globals->ptinst, m_docs, stats);
		m_globals->accumulateStats( stats, nofMatches, m_docs.size());
		if (g_errorBuffer->hasError())
		{
			m_globals->errors.push_back( g_errorBuffer->fetchError());
		}
	}
private:
	Globals* m_globals;
	std::vector<strus::utils::Document> m_docs;
};


int main( int argc, const char** argv)
{
	try
	{
		if (argc <= 1)
		{
			printUsage( argc, argv);
			return 0;
		}
		unsigned int nofThreads = 0;
		bool doOpimize = false;
		int argidx = 1;
		for (; argidx < argc && argv[argidx][0] == '-'; ++argidx)
		{
			if (std::strcmp( argv[argidx], "-h") == 0)
			{
				printUsage( argc, argv);
				return 0;
			}
			else if (std::strcmp( argv[argidx], "-o") == 0)
			{
				doOpimize = true;
			}
			else if (std::strcmp( argv[argidx], "-t") == 0)
			{
				nofThreads = strus::utils::getUintValue( argv[++argidx]);
			}
		}
		if (argc - argidx < 4)
		{
			std::cerr << "ERROR too few arguments" << std::endl;
			printUsage( argc, argv);
			return 1;
		}
		else if (argc - argidx > 5)
		{
			std::cerr << "ERROR too many arguments" << std::endl;
			printUsage( argc, argv);
			return 1;
		}
		initRand();
		g_errorBuffer = strus::createErrorBuffer_standard( 0, 1+nofThreads);
		if (!g_errorBuffer)
		{
			std::cerr << "construction of error buffer failed" << std::endl;
			return -1;
		}
		unsigned int nofFeatures = strus::utils::getUintValue( argv[ argidx+0]);
		unsigned int nofDocuments = strus::utils::getUintValue( argv[ argidx+1]);
		unsigned int documentSize = strus::utils::getUintValue( argv[ argidx+2]);
		unsigned int nofPatterns = strus::utils::getUintValue( argv[ argidx+3]);
		const char* joinop = (argc - argidx > 4)?argv[ argidx+4]:0;

		strus::local_ptr<strus::PatternMatcherInterface> pt( strus::createPatternMatcher_stream( g_errorBuffer));
		if (!pt.get()) throw std::runtime_error("failed to create pattern matcher");
		strus::local_ptr<strus::PatternMatcherInstanceInterface> ptinst( pt->createInstance());
		if (!ptinst.get()) throw std::runtime_error("failed to create pattern matcher instance");
		createRules( ptinst.get(), joinop, nofFeatures, nofPatterns);
		if (doOpimize)
		{
			ptinst->compile();
		}
		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error( "error creating automaton for evaluating rules");
		}
		Globals globals( ptinst.get());
		if (nofThreads)
		{
			std::cerr << "starting " << nofThreads << " threads for rule evaluation ..." << std::endl;

			std::vector<Task> taskar;
			for (unsigned int ti=0; ti<nofThreads; ++ti)
			{
				taskar.push_back( Task( &globals, createRandomDocuments( nofDocuments, documentSize, nofFeatures)));
			}
			std::vector<strus::Reference<strus::thread> > threadGroup;
			{
				for (unsigned int ti=0; ti<nofThreads; ++ti)
				{
					Task* task = &taskar.at( ti);
					strus::Reference<strus::thread> th( new strus::thread( &Task::run, task));
					threadGroup.push_back( th);
				}
				std::vector<strus::Reference<strus::thread> >::iterator gi = threadGroup.begin(), ge = threadGroup.end();
				for (; gi != ge; ++gi) (*gi)->join();
			}
			if (!globals.errors.empty())
			{
				std::vector<std::string>::const_iterator ei = globals.errors.begin(), ee = globals.errors.end();
				for (; ei != ee; ++ei)
				{
					std::cerr << "ERROR in thread: " << *ei << std::endl;
				}
			}
		}
		else
		{
			std::vector<strus::utils::Document> docs = createRandomDocuments( nofDocuments, documentSize, nofFeatures);
			std::cerr << "starting rule evaluation ..." << std::endl;

			std::map<std::string,double> stats;
			globals.totalNofMatches = processDocuments( ptinst.get(), docs, globals.stats);
			globals.totalNofDocs = docs.size();
		}
		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error("uncaugth exception");
		}
		std::cerr << "OK" << std::endl;
		std::cerr << "processed " << nofPatterns << " patterns on " << globals.totalNofDocs << " documents with total " << globals.totalNofMatches << " matches" << std::endl;
		std::cerr << "statistiscs:" << std::endl;
		std::map<std::string,double>::const_iterator gi = globals.stats.begin(), ge = globals.stats.end();
		for (; gi != ge; ++gi)
		{
			int value;
			if (gi->first == "nofTriggersAvgActive")
			{
				value = (uint64_t)(gi->second/globals.totalNofDocs + 0.5);
			}
			else
			{
				value = (uint64_t)(gi->second + 0.5);
			}
			std::cerr << "\t" << gi->first << ": " << value << std::endl;
		}
		delete g_errorBuffer;
		return 0;
	}
	catch (const std::runtime_error& err)
	{
		if (g_errorBuffer && g_errorBuffer->hasError())
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



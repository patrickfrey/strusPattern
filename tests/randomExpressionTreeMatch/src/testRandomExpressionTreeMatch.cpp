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
#include "strus/reference.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/tokenPatternMatchInterface.hpp"
#include "strus/tokenPatternMatchInstanceInterface.hpp"
#include "strus/tokenPatternMatchContextInterface.hpp"
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
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"

#undef STRUS_LOWLEVEL_DEBUG
#define RANDINT(MIN,MAX) ((std::rand()%(MAX-MIN))+MIN)

strus::ErrorBufferInterface* g_errorBuffer = 0;

class TreeNode
{
public:
	typedef strus::TokenPatternMatchInstanceInterface::JoinOperation JoinOperation;

	explicit TreeNode( unsigned int term_)
		:m_term(term_),m_op(0),m_range(0),m_cardinality(){}
	TreeNode( const JoinOperation& op_, std::vector<TreeNode*> args_, unsigned int range_, unsigned int cardinality_)
		:m_term(0),m_op(op_),m_args(args_),m_range(range_),m_cardinality(cardinality_){}
	TreeNode( const TreeNode& o)
		:m_term(o.m_term),m_op(o.m_op),m_args(o.m_args),m_range(o.m_range),m_cardinality(o.m_cardinality){}

	unsigned int term() const
	{
		return m_term;
	}
	JoinOperation op() const
	{
		return m_op;
	}
	const std::vector<TreeNode*>& args() const
	{
		return m_args;
	}
	unsigned int range() const
	{
		return m_range;
	}
	unsigned int cardinality() const
	{
		return m_cardinality;
	}
	static void deleteTree( TreeNode* nd)
	{
		std::vector<TreeNode*>::iterator ai = nd->m_args.begin(), ae = nd->m_args.end();
		for (; ai != ae; ++ai)
		{
			destroy( *ai);
		}
		delete nd;
	}

private:
	unsigned int m_term;
	JoinOperation m_op;
	std::vector<TreeNode*> m_args;
	unsigned int m_range;
	unsigned int m_cardinality;
};

class GlobalContext
{
public:
	GlobalContext( unsigned int nofFeatures, unsigned int nofRules)
		:m_nofRules(nofRules_),m_featdist(nofFeatures, 0.8),m_rangedist(20,1.3),m_selopdist( 5),m_argcdist(5,1.3){}

	unsigned int randomTerm() const
	{
		return m_featdist.random();
	}
	unsigned int randomRange() const
	{
		return m_featdist.random()-1;
	}
	TreeNode::JoinOperation randomOp() const
	{
		return (TreeNode::JoinOperation)(m_selopdist.random()-1);
	}
	unsigned int randomArgc() const
	{
		unsigned int rt = m_argcdist.random();
		if (rt == 1 && RANDINT(1,5)>=2) ++rt;
		return rt;
	}

private:
	unsigned int m_nofRules;
	strus::utils::ZipfDistribution m_featdist;
	strus::utils::ZipfDistribution m_rangedist;
	strus::utils::ZipfDistribution m_selopdist;
	strus::utils::ZipfDistribution m_argcdist;
}

static TreeNode* createRandomTree( GlobalContext* ctx, unsigned int depth)
{
	if (RANDINT( 1,5-depth) == 1)
	{
		return new TreeNode( 0);
	}
	else
	{
		unsigned int range = ctx->randomRange();
		unsigned int cardinality = RANDINT( 1,5)>=2?0:RANDINT(1,range);
		unsigned int argc = ctx->randomArgc();
		TreeNode::JoinOperation op = ctx->randomOp();
		std::vector<TreeNode*> args;
		for (unsigned int ai=0; ai<argc; ++ai)
		{
			args.push_back( createRandomTree( ctx), depth+1);
		}
		return new TreeNode( op, args, range, cardinality);
	}
}

static void createTermOpRule( strus::TokenPatternMatchInstanceInterface* ptinst, const char* joinopstr, unsigned int range, unsigned int cardinality, unsigned int* param, std::size_t paramsize)
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
		unsigned int delim_termid = strus::utils::termId( SentenceDelim, 0);
		ptinst->pushTerm( delim_termid);
		++paramsize;
	}
	for (; pi != pe; ++pi)
	{
		unsigned int termid = termId( Token, param[pi]);
		ptinst->pushTerm( termid);
		char variablename[ 32];
		snprintf( variablename, sizeof(variablename), "A%u", (unsigned int)pi);
		ptinst->attachVariable( variablename, 1.0f);
	}
	JoinOperation joinop = strus::utils::joinOperation( joinopstr);
	ptinst->pushExpression( joinop, paramsize, range, cardinality);
}

static void createTermOpPattern( strus::TokenPatternMatchInstanceInterface* ptinst, const char* operation, unsigned int range, unsigned int cardinality, unsigned int* param, std::size_t paramsize)
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

static void createRules( strus::TokenPatternMatchInstanceInterface* ptinst, const char* joinop, unsigned int nofFeatures, unsigned int nofRules)
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
		strus::utils::Document doc( createRandomDocument( di+1, docSize, nofFeatures));
		rt.push_back( doc);
	}
	return rt;
}

static unsigned int matchRules( const strus::TokenPatternMatchInstanceInterface* ptinst, const strus::utils::Document& doc, std::map<std::string,double>& globalstats)
{
	std::auto_ptr<strus::TokenPatternMatchContextInterface> mt( ptinst->createContext());
	std::vector<strus::utils::DocumentItem>::const_iterator di = doc.itemar.begin(), de = doc.itemar.end();
	unsigned int didx = 0;
	for (; di != de; ++di,++didx)
	{
		mt->putInput( strus::stream::PatternMatchToken( di->termid, di->pos, didx, 1));
		if (g_errorBuffer->hasError()) throw std::runtime_error("error matching rules");
	}
	std::vector<strus::stream::TokenPatternMatchResult> results = mt->fetchResults();
	unsigned int nofMatches = results.size();

	strus::stream::TokenPatternMatchStatistics stats = mt->getStatistics();
	std::vector<strus::stream::TokenPatternMatchStatistics::Item>::const_iterator
		li = stats.items().begin(), le = stats.items().end();
	for (; li != le; ++li)
	{
		globalstats[ li->name()] += li->value();
	}

#ifdef STRUS_LOWLEVEL_DEBUG
	std::vector<strus::stream::TokenPatternMatchResult>::const_iterator
		ri = results.begin(), re = results.end();
	for (; ri != re; ++ri)
	{
		std::cout << "match '" << ri->name() << "':";
		std::vector<strus::stream::TokenPatternMatchResultItem>::const_iterator
			ei = ri->items().begin(), ee = ri->items().end();
	
		for (; ei != ee; ++ei)
		{
			std::cout << " " << ei->name() << " [" << ei->ordpos()
					<< ", " << ei->origpos() << ", " << ei->origsize() << "]";
		}
		std::cout << std::endl;
	}
	std::cerr << "document stats:" << std::endl;
	std::vector<strus::stream::PatternMatchStatistics::Item>::const_iterator
		gi = stats.items().begin(), ge = stats.items().end();
	for (; gi != ge; ++gi)
	{
		std::cerr << "\t" << gi->name() << ": " << floor( gi->value()+0.5) << std::endl;
	}
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

static unsigned int runMatching( const strus::TokenPatternMatchInstanceInterface* ptinst, const std::vector<strus::utils::Document>& docs, std::map<std::string,double>& stats)
{
	unsigned int totalNofmatches = 0;
	std::vector<strus::utils::Document>::const_iterator di = docs.begin(), de = docs.end();
	for (; di != de; ++di)
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "document " << di->id << ":" << std::endl;
#endif
		unsigned int nofmatches = matchRules( ptinst, *di, stats);
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
	explicit Globals( const strus::TokenPatternMatchInstanceInterface* ptinst_)
		:ptinst(ptinst_),totalNofMatches(0),totalNofDocs(0){}

	const strus::TokenPatternMatchInstanceInterface* ptinst;
	std::map<std::string,double> stats;
	unsigned int totalNofMatches;
	unsigned int totalNofDocs;
	std::vector<std::string> errors;

public:
	void accumulateStats( std::map<std::string,double> addstats, unsigned int nofMatches, unsigned int nofDocs)
	{
		boost::mutex::scoped_lock lock( mutex);
		std::map<std::string,double>::const_iterator
			li = addstats.begin(), le = addstats.end();
		for (; li != le; ++li)
		{
			stats[ li->first] += li->second;
		}
		totalNofMatches += nofMatches;
		totalNofDocs += nofDocs;
	}

private:
	boost::mutex mutex;
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
		unsigned int nofMatches = runMatching( m_globals->ptinst, m_docs, stats);
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

		std::auto_ptr<strus::TokenPatternMatchInterface> pt( strus::createTokenPatternMatch_standard( g_errorBuffer));
		if (!pt.get()) throw std::runtime_error("failed to create pattern matcher");
		std::auto_ptr<strus::TokenPatternMatchInstanceInterface> ptinst( pt->createInstance());
		if (!ptinst.get()) throw std::runtime_error("failed to create pattern matcher instance");
		createRules( ptinst.get(), joinop, nofFeatures, nofPatterns);
		if (doOpimize)
		{
			ptinst->compile( strus::stream::TokenPatternMatchOptions());
		}
		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error( "error creating automaton for evaluating rules");
		}
		Globals globals( ptinst.get());
		boost::posix_time::time_duration duration;
		if (nofThreads)
		{
			std::cerr << "starting " << nofThreads << " threads for rule evaluation ..." << std::endl;
			boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::local_time();

			std::vector<Task> taskar;
			for (unsigned int ti=0; ti<nofThreads; ++ti)
			{
				taskar.push_back( Task( &globals, createRandomDocuments( nofDocuments, documentSize, nofFeatures)));
			}
			{
				boost::thread_group tgroup;
				for (unsigned int ti=0; ti<nofThreads; ++ti)
				{
					tgroup.create_thread( boost::bind( &Task::run, &taskar[ti]));
				}
				tgroup.join_all();
			}
			boost::posix_time::ptime end_time = boost::posix_time::microsec_clock::local_time();
			duration = end_time - start_time;
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
			boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::local_time();
	
			std::map<std::string,double> stats;
			globals.totalNofMatches = runMatching( ptinst.get(), docs, globals.stats);
			globals.totalNofDocs = docs.size();

			boost::posix_time::ptime end_time = boost::posix_time::microsec_clock::local_time();
			duration = end_time - start_time;
		}
		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error("uncaugth exception");
		}
		std::cerr << "OK" << std::endl;
		std::cerr << "processed " << nofPatterns << " patterns on " << globals.totalNofDocs << " documents with total " << globals.totalNofMatches << " matches in " << duration.total_milliseconds() << " milliseconds" << std::endl;
		std::cerr << "statistiscs:" << std::endl;
		std::map<std::string,double>::const_iterator gi = globals.stats.begin(), ge = globals.stats.end();
		for (; gi != ge; ++gi)
		{
			int value;
			if (gi->first == "nofTriggersAvgActive")
			{
				value = (int)(gi->second/globals.totalNofDocs + 0.5);
			}
			else
			{
				value = (int)(gi->second + 0.5);
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



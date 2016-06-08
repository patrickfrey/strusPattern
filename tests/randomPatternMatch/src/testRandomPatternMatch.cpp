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
#include "strus/streamPatternMatchInterface.hpp"
#include "strus/streamPatternMatchInstanceInterface.hpp"
#include "strus/streamPatternMatchContextInterface.hpp"
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

#undef STRUS_LOWLEVEL_DEBUG
#define RANDINT(MIN,MAX) ((std::rand()%(MAX-MIN))+MIN)

strus::ErrorBufferInterface* g_errorBuffer = 0;

struct DocumentItem
{
	unsigned int pos;
	unsigned int termid;

	DocumentItem( unsigned int pos_, unsigned int termid_)
		:pos(pos_),termid(termid_){}
	DocumentItem( const DocumentItem& o)
		:pos(o.pos),termid(o.termid){}
};

struct Document
{
	std::string id;
	std::vector<DocumentItem> itemar;

	explicit Document( const std::string& id_)
		:id(id_){}
	Document( const Document& o)
		:id(o.id),itemar(o.itemar){}
};

class ZipfDistribution
{
public:
	explicit ZipfDistribution( std::size_t size, double S = 0.0)
	{
		std::size_t ii=1, ie=size;
		m_ar.push_back( 1.0);
		for (; ii < ie; ++ii)
		{
			if (S > std::numeric_limits<double>::epsilon())
			{
				m_ar.push_back( m_ar[ ii-1] + 1.0 / pow((double)(ii+1), S));
			}
			else
			{
				m_ar.push_back( m_ar[ ii-1] + 1.0 / (double)(ii+1));
			}
		}
	}

	unsigned int random() const
	{
		double val = m_ar.back() * (double)std::rand() / (double)RAND_MAX;
		std::size_t ii=0;
		for (; ii<m_ar.size(); ++ii)
		{
			if (val < m_ar[ii])
			{
				break;
			}
			while (m_ar.size() - ii > 100 && val > m_ar[ ii+100])
			{
				ii+=100;
			}
		}
		return ii+1;
	}

private:
	std::vector<double> m_ar;
};

enum TermType {Token, SentenceDelim};
static unsigned int termId( TermType tp, unsigned int no)
{
	return ((unsigned int)tp << 24) + no;
}

static Document createRandomDocument( unsigned int no, unsigned int size, unsigned int mod)
{
	ZipfDistribution featdist( mod);
	char docid[ 32];
	snprintf( docid, sizeof(docid), "doc_%u", no);
	Document rt( docid);
	unsigned int ii = 0, ie = size;
	for (; ii < ie; ++ii)
	{
		unsigned int tok = featdist.random();
		rt.itemar.push_back( DocumentItem( ii+1, termId( Token, tok)));
		if (RANDINT(0,12) == 0)
		{
			rt.itemar.push_back( DocumentItem( ii+1, termId( SentenceDelim, 0)));
		}
	}
	return rt;
}

typedef strus::StreamPatternMatchInstanceInterface::JoinOperation JoinOperation;
static JoinOperation joinOperation( const char* joinopstr)
{
	static const char* ar[] = {"sequence","sequence_struct","within","within_struct","any",0};
	std::size_t ai = 0;
	for (; ar[ai]; ++ai)
	{
		if (0 == std::strcmp( joinopstr, ar[ai]))
		{
			return (JoinOperation)ai;
		}
	}
	throw std::runtime_error( "unknown join operation");
}

static void createTermOpRule( strus::StreamPatternMatchInstanceInterface* ptinst, const char* joinopstr, unsigned int range, unsigned int cardinality, unsigned int* param, std::size_t paramsize)
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
		unsigned int delim_termid = termId( SentenceDelim, 0);
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
	JoinOperation joinop = joinOperation( joinopstr);
	ptinst->pushExpression( joinop, paramsize, range, cardinality);
}

static void createTermOpPattern( strus::StreamPatternMatchInstanceInterface* ptinst, const char* operation, unsigned int range, unsigned int cardinality, unsigned int* param, std::size_t paramsize)
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
	ptinst->closePattern( rulename, true);
}

static void createRules( strus::StreamPatternMatchInstanceInterface* ptinst, const char* joinop, unsigned int nofFeatures, unsigned int nofRules)
{
	ZipfDistribution featdist( nofFeatures, 0.8);
	ZipfDistribution rangedist( 10, 1.7);
	ZipfDistribution selopdist( 5);
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

static std::vector<Document> createRandomDocuments( unsigned int collSize, unsigned int docSize, unsigned int nofFeatures)
{
	std::vector<Document> rt;
	unsigned int di=0, de=collSize;

	for (di=0; di < de; ++di)
	{
		Document doc( createRandomDocument( di+1, docSize, nofFeatures));
		rt.push_back( doc);
	}
	return rt;
}

static unsigned int getUintValue( const char* arg)
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

static std::string doubleToString( double val_)
{
	unsigned int val = (unsigned int)::floor( val_ * 1000);
	unsigned int val_sec = val / 1000;
	unsigned int val_ms = val & 1000;
	std::ostringstream val_str;
	val_str << val_sec << "." << std::setfill('0') << std::setw(3) << val_ms;
	return val_str.str();
}

static unsigned int matchRules( const strus::StreamPatternMatchInstanceInterface* ptinst, const Document& doc, std::map<std::string,double>& globalstats)
{
	std::auto_ptr<strus::StreamPatternMatchContextInterface> mt( ptinst->createContext());
	std::vector<DocumentItem>::const_iterator di = doc.itemar.begin(), de = doc.itemar.end();
	unsigned int didx = 0;
	for (; di != de; ++di,++didx)
	{
		mt->putInput( strus::stream::PatternMatchTerm( di->termid, di->pos, didx, 1));
		if (g_errorBuffer->hasError()) throw std::runtime_error("error matching rules");
	}
	std::vector<strus::stream::PatternMatchResult> results = mt->fetchResults();
	unsigned int nofMatches = results.size();

	strus::stream::PatternMatchStatistics stats = mt->getStatistics();
	std::vector<strus::stream::PatternMatchStatistics::Item>::const_iterator
		li = stats.items().begin(), le = stats.items().end();
	for (; li != le; ++li)
	{
		globalstats[ li->name()] += li->value();
	}

#ifdef STRUS_LOWLEVEL_DEBUG
	std::vector<strus::stream::PatternMatchResult>::const_iterator
		ri = results.begin(), re = results.end();
	for (; ri != re; ++ri)
	{
		std::cout << "match '" << ri->name() << "':";
		std::vector<strus::stream::PatternMatchResultItem>::const_iterator
			ei = ri->itemlist().begin(), ee = ri->itemlist().end();
	
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

static unsigned int runMatching( const strus::StreamPatternMatchInstanceInterface* ptinst, const std::vector<Document>& docs, std::map<std::string,double>& stats)
{
	unsigned int totalNofmatches = 0;
	std::vector<Document>::const_iterator di = docs.begin(), de = docs.end();
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
	explicit Globals( const strus::StreamPatternMatchInstanceInterface* ptinst_)
		:ptinst(ptinst_),totalNofMatches(0),totalNofDocs(0){}

	const strus::StreamPatternMatchInstanceInterface* ptinst;
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
	Task( Globals* globals_, const std::vector<Document>& docs_)
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
	std::vector<Document> m_docs;
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
				nofThreads = getUintValue( argv[++argidx]);
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
		unsigned int nofFeatures = getUintValue( argv[ argidx+0]);
		unsigned int nofDocuments = getUintValue( argv[ argidx+1]);
		unsigned int documentSize = getUintValue( argv[ argidx+2]);
		unsigned int nofPatterns = getUintValue( argv[ argidx+3]);
		const char* joinop = (argc - argidx > 4)?argv[ argidx+4]:0;

		std::auto_ptr<strus::StreamPatternMatchInterface> pt( strus::createStreamPatternMatch_standard( g_errorBuffer));
		if (!pt.get()) throw std::runtime_error("failed to create pattern matcher");
		std::auto_ptr<strus::StreamPatternMatchInstanceInterface> ptinst( pt->createInstance());
		if (!ptinst.get()) throw std::runtime_error("failed to create pattern matcher instance");
		createRules( ptinst.get(), joinop, nofFeatures, nofPatterns);
		if (doOpimize)
		{
			ptinst->optimize( strus::StreamPatternMatchInstanceInterface::OptimizeOptions(0.01f,5,5));
		}
		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error( "error creating automaton for evaluating rules");
		}
		Globals globals( ptinst.get());
		double duration = 0.0;
		if (nofThreads)
		{
			std::cerr << "starting " << nofThreads << " threads for rule evaluation ..." << std::endl;
			time_t start_time;
			time_t end_time;
			std::time( &start_time);

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
			time( &end_time);
			duration = std::difftime( end_time, start_time);
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
			std::vector<Document> docs = createRandomDocuments( nofDocuments, documentSize, nofFeatures);
			std::cerr << "starting rule evaluation ..." << std::endl;
			time_t start_time;
			time_t end_time;
			std::time( &start_time);
	
			std::map<std::string,double> stats;
			globals.totalNofMatches = runMatching( ptinst.get(), docs, globals.stats);
			globals.totalNofDocs = docs.size();

			time( &end_time);
			duration = std::difftime( end_time, start_time);
		}
		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error("uncaugth exception");
		}
		std::cerr << "OK" << std::endl;
		std::cerr << "processed " << nofPatterns << " patterns on " << globals.totalNofDocs << " documents with total " << globals.totalNofMatches << " matches in " << doubleToString(duration) << " seconds" << std::endl;
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



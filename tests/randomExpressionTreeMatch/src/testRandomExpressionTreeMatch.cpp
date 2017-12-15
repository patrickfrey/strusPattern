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
#include "strus/base/fileio.hpp"
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
#include <set>
#include <string>
#include <vector>
#include <memory>
#include <limits>
#include <ctime>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <algorithm>

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

class TreeNode
{
public:
	typedef strus::PatternMatcherInstanceInterface::JoinOperation JoinOperation;

	explicit TreeNode( unsigned int term_)
		:m_term(term_),m_op(strus::PatternMatcherInstanceInterface::OpSequence),m_range(0),m_cardinality(),m_rulename(){}
	TreeNode( const JoinOperation& op_, const std::vector<TreeNode*>& args_, unsigned int range_, unsigned int cardinality_)
		:m_term(0),m_op(op_),m_args(args_),m_range(range_),m_cardinality(cardinality_),m_rulename(){}
	TreeNode( const TreeNode& o)
		:m_term(o.m_term),m_op(o.m_op),m_args(o.m_args),m_range(o.m_range),m_cardinality(o.m_cardinality),m_variable(o.m_variable),m_rulename(o.m_rulename){}

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
	const char* variable() const
	{
		return m_variable.empty()?0:m_variable.c_str();
	}
	void attachVariable( unsigned int variable_)
	{
		char namestr[ 64];
		::snprintf( namestr, sizeof(namestr), "V%u", variable_);
		m_variable.append( namestr);
	}
	void removeVariable()
	{
		m_variable.clear();
	}
	void setName( unsigned int idx_)
	{
		char namestr[ 64];
		::snprintf( namestr, sizeof(namestr), "pattern_%u", idx_);
		m_rulename.append( namestr);
	}
	const char* name() const
	{
		return m_rulename.empty()?0:m_rulename.c_str();
	}
	static void deleteTree( TreeNode* nd)
	{
		deleteTreeAr( nd->m_args);
		delete nd;
	}
	static void deleteTreeAr( std::vector<TreeNode*>& ar)
	{
		std::vector<TreeNode*>::iterator ai = ar.begin(), ae = ar.end();
		for (; ai != ae; ++ai)
		{
			deleteTree( *ai);
		}
	}
	static const char* opName( const JoinOperation& op)
	{
		switch (op)
		{
			case strus::PatternMatcherInstanceInterface::OpSequence:
				return "sequence";
			case strus::PatternMatcherInstanceInterface::OpSequenceImm:
				return "sequence_imm";
			case strus::PatternMatcherInstanceInterface::OpSequenceStruct:
				return "sequence_struct";
			case strus::PatternMatcherInstanceInterface::OpWithin:
				return "within";
			case strus::PatternMatcherInstanceInterface::OpWithinStruct:
				return "within_struct";
			case strus::PatternMatcherInstanceInterface::OpAny:
				return "any";
			case strus::PatternMatcherInstanceInterface::OpAnd:
				return "and";
		}
		return 0;
	}

	static void print( std::ostream& out, const TreeNode* nd, std::size_t indent=1)
	{
		if (nd->name())
		{
			out << nd->name() << ":" << std::endl;
		}
		std::string indentstr( indent*2, ' ');
		if (nd->m_term && nd->args().empty())
		{
			out << indentstr;
			out << "term '" << nd->term() << "'";
			if (nd->variable())
			{
				out << " -> " << nd->m_variable;
			}
			out << std::endl;
		}
		else
		{
			out << indentstr;
			out << opName( nd->op()) << ", cardinality " << nd->cardinality() << ", range " << nd->range();
			if (nd->variable())
			{
				out << " -> " << nd->m_variable;
			}
			out << std::endl;
			std::vector<TreeNode*>::const_iterator ai = nd->m_args.begin(), ae = nd->m_args.end();
			for (; ai != ae; ++ai)
			{
				print( out, *ai, indent+1);
			}
		}
	}
	std::string tostring() const
	{
		std::ostringstream outbuf;
		print( outbuf, this);
		return outbuf.str();
	}

private:
	unsigned int m_term;
	JoinOperation m_op;
	std::vector<TreeNode*> m_args;
	unsigned int m_range;
	unsigned int m_cardinality;
	std::string m_variable;
	std::string m_rulename;
};

class GlobalContext
{
public:
	GlobalContext( unsigned int nofFeatures, unsigned int nofRules_)
		:m_nofRules(nofRules_),m_featdist(nofFeatures, 0.8),m_rangedist(20,1.3),m_selopdist(5),m_argcdist(5,1.3){}

	unsigned int randomTerm() const
	{
		return m_featdist.random();
	}
	unsigned int randomRange() const
	{
		return m_rangedist.random()-1;
	}
	TreeNode::JoinOperation randomOp() const
	{
		const TreeNode::JoinOperation ar[5] =
		{
			strus::PatternMatcherInstanceInterface::OpSequence,
			strus::PatternMatcherInstanceInterface::OpSequenceStruct,
			strus::PatternMatcherInstanceInterface::OpWithin,
			strus::PatternMatcherInstanceInterface::OpWithinStruct,
			strus::PatternMatcherInstanceInterface::OpAny,
			//[+]strus::PatternMatcherInstanceInterface::OpAnd
		};
		return ar[ (m_selopdist.random()-1)];
	}
	unsigned int randomArgc() const
	{
		unsigned int rt = m_argcdist.random();
		if (rt == 1 && RANDINT(1,5)>=2) ++rt;
		return rt;
	}
	unsigned int nofRules() const
	{
		return m_nofRules;
	}

private:
	unsigned int m_nofRules;
	strus::utils::ZipfDistribution m_featdist;
	strus::utils::ZipfDistribution m_rangedist;
	strus::utils::ZipfDistribution m_selopdist;
	strus::utils::ZipfDistribution m_argcdist;
};

static TreeNode* createRandomTree( const GlobalContext* ctx, const strus::utils::Document& doc, std::size_t& docitr, unsigned int depth=0)
{
	TreeNode* rt = 0;
	if (RANDINT( 1,5-depth) == 1)
	{
		if (docitr >= doc.itemar.size()) return 0;
		rt = new TreeNode( termId( strus::utils::Token, doc.itemar[ docitr].termid));
	}
	else
	{
		unsigned int argc = ctx->randomArgc();
		unsigned int range = argc + ctx->randomRange();
		unsigned int cardinality = 0; //... it seems currently too difficult to handle other than the default cardinality in this test
		TreeNode::JoinOperation op = ctx->randomOp();
		std::vector<TreeNode*> args;
		unsigned int rangesum = 0;
		unsigned int ai;
		for (ai=0; docitr < doc.itemar.size() && ai<argc; ++ai,++docitr)
		{
			TreeNode* arg = createRandomTree( ctx, doc, docitr, depth+1);
			if (arg == 0)
			{
				TreeNode::deleteTreeAr( args);
				return 0;
			}
			args.push_back( arg);
			rangesum += arg->range();
		}
		if (argc == 1
			&& (op == strus::PatternMatcherInstanceInterface::OpSequenceStruct
				|| op == strus::PatternMatcherInstanceInterface::OpWithinStruct)
			&& args[0]->term() == termId( strus::utils::SentenceDelim, 0))
		{
			// ... add another node for a sequence of size 1, if we have a sequence of length 1 with a delimiter element only (prevent anomaly)
			TreeNode* arg = createRandomTree( ctx, doc, docitr, depth+1);
			if (arg == 0)
			{
				TreeNode::deleteTreeAr( args);
				return 0;
			}
			args.push_back( arg);
			rangesum += arg->range();
		}
		range += rangesum;
		if (ai < argc)
		{
			TreeNode::deleteTreeAr( args);
			return 0;
		}
		switch (op)
		{
			case strus::PatternMatcherInstanceInterface::OpSequence:
			case strus::PatternMatcherInstanceInterface::OpSequenceImm:
			{
				break;
			}
			case strus::PatternMatcherInstanceInterface::OpSequenceStruct:
			{
				TreeNode* delim = new TreeNode( termId( strus::utils::SentenceDelim, 0));
				args.insert( args.begin(), delim);
				++argc;
				break;
			}
			case strus::PatternMatcherInstanceInterface::OpWithin:
			{
				for (int ii=0; ii<3; ii++)
				{
					unsigned int r1 = RANDINT(0,argc);
					unsigned int r2 = RANDINT(0,argc);
					if (r1 != r2) std::swap( args[r1], args[r2]);
				}
				break;
			}
			case strus::PatternMatcherInstanceInterface::OpWithinStruct:
			{
				for (int ii=0; ii<3; ii++)
				{
					unsigned int r1 = RANDINT(0,argc);
					unsigned int r2 = RANDINT(0,argc);
					if (r1 != r2) std::swap( args[r1], args[r2]);
				}
				TreeNode* delim = new TreeNode( termId( strus::utils::SentenceDelim, 0));
				args.insert( args.begin(), delim);
				++argc;
				break;
			}
			case strus::PatternMatcherInstanceInterface::OpAny:
				cardinality = 0;
				break;
			case strus::PatternMatcherInstanceInterface::OpAnd:
				throw std::runtime_error( "operator 'And' not implemented yet");
		}
		rt = new TreeNode( op, args, range, cardinality);
	}
	if (RANDINT(1,10) == 1)
	{
		rt->attachVariable( RANDINT(1,10));
	}
	return rt;
}

static std::vector<TreeNode*> createRandomTrees( const GlobalContext* ctx, const std::vector<strus::utils::Document>& docs)
{
	if (docs.empty()) return std::vector<TreeNode*>();
	std::vector<TreeNode*> rt;
	while (rt.size() < ctx->nofRules())
	{
		const strus::utils::Document& doc = docs[ rt.size() % docs.size()];
		std::size_t docitr = 0;
		TreeNode* tree = createRandomTree( ctx, doc, docitr);
		if (tree)
		{
			if (tree->op() == strus::PatternMatcherInstanceInterface::OpAny)
			{
				TreeNode::deleteTree( tree);
				continue;
			}
			if (tree->variable())
			{
				tree->removeVariable();
			}
			tree->setName( rt.size()+1);
			rt.push_back( tree);
		}
	}
	return rt;
}

typedef std::multimap<unsigned int,std::size_t> KeyTokenMap;
static void fillKeyTokens( KeyTokenMap& keytokenmap, TreeNode* tree, std::size_t treeidx)
{
	if (tree->term() && tree->args().empty())
	{
		keytokenmap.insert( KeyTokenMap::value_type( tree->term(), treeidx));
	}
	else switch (tree->op())
	{
		case strus::PatternMatcherInstanceInterface::OpSequence:
		case strus::PatternMatcherInstanceInterface::OpSequenceImm:
			fillKeyTokens( keytokenmap, tree->args()[ 0], treeidx);
			break;
		case strus::PatternMatcherInstanceInterface::OpSequenceStruct:
			fillKeyTokens( keytokenmap, tree->args()[ 1], treeidx);
			break;
		case strus::PatternMatcherInstanceInterface::OpWithin:
		{
			std::vector<TreeNode*>::const_iterator ti = tree->args().begin(), te = tree->args().end();
			for (; ti != te; ++ti)
			{
				fillKeyTokens( keytokenmap, *ti, treeidx);
			}
			break;
		}
		case strus::PatternMatcherInstanceInterface::OpWithinStruct:
		{
			std::vector<TreeNode*>::const_iterator ti = tree->args().begin(), te = tree->args().end();
			for (++ti; ti != te; ++ti)
			{
				fillKeyTokens( keytokenmap, *ti, treeidx);
			}
			break;
		}
		case strus::PatternMatcherInstanceInterface::OpAny:
		{
			std::vector<TreeNode*>::const_iterator ti = tree->args().begin(), te = tree->args().end();
			for (; ti != te; ++ti)
			{
				fillKeyTokens( keytokenmap, *ti, treeidx);
			}
			break;
		}
		case strus::PatternMatcherInstanceInterface::OpAnd:
			throw std::runtime_error( "operator 'And' not implemented yet");
	}
}

static void fillKeyTokens( KeyTokenMap& keytokenmap, std::vector<TreeNode*> treear)
{
	std::vector<TreeNode*>::const_iterator ti = treear.begin(), te = treear.end();
	for (std::size_t tidx=0; ti != te; ++ti,++tidx)
	{
		fillKeyTokens( keytokenmap, *ti, tidx);
	}
}

static void createExpression( strus::PatternMatcherInstanceInterface* ptinst, const GlobalContext* ctx, const TreeNode* tree)
{
	if (tree->term() && tree->args().empty())
	{
		ptinst->pushTerm( tree->term());
	}
	else
	{
		std::vector<TreeNode*>::const_iterator ti = tree->args().begin(), te = tree->args().end();
		for (; ti != te; ++ti)
		{
			createExpression( ptinst, ctx, *ti);
		}
		ptinst->pushExpression( tree->op(), tree->args().size(), tree->range(), tree->cardinality());
	}
	if (tree->variable())
	{
		ptinst->attachVariable( tree->variable());
	}
}

static void createRules( strus::PatternMatcherInstanceInterface* ptinst, const GlobalContext* ctx, const std::vector<TreeNode*> treear)
{
	std::vector<TreeNode*>::const_iterator ti = treear.begin(), te = treear.end();
	for (unsigned int tidx=0; ti != te; ++ti,++tidx)
	{
		createExpression( ptinst, ctx, *ti);
		ptinst->definePattern( (*ti)->name(), true);
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

static std::vector<strus::analyzer::PatternMatcherResult> processDocument( const strus::PatternMatcherInstanceInterface* ptinst, const strus::utils::Document& doc, std::map<std::string,double>& globalstats)
{
	strus::local_ptr<strus::PatternMatcherContextInterface> mt( ptinst->createContext());
	std::vector<strus::utils::DocumentItem>::const_iterator di = doc.itemar.begin(), de = doc.itemar.end();
	unsigned int didx = 0;
	for (; di != de; ++di,++didx)
	{
		mt->putInput( strus::analyzer::PatternLexem( di->termid, di->pos, 0/*origseg*/, didx, 1));
		if (g_errorBuffer->hasError()) throw std::runtime_error("error matching rules");
	}
	std::vector<strus::analyzer::PatternMatcherResult> results = mt->fetchResults();

	strus::analyzer::PatternMatcherStatistics stats = mt->getStatistics();
	std::vector<strus::analyzer::PatternMatcherStatistics::Item>::const_iterator
		li = stats.items().begin(), le = stats.items().end();
	for (; li != le; ++li)
	{
		globalstats[ li->name()] += li->value();
	}
	return results;
}

typedef std::vector<std::size_t> IndexTuple;
static std::vector<IndexTuple> getIndexPermurations( std::size_t beginidx, std::size_t endidx)
{
	std::vector<IndexTuple> rt;
	if (beginidx+1 == endidx)
	{
		IndexTuple elem;
		elem.push_back( beginidx);
		rt.push_back( elem);
		return rt;
	}
	std::vector<IndexTuple> restperm = getIndexPermurations( beginidx+1, endidx);
	std::vector<IndexTuple>::const_iterator pi = restperm.begin(), pe = restperm.end();
	for (; pi != pe; ++pi)
	{
		std::size_t tidx = 0;
		for (; tidx <= pi->size(); ++tidx)
		{
			IndexTuple rtelem = *pi;
			rtelem.insert( rtelem.begin() + tidx, beginidx);
			rt.push_back( rtelem);
		}
	}
	return rt;
}

struct TreeMatchResult
{
	bool valid;
	std::size_t startidx;
	std::size_t endidx;
	unsigned int ordpos;
	unsigned int ordsize;
	std::vector<strus::analyzer::PatternMatcherResultItem> itemar;

	TreeMatchResult()
		:valid(false),startidx(0),endidx(0),ordpos(0),ordsize(0){}
	TreeMatchResult( std::size_t startidx_, std::size_t endidx_, unsigned int ordpos_, unsigned int ordsize_)
		:valid(true),startidx(startidx_),endidx(endidx_),ordpos(ordpos_),ordsize(ordsize_){}
	TreeMatchResult( const TreeMatchResult& o)
		:valid(o.valid),startidx(o.startidx),endidx(o.endidx),ordpos(o.ordpos),ordsize(o.ordsize),itemar(o.itemar){}
	TreeMatchResult& operator=( const TreeMatchResult& o)
	{
		valid = o.valid;
		startidx = o.startidx;
		endidx = o.endidx;
		ordpos = o.ordpos;
		ordsize = o.ordsize;
		itemar = o.itemar;
		return *this;
	}

	void join( const TreeMatchResult& o)
	{
		if (!valid)
		{
			*this = o;
		}
		else if (o.valid)
		{
			unsigned int ordend = ordpos + ordsize;
			if (ordend < o.ordpos + o.ordsize)
			{
				ordend = o.ordpos + o.ordsize;
			}
			if (ordpos > o.ordpos)
			{
				ordpos = o.ordpos;
			}
			if (startidx > o.startidx)
			{
				startidx = o.startidx;
			}
			if (endidx < o.endidx)
			{
				endidx = o.endidx;
			}
			ordsize = ordend - ordpos;
			itemar.insert( itemar.end(), o.itemar.begin(), o.itemar.end());
		}
	}
};

static TreeMatchResult matchTree( const TreeNode* tree, const strus::utils::Document& doc, std::size_t didx, unsigned int endpos, unsigned int firstTerm)
{
	TreeMatchResult rt;
	if (tree->term() && tree->args().empty())
	{
		if (firstTerm)
		{
			if (didx >= doc.itemar.size()) return TreeMatchResult();
			if (doc.itemar[ didx].pos < endpos)
			{
				endpos = doc.itemar[ didx].pos;
			}
		}
		for (;didx < doc.itemar.size(); ++didx)
		{
			if (doc.itemar[ didx].pos > endpos) break;
			if (doc.itemar[ didx].termid == tree->term())
			{
				if (firstTerm && doc.itemar[ didx].termid != firstTerm) continue;

				rt = TreeMatchResult( didx, didx+1, doc.itemar[ didx].pos, 1);
				break;
			}
		}
	}
	else
	{
		switch (tree->op())
		{
			case strus::PatternMatcherInstanceInterface::OpSequenceImm:
				throw std::runtime_error("not implemented for test: OpSequenceStructImm");
			case strus::PatternMatcherInstanceInterface::OpSequenceStruct:
			{
				std::vector<TreeNode*>::const_iterator ai = tree->args().begin(), ae = tree->args().end();
				for (++ai; ai != ae; ++ai)
				{
					TreeMatchResult a_result = matchTree( *ai, doc, didx, endpos, firstTerm);
					if (!a_result.valid)
					{
						rt = TreeMatchResult();
						break;
					}
					if (a_result.ordpos + tree->range() < endpos)
					{
						endpos = a_result.ordpos + tree->range();
					}
					firstTerm = 0;
					rt.join( a_result);
					didx = rt.endidx;
					unsigned int nextpos = a_result.ordpos + a_result.ordsize;
					for (;didx < doc.itemar.size() && doc.itemar[ didx].pos < nextpos; ++didx){}
				}
				if (rt.valid)
				{
					if (rt.endidx < doc.itemar.size())
					{
						endpos = doc.itemar[ rt.endidx].pos;
					}
					TreeMatchResult delim = matchTree( tree->args()[0], doc, rt.startidx, rt.ordpos + rt.ordsize, 0);
					if (delim.valid && delim.endidx < rt.endidx) return TreeMatchResult();
				}
				break;
			}
			case strus::PatternMatcherInstanceInterface::OpSequence:
			{
				std::vector<TreeNode*>::const_iterator ai = tree->args().begin(), ae = tree->args().end();
				for (; ai != ae; ++ai)
				{
					TreeMatchResult a_result = matchTree( *ai, doc, didx, endpos, firstTerm);
					if (!a_result.valid)
					{
						rt = TreeMatchResult();
						break;
					}
					if (a_result.ordpos + tree->range() < endpos)
					{
						endpos = a_result.ordpos + tree->range();
					}
					firstTerm = 0;
					rt.join( a_result);
					didx = rt.endidx;
					unsigned int nextpos = a_result.ordpos + a_result.ordsize;
					for (;didx < doc.itemar.size() && doc.itemar[ didx].pos < nextpos; ++didx){}
				}
				break;
			}
			case strus::PatternMatcherInstanceInterface::OpWithin:
			case strus::PatternMatcherInstanceInterface::OpWithinStruct:
			{
				std::size_t aidx;
				strus::PatternMatcherInstanceInterface::JoinOperation seqop;
				if (tree->op() == strus::PatternMatcherInstanceInterface::OpWithin)
				{
					aidx = 0;
					seqop = strus::PatternMatcherInstanceInterface::OpSequence;
				}
				else
				{
					aidx = 1;
					seqop = strus::PatternMatcherInstanceInterface::OpSequenceStruct;
				}
				std::vector<IndexTuple> argperm = getIndexPermurations( aidx, tree->args().size());
				std::vector<IndexTuple>::const_iterator ai = argperm.begin(), ae = argperm.end();
				for (; ai != ae; ++ai)
				{
					std::vector<TreeNode*> pargs;
					if (tree->op() == strus::PatternMatcherInstanceInterface::OpWithinStruct)
					{
						pargs.push_back( tree->args()[0]);
					}
					IndexTuple::const_iterator xi = ai->begin(), xe = ai->end();
					for (; xi != xe; ++xi)
					{
						pargs.push_back( tree->args()[ *xi]);
					}
					TreeNode tree_alt( seqop, pargs, tree->range(), tree->cardinality());
					
					TreeMatchResult candidate = matchTree( &tree_alt, doc, didx, endpos, firstTerm);
					if (candidate.valid)
					{
						if (!rt.valid || candidate.endidx < rt.endidx)
						{
							rt = candidate;
						}
					}
				}
				break;
			}
			case strus::PatternMatcherInstanceInterface::OpAnd:
				throw std::runtime_error( "operator 'And' not implemented yet");
			case strus::PatternMatcherInstanceInterface::OpAny:
			{
				TreeMatchResult selected;
				std::vector<TreeNode*>::const_iterator ai = tree->args().begin(), ae = tree->args().end();
				for (; ai != ae && !rt.valid; ++ai)
				{
					TreeMatchResult candidate = matchTree( *ai, doc, didx, endpos, firstTerm);
					if (candidate.valid)
					{
						if (candidate.ordpos + tree->range() < endpos)
						{
							endpos = candidate.ordpos + tree->range();
						}
						if (selected.valid)
						{
							if (candidate.endidx < selected.endidx)
							{
								selected = candidate;
							}
						}
						else
						{
							selected = candidate;
						}
					}
				}
				rt.join( selected);
				break;
			}
		}
	}
	if (rt.valid && tree->variable())
	{
		strus::analyzer::PatternMatcherResultItem item( tree->variable(), rt.ordpos, rt.ordpos + rt.ordsize, 0/*start_origseg*/, rt.startidx, 0/*end_origseg*/, rt.endidx);
		rt.itemar.push_back( item);
	}
	return rt;
}


static std::vector<strus::analyzer::PatternMatcherResult>
	processDocumentAlt( const KeyTokenMap& keytokenmap, const std::vector<TreeNode*> treear, const strus::utils::Document& doc)
{
	std::vector<strus::analyzer::PatternMatcherResult> rt;
	std::vector<strus::utils::DocumentItem>::const_iterator di = doc.itemar.begin(), de = doc.itemar.end();
	for (std::size_t didx=0; di != de; ++di,++didx)
	{
		typedef std::pair<KeyTokenMap::const_iterator,KeyTokenMap::const_iterator> CandidateRange;
		CandidateRange candidateRange = keytokenmap.equal_range( di->termid);
		KeyTokenMap::const_iterator ri = candidateRange.first, re = candidateRange.second;
		std::pair<unsigned int,std::size_t> prevkey( 0,0);
		for (; ri != re; ++ri)
		{
			if (ri->first == prevkey.first && ri->second == prevkey.second) continue;
			prevkey.first = ri->first;
			prevkey.second = ri->second; //... eliminate dumplicates for redundant keytokens for rules

			const TreeNode* candidateTree = treear[ ri->second];
			unsigned int endpos = di->pos + candidateTree->range();
			TreeMatchResult match = matchTree( candidateTree, doc, didx, endpos, ri->first);
			if (match.valid)
			{
				strus::analyzer::PatternMatcherResult result( candidateTree->name(), match.ordpos, match.ordpos + match.ordsize, 0/*start_origseg*/, match.startidx, 0/*end_origseg*/, match.endidx, match.itemar);
				rt.push_back( result);
			}
		}
	}
	return rt;
}

static void printUsage( int argc, const char* argv[])
{
	std::cerr << "usage: " << argv[0] << " [<options>] <features> <nofdocs> <docsize> <nofpatterns>" << std::endl;
	std::cerr << "<options>= -h print this usage, -o do optimize automaton, -t <N> number of threads" << std::endl;
	std::cerr << "<features>= number of distinct features" << std::endl;
	std::cerr << "<nofdocs> = number of documents to insert" << std::endl;
	std::cerr << "<docsize> = size of a document" << std::endl;
	std::cerr << "<nofpatterns> = number of patterns to use" << std::endl;
}

bool compareResultItem( const strus::analyzer::PatternMatcherResultItem &res1, const strus::analyzer::PatternMatcherResultItem &res2)
{
	int cmp = std::strcmp( res1.name(), res2.name());
	if (cmp) return cmp < 0;
	if (res1.start_ordpos() != res2.start_ordpos()) return res1.start_ordpos() < res2.start_ordpos();
	if (res1.end_ordpos() != res2.end_ordpos()) return res1.end_ordpos() < res2.end_ordpos();
	if (res1.start_origseg() != res2.start_origseg()) return res1.start_origseg() < res2.start_origseg();
	if (res1.start_origpos() != res2.start_origpos()) return res1.start_origpos() < res2.start_origpos();
	if (res1.end_origseg() != res2.end_origseg()) return res1.end_origseg() < res2.end_origseg();
	if (res1.end_origpos() != res2.end_origpos()) return res1.end_origpos() < res2.end_origpos();
	return true;
}

bool compareResult( const strus::analyzer::PatternMatcherResult &res1, const strus::analyzer::PatternMatcherResult &res2)
{
	int cmp = std::strcmp( res1.name(), res2.name());
	if (cmp) return cmp < 0;
	if (res1.start_ordpos() != res2.start_ordpos()) return res1.start_ordpos() < res2.start_ordpos();
	if (res1.end_ordpos() != res2.end_ordpos()) return res1.end_ordpos() < res2.end_ordpos();
	if (res1.start_origseg() != res2.start_origseg()) return res1.start_origseg() < res2.start_origseg();
	if (res1.end_origseg() != res2.end_origseg()) return res1.end_origseg() < res2.end_origseg();
	if (res1.start_origpos() != res2.start_origpos()) return res1.start_origpos() < res2.start_origpos();
	if (res1.end_origpos() != res2.end_origpos()) return res1.end_origpos() < res2.end_origpos();
	if (res1.items().size() != res2.items().size()) return res1.items().size() < res2.items().size();
	std::vector<strus::analyzer::PatternMatcherResultItem>::const_iterator
		i1 = res1.items().begin(), e1 = res1.items().end(),
		i2 = res2.items().begin(), e2 = res2.items().end();
	for (; i1 != e1 && i2 != e2; ++i1,++i2)
	{
		if (!compareResultItem( *i1, *i2)) return false;
	}
	return true;
}

static std::vector<strus::analyzer::PatternMatcherResult>
	eliminateDuplicates( const std::vector<strus::analyzer::PatternMatcherResult>& results)
{
	std::vector<strus::analyzer::PatternMatcherResult> rt;
	std::vector<strus::analyzer::PatternMatcherResult>::const_iterator ri = results.begin(), re = results.end();
	const strus::analyzer::PatternMatcherResult* prev = 0;
	for (; ri != re; ++ri)
	{
		if (!prev || !compareResult( *ri, *prev)) rt.push_back( *ri);
		prev = &*ri;
	}
	return rt;
}

static std::vector<strus::analyzer::PatternMatcherResult>
	sortResults( const std::vector<strus::analyzer::PatternMatcherResult>& results)
{
	std::vector<strus::analyzer::PatternMatcherResult> rt;
	std::vector<strus::analyzer::PatternMatcherResult>::const_iterator ri = results.begin(), re = results.end();
	for (; ri != re; ++ri)
	{
		std::vector<strus::analyzer::PatternMatcherResultItem> items = ri->items();
		std::sort( items.begin(), items.end(), compareResultItem);
		rt.push_back( strus::analyzer::PatternMatcherResult( ri->name(), ri->start_ordpos(), ri->end_ordpos(), ri->start_origseg(), ri->start_origpos(), ri->end_origseg(), ri->end_origpos(), items));
	}
	std::sort( rt.begin(), rt.end(), compareResult);
	return rt;
}

//
// We have comparison differences mainly due to non determinism of random generated rules and the fact
// that strus pattern stops with the first match (non generating all possible solutions).
//
#ifdef STRUS_TEST_RANDOM_EXPRESSION_TREE_COMPARE_RES_EXACT
static bool compareResults( const std::vector<strus::analyzer::PatternMatcherResult>& results, const std::vector<strus::analyzer::PatternMatcherResult>& expectedResults)
{
	if (results.size() != expectedResults.size()) return false;
	std::vector<strus::analyzer::PatternMatcherResult>::const_iterator ri = results.begin(), re = results.end();
	std::vector<strus::analyzer::PatternMatcherResult>::const_iterator xi = expectedResults.begin(), xe = expectedResults.end();
	for (; xi != xe && ri != re; ++ri,++xi)
	{
		if (!compareResult( *ri, *xi)) return false;
	}
	return true;
}
#else
static bool compareResults( const std::vector<strus::analyzer::PatternMatcherResult>& results, const std::vector<strus::analyzer::PatternMatcherResult>& expectedResults)
{
	std::vector<strus::analyzer::PatternMatcherResult>::const_iterator ri = results.begin(), re = results.end();
	std::vector<strus::analyzer::PatternMatcherResult>::const_iterator xi = expectedResults.begin(), xe = expectedResults.end();
	std::set<std::string> set_res;
	std::set<std::string> set_exp;
	for (; xi != xe; ++xi)
	{
		std::ostringstream item;
		item << xi->name() << "_" << xi->start_ordpos() << ".." << xi->end_ordpos() << "(" << xi->start_origseg() << "|" << xi->start_origpos() << " .. " << xi->end_origseg() << "|" << xi->end_origpos() << ")";
		set_exp.insert( item.str());
	}
	for (; ri != re; ++ri)
	{
		std::ostringstream item;
		item << ri->name() << "_" << ri->start_ordpos() << ".." << ri->end_ordpos() << "(" << ri->start_origseg() << "|" << ri->start_origpos() << " .. " << ri->end_origseg() << "|" << ri->end_origpos() << ")";
		set_res.insert( item.str());
	}
	std::set<std::string>::const_iterator sri = set_res.begin(), sre = set_res.end();
	std::set<std::string>::const_iterator sxi = set_exp.begin(), sxe = set_exp.end();
	for (; sxi != sxe && sri != sre; ++sri,++sxi)
	{
		if (*sri != *sxi)
		{
			std::cerr << "first diff in result " << *sri << " != " << *sxi << std::endl;
			return false;
		}
	}
	return sxi == sxe && sri == sre;
}
#endif

static unsigned int processDocuments( const strus::PatternMatcherInstanceInterface* ptinst, const KeyTokenMap& keytokenmap, const std::vector<TreeNode*> treear, const std::vector<strus::utils::Document>& docs, std::map<std::string,double>& stats, const char* outputpath)
{
	unsigned int totalNofmatches = 0;
	std::vector<strus::utils::Document>::const_iterator di = docs.begin(), de = docs.end();
	std::size_t didx = 0;
	for (; di != de; ++di,++didx)
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "document " << di->tostring() << std::endl;
#endif
		std::vector<strus::analyzer::PatternMatcherResult>
			results = eliminateDuplicates( sortResults( processDocument( ptinst, *di, stats)));

		if (outputpath)
		{
			std::ostringstream out;
			out << "number of matches " << results.size() << std::endl;
			strus::utils::printResults( out, std::vector<strus::SegmenterPosition>(), results);

			std::string outputfile( outputpath);
			outputfile.push_back( strus::dirSeparator());
			outputfile.append( "res.txt");

			strus::writeFile( outputfile, out.str());
		}
		std::vector<strus::analyzer::PatternMatcherResult>
			expectedResults = eliminateDuplicates( sortResults( processDocumentAlt( keytokenmap, treear, *di)));

		if (outputpath)
		{
			std::ostringstream out;
			out << "number of matches " << expectedResults.size() << std::endl;
			strus::utils::printResults( out, std::vector<strus::SegmenterPosition>(), expectedResults);

			std::string outputfile( outputpath);
			outputfile.push_back( strus::dirSeparator());
			outputfile.append( "exp.txt");

			strus::writeFile( outputfile, out.str());
		}

		if (!compareResults( results, expectedResults))
		{
			throw std::runtime_error(std::string( "results differ to expected for document ") + di->id);
		}
		totalNofmatches += results.size();
		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error("error matching rule");
		}
	}
	return totalNofmatches;
}


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
		bool doOptimize = false;
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
				doOptimize = true;
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
		const char* outputpath = (argc - argidx > 4)? argv[ argidx+4] : 0;

		strus::local_ptr<strus::PatternMatcherInterface> pt( strus::createPatternMatcher_stream( g_errorBuffer));
		if (!pt.get()) throw std::runtime_error("failed to create pattern matcher");
		strus::local_ptr<strus::PatternMatcherInstanceInterface> ptinst( pt->createInstance());
		if (!ptinst.get()) throw std::runtime_error("failed to create pattern matcher instance");

		GlobalContext ctx( nofFeatures, nofPatterns);
		std::vector<strus::utils::Document> docs = createRandomDocuments( nofDocuments, documentSize, nofFeatures);
		std::vector<TreeNode*> treear = createRandomTrees( &ctx, docs);
		KeyTokenMap keyTokenMap;
		fillKeyTokens( keyTokenMap, treear);
		createRules( ptinst.get(), &ctx, treear);
		if (doOptimize)
		{
			ptinst->compile();
		}
		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error( "error creating automaton for evaluating rules");
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "patterns processed" << std::endl;
		std::vector<TreeNode*>::const_iterator ti = treear.begin(), te = treear.end();
		for (; ti != te; ++ti)
		{
			std::cout << (*ti)->tostring() << std::endl;
		}
#endif
		std::cerr << "starting rule evaluation ..." << std::endl;

		std::map<std::string,double> stats;
		unsigned int totalNofMatches = processDocuments( ptinst.get(), keyTokenMap, treear, docs, stats, outputpath);
		unsigned int totalNofDocs = docs.size();

		if (g_errorBuffer->hasError())
		{
			throw std::runtime_error("uncaugth exception");
		}
		std::cerr << "OK" << std::endl;
		std::cerr << "processed " << nofPatterns << " patterns on " << totalNofDocs << " documents with total " << totalNofMatches << " matches" << std::endl;
		std::cerr << "statistiscs:" << std::endl;
		std::map<std::string,double>::const_iterator gi = stats.begin(), ge = stats.end();
		for (; gi != ge; ++gi)
		{
			int value;
			if (gi->first == "nofTriggersAvgActive")
			{
				value = (int)(gi->second/totalNofDocs + 0.5);
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



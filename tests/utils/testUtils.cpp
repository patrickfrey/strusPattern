/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "testUtils.hpp"
#include "strus/analyzer/tokenPatternMatchResultItem.hpp"
#include <iostream>
#include <sstream>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <new>

#define RANDINT(MIN,MAX) ((std::rand()%(MAX-MIN))+MIN)

using namespace strus;
using namespace strus::utils;

std::string Document::tostring() const
{
	std::ostringstream outbuf;
	outbuf << "docid=" << id << ", items={";
	std::vector<DocumentItem>::const_iterator ti = itemar.begin(), te = itemar.end();
	for (int tidx=0; ti != te; ++ti,++tidx)
	{
		if (tidx) outbuf << ", ";
		outbuf << ti->pos << ":" << ti->termid;
	}
	outbuf << "}";
	return outbuf.str();
}

ZipfDistribution::ZipfDistribution( std::size_t size, double S)
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

unsigned int ZipfDistribution::random() const
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

unsigned int utils::termId( TermType tp, unsigned int no)
{
	return ((unsigned int)tp << 24) + no;
}

Document utils::createRandomDocument( unsigned int no, unsigned int size, unsigned int mod)
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

typedef strus::TokenPatternMatchInstanceInterface::JoinOperation JoinOperation;
JoinOperation utils::joinOperation( const char* joinopstr)
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

unsigned int utils::getUintValue( const char* arg)
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

void utils::printResults( std::ostream& out, const std::vector<strus::SegmenterPosition>& segmentposmap, const std::vector<strus::analyzer::TokenPatternMatchResult>& results, const char* src)
{
	std::vector<strus::analyzer::TokenPatternMatchResult>::const_iterator
		ri = results.begin(), re = results.end();
	for (; ri != re; ++ri)
	{
		std::size_t start_origsegsrcpos = segmentposmap.empty()?ri->start_origseg():segmentposmap[ ri->start_origseg()];
		std::size_t end_origsegsrcpos = segmentposmap.empty()?ri->end_origseg():segmentposmap[ ri->end_origseg()];

		out << "match '" << ri->name() << "' at " << ri->ordpos() << " ["
		    << start_origsegsrcpos << "|" << ri->start_origpos() << ".."
		    << end_origsegsrcpos << "|" << ri->end_origpos()
		    << "]:";
		std::vector<strus::analyzer::TokenPatternMatchResultItem>::const_iterator
			ei = ri->items().begin(), ee = ri->items().end();
	
		for (; ei != ee; ++ei)
		{
			start_origsegsrcpos = segmentposmap.empty()?ei->start_origseg():segmentposmap[ ei->start_origseg()];
			end_origsegsrcpos = segmentposmap.empty()?ei->end_origseg():segmentposmap[ ei->end_origseg()];

			out << " " << ei->name() << " [" << ei->ordpos()
					<< ", " << start_origsegsrcpos << "|" << ei->start_origpos()
					<< ".." << end_origsegsrcpos << "|" << ei->end_origpos()
					<< "]";
			if (src)
			{
				std::size_t start_srcpos = start_origsegsrcpos + ei->start_origpos();
				std::size_t end_srcpos = end_origsegsrcpos + ei->end_origpos();
				out << " '" << std::string( src+start_srcpos, end_srcpos-start_srcpos) << "'";
			}
		}
		out << std::endl;
	}
}

void utils::printStatistics( std::ostream& out, const strus::analyzer::TokenPatternMatchStatistics& stats)
{
	out << "Statistics:" << std::endl;
	std::vector<strus::analyzer::TokenPatternMatchStatistics::Item>::const_iterator
		gi = stats.items().begin(), ge = stats.items().end();
	for (; gi != ge; ++gi)
	{
		out << "\t" << gi->name() << ": " << floor( gi->value()+0.5) << std::endl;
	}
}


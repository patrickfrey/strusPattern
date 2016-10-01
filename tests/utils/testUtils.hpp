/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Some utility classes and funtions for the StrusStream tests
/// \file "testUtils.hpp"
#ifndef _STRUS_STREAM_TEST_UTILS_HPP_INCLUDED
#define _STRUS_STREAM_TEST_UTILS_HPP_INCLUDED
#include "strus/tokenPatternMatchInstanceInterface.hpp"
#include "strus/analyzer/tokenPatternMatchResult.hpp"
#include "strus/analyzer/tokenPatternMatchStatistics.hpp"
#include "strus/segmenterContextInterface.hpp"
#include <vector>
#include <string>
#include <iostream>

namespace strus {
namespace utils {

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

	std::string tostring() const;
};

class ZipfDistribution
{
public:
	explicit ZipfDistribution( std::size_t size, double S = 0.0);
	unsigned int random() const;

private:
	std::vector<double> m_ar;
};

enum TermType {Token, SentenceDelim};
unsigned int termId( TermType tp, unsigned int no);

Document createRandomDocument( unsigned int no, unsigned int size, unsigned int mod);

typedef strus::TokenPatternMatchInstanceInterface::JoinOperation JoinOperation;
JoinOperation joinOperation( const char* joinopstr);

unsigned int getUintValue( const char* arg);
void printResults( std::ostream& out, const std::vector<strus::SegmenterPosition>& segmentposmap, const std::vector<strus::analyzer::TokenPatternMatchResult>& results, const char* src=0);
void printStatistics( std::ostream& out, const strus::analyzer::TokenPatternMatchStatistics& stats);

}} //namespace
#endif



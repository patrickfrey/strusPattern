/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief StrusStream structure desribing a result of a token pattern matcher
/// \file "tokenPatternMatchResult.hpp"
#ifndef _STRUS_STREAM_TOKEN_PATTERN_MATCH_RESULT_HPP_INCLUDED
#define _STRUS_STREAM_TOKEN_PATTERN_MATCH_RESULT_HPP_INCLUDED
#include "strus/stream/tokenPatternMatchResultItem.hpp"
#include "strus/base/stdint.h"
#include <string>
#include <vector>

namespace strus {
namespace stream {

/// \brief Structure desribing a result of a token pattern matcher
class TokenPatternMatchResult
{
public:
	typedef TokenPatternMatchResultItem Item;

	/// \brief Constructor
	TokenPatternMatchResult( const char* name_, unsigned int ordpos_, uint16_t origseg_, uint16_t origpos_, const std::vector<Item>& itemlist_=std::vector<Item>())
		:m_name(name_),m_ordpos(ordpos_),m_origseg(origseg_),m_origpos(origpos_),m_itemlist(itemlist_){}
	/// \brief Copy constructor
	TokenPatternMatchResult( const TokenPatternMatchResult& o)
		:m_name(o.m_name),m_ordpos(o.m_ordpos),m_origseg(o.m_origseg),m_origpos(o.m_origpos),m_itemlist(o.m_itemlist){}
	/// \brief Destructor
	~TokenPatternMatchResult(){}

	/// \brief Name of the result, defined by the name of the pattern of the match
	const char* name() const			{return m_name;}
	/// \brief Ordinal (counting) position of the match (resp. the first term of the match)
	unsigned int ordpos() const			{return m_ordpos;}
	/// \brief Original segment index of the start of the result item in the source
	std::size_t origseg() const			{return m_origseg;}
	/// \brief Original byte position start of the result item in the source segment as UTF-8 specified with start_origseg
	std::size_t origpos() const			{return m_origpos;}
	/// \brief List of result items defined by variables assigned to nodes of the pattern of the match
	const std::vector<Item>& items() const		{return m_itemlist;}

private:
	const char* m_name;
	unsigned int m_ordpos;
	uint16_t m_origseg;
	uint16_t m_origpos;
	std::vector<Item> m_itemlist;
};


}} //namespace
#endif

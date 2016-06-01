/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure desribing a pattern match result
/// \file "patternMatchResult.hpp"
#ifndef _STRUS_STREAM_PATTERN_MATCH_RESULT_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_RESULT_HPP_INCLUDED
#include "strus/stream/patternMatchResultItem.hpp"
#include <string>
#include <vector>

namespace strus {
namespace stream {

/// \brief Structure desribing a pattern match result
class PatternMatchResult
{
public:
	typedef PatternMatchResultItem Item;

	/// \brief Constructor
	PatternMatchResult( const char* name_, const std::vector<Item>& itemlist_)
		:m_name(name_),m_itemlist(itemlist_){}
	/// \brief Copy constructor
	PatternMatchResult( const PatternMatchResult& o)
		:m_name(o.m_name),m_itemlist(o.m_itemlist){}
	/// \brief Destructor
	~PatternMatchResult(){}

	/// \brief Name of the result, defined by the name of the pattern of the match
	const char* name() const			{return m_name;}
	/// \brief List of result items
	const std::vector<Item>& itemlist() const	{return m_itemlist;}

private:
	const char* m_name;
	std::vector<Item> m_itemlist;
};


}} //namespace
#endif

/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for creating an automaton for detecting patterns in a document stream
/// \file "patternMatchResult.hpp"
#ifndef _STRUS_STREAM_PATTERN_MATCH_RESULT_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_RESULT_HPP_INCLUDED
#include "strus/stream/patternMatchResultItem.hpp"
#include <string>
#include <vector>

namespace strus {
namespace stream {

/// \brief Interface for creating an automaton for detecting patterns in a document stream
class PatternMatchResult
{
public:
	typedef PatternMatchResultItem Item;

	PatternMatchResult( const std::string& name_, const std::vector<Item>& itemlist_)
		:m_name(name_),m_itemlist(itemlist_){}
	PatternMatchResult( const PatternMatchResult& o)
		:m_name(o.m_name),m_itemlist(o.m_itemlist){}
	~PatternMatchResult(){}

	const std::string& name() const			{return m_name;}
	const std::vector<Item>& itemlist() const	{return m_itemlist;}

private:
	std::string m_name;
	std::vector<Item> m_itemlist;
};


}} //namespace
#endif

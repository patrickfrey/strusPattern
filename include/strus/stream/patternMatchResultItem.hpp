/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for creating an automaton for detecting patterns in a document stream
/// \file "patternMatchResult.hpp"
#ifndef _STRUS_STREAM_PATTERN_MATCH_RESULT_ITEM_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_RESULT_ITEM_HPP_INCLUDED
#include <string>

namespace strus {
namespace stream {

/// \brief Interface for creating an automaton for detecting patterns in a document stream
class PatternMatchResultItem
{
public:
	PatternMatchResultItem( const char* name_, unsigned int ordpos_, std::size_t origpos_, std::size_t origsize_)
		:m_name(name_),m_ordpos(ordpos_),m_origpos(origpos_),m_origsize(origsize_){}
	PatternMatchResultItem( const PatternMatchResultItem& o)
		:m_name(o.m_name),m_ordpos(o.m_ordpos),m_origpos(o.m_origpos),m_origsize(o.m_origsize){}
	~PatternMatchResultItem(){}

	const char* name() const			{return m_name;}
	unsigned int ordpos() const			{return m_ordpos;}
	std::size_t origpos() const			{return m_origpos;}
	std::size_t origsize() const			{return m_origsize;}

private:
	const char* m_name;
	unsigned int m_ordpos;
	std::size_t m_origpos;
	std::size_t m_origsize;
};


}} //namespace
#endif


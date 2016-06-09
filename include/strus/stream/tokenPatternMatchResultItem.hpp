/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief StrusStream structure desribing a result item of a token pattern matcher
/// \file "tokenPatternMatchResultItem.hpp"
#ifndef _STRUS_STREAM_TOKEN_PATTERN_MATCH_RESULT_ITEM_HPP_INCLUDED
#define _STRUS_STREAM_TOKEN_PATTERN_MATCH_RESULT_ITEM_HPP_INCLUDED
#include <string>

namespace strus {
namespace stream {

/// \brief Result item structure of a pattern match result
class TokenPatternMatchResultItem
{
public:
	/// \brief Constructor
	TokenPatternMatchResultItem( const char* name_, unsigned int ordpos_, std::size_t origpos_, std::size_t origsize_, float weight_)
		:m_name(name_),m_ordpos(ordpos_),m_origpos(origpos_),m_origsize(origsize_),m_weight(weight_){}
	/// \brief Copy constructor
	TokenPatternMatchResultItem( const TokenPatternMatchResultItem& o)
		:m_name(o.m_name),m_ordpos(o.m_ordpos),m_origpos(o.m_origpos),m_origsize(o.m_origsize),m_weight(o.m_weight){}
	/// \brief Destructor
	~TokenPatternMatchResultItem(){}

	/// \brief Name of the item, defined by the variable assigned to the match
	const char* name() const			{return m_name;}
	/// \brief Ordinal (counting) position of the match (resp. the first term of the match)
	unsigned int ordpos() const			{return m_ordpos;}
	/// \brief Original position of the match in the source
	std::size_t origpos() const			{return m_origpos;}
	/// \brief Original size of the match in the source
	std::size_t origsize() const			{return m_origsize;}
	/// \brief Weight of the item, defined by the weight of the variable assignment
	float weight() const				{return m_weight;}

private:
	const char* m_name;
	unsigned int m_ordpos;
	std::size_t m_origpos;
	std::size_t m_origsize;
	float m_weight;
};


}} //namespace
#endif


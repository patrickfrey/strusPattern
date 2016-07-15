/*
 * Copyright (c) 2016 Patrick P. Frey
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
#include "strus/base/stdint.h"

namespace strus {
namespace stream {

/// \brief Result item structure of a pattern match result
class TokenPatternMatchResultItem
{
public:
	/// \brief Constructor
	TokenPatternMatchResultItem( const char* name_, unsigned int ordpos_, uint16_t start_origseg_, uint16_t start_origpos_, uint16_t end_origseg_, uint16_t end_origpos_, float weight_)
		:m_name(name_),m_ordpos(ordpos_),m_start_origseg(start_origseg_),m_start_origpos(start_origpos_),m_end_origseg(end_origseg_),m_end_origpos(end_origpos_),m_weight(weight_){}
	/// \brief Copy constructor
	TokenPatternMatchResultItem( const TokenPatternMatchResultItem& o)
		:m_name(o.m_name),m_ordpos(o.m_ordpos),m_start_origseg(o.m_start_origseg),m_start_origpos(o.m_start_origpos),m_end_origseg(o.m_end_origseg),m_end_origpos(o.m_end_origpos),m_weight(o.m_weight){}
	/// \brief Destructor
	~TokenPatternMatchResultItem(){}

	/// \brief Name of the item, defined by the variable assigned to the match
	const char* name() const			{return m_name;}
	/// \brief Ordinal (counting) position of the match (resp. the first term of the match)
	unsigned int ordpos() const			{return m_ordpos;}
	/// \brief Original segment index of the start of the result item in the source
	std::size_t start_origseg() const		{return m_start_origseg;}
	/// \brief Original byte position start of the result item in the source segment as UTF-8 specified with start_origseg
	std::size_t start_origpos() const		{return m_start_origpos;}
	/// \brief Original segment index of the end of the result item in the source
	std::size_t end_origseg() const			{return m_end_origseg;}
	/// \brief Original byte position end of the result item in the source segment as UTF-8 specified with start_origseg
	std::size_t end_origpos() const			{return m_end_origpos;}
	/// \brief Weight of the item, defined by the weight of the variable assignment
	float weight() const				{return m_weight;}

private:
	const char* m_name;
	unsigned int m_ordpos;
	uint16_t m_start_origseg;
	uint16_t m_start_origpos;
	uint16_t m_end_origseg;
	uint16_t m_end_origpos;
	float m_weight;
};


}} //namespace
#endif


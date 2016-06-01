/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure describing a token in the source of pattern matching
/// \file "patternMatchToken.hpp"
#ifndef _STRUS_STREAM_PATTERN_MATCH_TOKEN_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_TOKEN_HPP_INCLUDED
#include <cstddef>

namespace strus {
namespace stream {

/// \brief Structure describing a token in the source of pattern matching
class PatternMatchToken
{
public:
	/// \brief Constructor
	PatternMatchToken( unsigned int ordpos_, std::size_t origpos_, std::size_t origsize_)
		:m_ordpos(ordpos_),m_origpos(origpos_),m_origsize(origsize_){}
	/// \brief Copy constructor
	PatternMatchToken( const PatternMatchToken& o)
		:m_ordpos(o.m_ordpos),m_origpos(o.m_origpos),m_origsize(o.m_origsize){}
	/// \brief Destructor
	~PatternMatchToken(){}

	/// \brief Ordinal (counting) position assigned to the token
	unsigned int ordpos() const			{return m_ordpos;}
	/// \brief Original position of the token in the source
	std::size_t origpos() const			{return m_origpos;}
	/// \brief Original size of the token in the source
	std::size_t origsize() const			{return m_origsize;}

private:
	unsigned int m_ordpos;
	std::size_t m_origpos;
	std::size_t m_origsize;
};


}} //namespace
#endif


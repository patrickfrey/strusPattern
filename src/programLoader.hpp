/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief StrusStream program loader for loading pattern definitions from source
/// \file "programLoader.hpp"
#ifndef _STRUS_STREAM_PATTERN_MATCH_PROGRAM_LOADER_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_PROGRAM_LOADER_HPP_INCLUDED
#include "utils.hpp"
#include <cstddef>

namespace strus {

/// \brief StrusStream program loader for loading pattern definitions from source
class PatternMatchProgramLoader
{
public:
	/// \brief Constructor
	PatternMatchProgramLoader(){}
	/// \brief Copy constructor
	PatternMatchProgramLoader( const PatternMatchProgramLoader& o)
		:m_id(o.m_id),m_ordpos(o.m_ordpos),m_origpos(o.m_origpos),m_origsize(o.m_origsize){}
	/// \brief Destructor
	~PatternMatchProgramLoader(){}

	/// \brief Internal identifier of the term
	unsigned int id() const				{return m_id;}
	/// \brief Ordinal (counting) position assigned to the token
	unsigned int ordpos() const			{return m_ordpos;}
	/// \brief Original position of the token in the source
	std::size_t origpos() const			{return m_origpos;}
	/// \brief Original size of the token in the source
	std::size_t origsize() const			{return m_origsize;}

private:
	utils::SharedPtr<TokenPatternMatchInstanceInterface> m_tokenPatternMatch;
	utils::SharedPtr<TokenPatternMatchInstanceInterface> m_tokenPatternMatch;
};


}} //namespace
#endif


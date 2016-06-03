/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for detecting terms defined as regular expressions
/// \file "streamTermMatchContextInterface.hpp"
#ifndef _STRUS_STREAM_TERM_MATCH_CONTEXT_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_TERM_MATCH_CONTEXT_INTERFACE_HPP_INCLUDED
#include "strus/stream/patternMatchToken.hpp"
#include "strus/stream/patternMatchTerm.hpp"
#include <vector>

namespace strus
{

/// \brief Interface for building the automaton for detecting terms defined as regular expressions
class StreamTermMatchContextInterface
{
public:
	/// \brief Destructor
	virtual ~StreamTermMatchContextInterface(){}

	/// \brief Do process a list of tokens and return a list of labeled terms found that matched
	/// \param[in] tokens tokens to match against
	/// \param[in] src pointer to source of the tokens to match against
	virtual std::vector<stream::PatternMatchTerm> match( const std::vector<stream::PatternMatchToken>& tokens, const char* src)=0;
};

} //namespace
#endif



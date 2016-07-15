/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for detecting patterns (structures formed by tokens) in one document
/// \file "tokenPatternMatchContextInterface.hpp"
#ifndef _STRUS_STREAM_TOKEN_PATTERN_MATCH_CONTEXT_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_TOKEN_PATTERN_MATCH_CONTEXT_INTERFACE_HPP_INCLUDED
#include "strus/stream/tokenPatternMatchResult.hpp"
#include "strus/stream/patternMatchToken.hpp"
#include "strus/stream/tokenPatternMatchStatistics.hpp"
#include <vector>

namespace strus
{

/// \brief Interface for detecting patterns (structures formed by tokens) in one document
class TokenPatternMatchContextInterface
{
public:
	/// \brief Destructor
	virtual ~TokenPatternMatchContextInterface(){}

	/// \brief Feed the next input token
	/// \param[in] token the token to feed
	/// \remark The input terms must be fed in ascending order of 'ordpos'
	virtual void putInput( const stream::PatternMatchToken& token)=0;

	/// \brief Get the list of matches detected in the current document
	/// \return the list of matches
	virtual std::vector<stream::TokenPatternMatchResult> fetchResults() const=0;

	/// \brief Get the statistics for global analysis
	/// \return the statistics data gathered during processing
	virtual stream::TokenPatternMatchStatistics getStatistics() const=0;
};

} //namespace
#endif


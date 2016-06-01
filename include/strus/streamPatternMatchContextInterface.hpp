/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STREAM_PATTERN_MATCH_CONTEXT_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_CONTEXT_INTERFACE_HPP_INCLUDED
#include "strus/stream/patternMatchResult.hpp"
#include "strus/stream/patternMatchTerm.hpp"
#include "strus/stream/patternMatchStatistics.hpp"
#include <vector>

namespace strus
{

/// \brief Interface for detecting patterns in one document
class StreamPatternMatchContextInterface
{
public:
	/// \brief Destructor
	virtual ~StreamPatternMatchContextInterface(){}

	/// \brief Feed the next input term
	/// \brief term term to feed
	/// \remark The input terms must be fed in ascending order of 'ordpos'
	virtual void putInput( const stream::PatternMatchTerm& term)=0;

	/// \brief Get the list of matches detected in the current document
	/// \return the list of matches
	virtual std::vector<stream::PatternMatchResult> fetchResults() const=0;

	/// \brief Get the statistics for global pattern matching analysis
	/// \return the statistics data gathered during processing
	virtual stream::PatternMatchStatistics getStatistics() const=0;
};

} //namespace
#endif


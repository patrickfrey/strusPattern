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
#include <vector>

namespace strus
{

/// \brief Interface for detecting patterns in a document
class StreamPatternMatchContextInterface
{
public:
	/// \brief Destructor
	virtual ~StreamPatternMatchContextInterface(){}

	/// \brief Get the internal numeric identifier of the term with the given name or 0 if not defined
	/// \param[in] type type name of the term
	/// \param[in] value value string of the term
	/// \return term identifier or 0, if not defined
	virtual unsigned int termId( const std::string& type, const std::string& value) const=0;

	virtual void putInput( unsigned int termid, unsigned int ordpos, unsigned int origpos, unsigned int origsize)=0;

	virtual std::vector<stream::PatternMatchResult> fetchResults() const=0;

	struct Statistics
	{
		unsigned int nofPatternsTriggered;
		double nofOpenPatterns;

		Statistics()
			:nofPatternsTriggered(0),nofOpenPatterns(0){}
		Statistics( const Statistics& o)
			:nofPatternsTriggered(o.nofPatternsTriggered),nofOpenPatterns(o.nofOpenPatterns){}
	};

	virtual void getStatistics( Statistics& stats) const=0;
};

} //namespace
#endif


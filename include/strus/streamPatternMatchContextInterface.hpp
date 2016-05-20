/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STREAM_PATTERN_MATCH_CONTEXT_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_CONTEXT_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Interface for detecting patterns in a document
class StreamPatternMatchContextInterface
{
public:
	virtual ~StreamPatternMatchContextInterface(){}

	virtual void feedTerm( unsigned int termid, unsigned int ordpos, unsigned int bytepos, unsigned int bytesize)=0;

	
};

//namespace
#endif

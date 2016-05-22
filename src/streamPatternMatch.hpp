/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of an automaton for detecting patterns in a document stream
/// \file "streamPatternMatch.hpp"
#ifndef _STRUS_STREAM_PATTERN_MATCH_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_IMPLEMENTATION_HPP_INCLUDED
#include "strus/streamPatternMatchInterface.hpp"

namespace strus
{
/// \brief Forward declaration
class StreamPatternMatchInstanceInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Implementation of an automaton builder for detecting patterns in a document stream
class StreamPatternMatch
	:public StreamPatternMatchInterface
{
public:
	explicit StreamPatternMatch( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~StreamPatternMatch(){}

	virtual StreamPatternMatchInstanceInterface* createInstance() const;

private:
	ErrorBufferInterface* m_errorhnd;
};

} //namespace
#endif

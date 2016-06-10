/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of an automaton builder for detecting patterns of tokens in a document stream
/// \file "tokenPatternMatch.hpp"
#ifndef _STRUS_STREAM_PATTERN_MATCH_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_IMPLEMENTATION_HPP_INCLUDED
#include "strus/tokenPatternMatchInterface.hpp"

namespace strus
{
/// \brief Forward declaration
class TokenPatternMatchInstanceInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Implementation of an automaton builder for detecting patterns of tokens in a document stream
class TokenPatternMatch
	:public TokenPatternMatchInterface
{
public:
	explicit TokenPatternMatch( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~TokenPatternMatch(){}

	virtual TokenPatternMatchInstanceInterface* createInstance() const;

private:
	ErrorBufferInterface* m_errorhnd;
};

} //namespace
#endif

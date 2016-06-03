/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of detecting terms defined as regular expressions
/// \file "streamTermMatch.hpp"
#ifndef _STRUS_STREAM_TERM_MATCH_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STREAM_TERM_MATCH_IMPLEMENTATION_HPP_INCLUDED
#include "strus/streamTermMatchInterface.hpp"

namespace strus {

///\brief Forward declaration
class ErrorBufferInterface;

/// \brief Interface for creating an automaton for detecting terms defined as regular expressions
class StreamTermMatch
	:public StreamTermMatchInterface
{
public:
	explicit StreamTermMatch( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	/// \brief Destructor
	virtual ~StreamTermMatch(){}

	/// \brief Create an instance to build the regular expressions for a term matcher
	/// \return the term matcher instance
	virtual StreamTermMatchInstanceInterface* createInstance() const;

private:
	ErrorBufferInterface* m_errorhnd;
};

}//namespace
#endif




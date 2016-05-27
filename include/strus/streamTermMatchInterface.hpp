/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for creating an automaton for detecting terms defined as regular expressions
/// \file "streamTermMatchInterface.hpp"
#ifndef _STRUS_STREAM_TERM_MATCH_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_TERM_MATCH_INTERFACE_HPP_INCLUDED

namespace strus
{
/// \brief Forward declaration
class StreamTermMatchInstanceInterface;

/// \brief Interface for creating an automaton for detecting terms defined as regular expressions
class StreamTermMatchInterface
{
public:
	/// \brief Destructor
	virtual ~StreamTermMatchInterface(){}

	/// \brief Create an instance to build the regular expressions for a term matcher
	/// \return the term matcher instance
	virtual StreamTermMatchInstanceInterface* createInstance() const=0;
};

} //namespace
#endif


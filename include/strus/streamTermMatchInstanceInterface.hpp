/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for building the automaton for detecting terms defined as regular expressions
/// \file "streamTermMatchInstanceInterface.hpp"
#ifndef _STRUS_STREAM_TERM_MATCH_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_TERM_MATCH_INSTANCE_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Forward declaration
class StreamTermMatchContextInterface;

/// \brief Interface for building the automaton for detecting terms defined as regular expressions
class StreamTermMatchInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~StreamTermMatchInstanceInterface(){}

	/// \brief Create the context to process a chunk of text with the text matcher
	/// \return the term matcher context
	/// \remark The context cannot be reset. So the context has to be recreated for every processed unit (document)
	virtual StreamTermMatchContextInterface* createContext() const=0;
};

} //namespace
#endif


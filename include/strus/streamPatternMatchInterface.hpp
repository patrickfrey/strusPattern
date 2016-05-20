/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for creating an automaton for detecting patterns in a document stream
/// \file "streamPatternMatchInterface.hpp"
#ifndef _STRUS_STREAM_PATTERN_MATCH_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_INTERFACE_HPP_INCLUDED

namespace strus
{
/// \brief Forward declaration
class StreamPatternMatchInstanceInterface;

/// \brief Interface for creating an automaton for detecting patterns in a document stream
class StreamPatternMatchInterface
{
public:
	virtual ~StreamPatternMatchInterface(){}

	virtual StreamPatternMatchInstanceInterface* createInstance() const;
};

//namespace
#endif


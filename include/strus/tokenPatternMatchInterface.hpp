/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for creating an automaton for detecting patterns of tokens in a document stream
/// \file "tokenPatternMatchInterface.hpp"
#ifndef _STRUS_STREAM_TOKEN_PATTERN_MATCH_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_TOKEN_PATTERN_MATCH_INTERFACE_HPP_INCLUDED
#include <vector>
#include <string>

namespace strus
{
/// \brief Forward declaration
class TokenPatternMatchInstanceInterface;

/// \brief Interface for creating an automaton for detecting patterns of tokens in a document stream
class TokenPatternMatchInterface
{
public:
	/// \brief Destructor
	virtual ~TokenPatternMatchInterface(){}

	/// \brief Get the list of option names you can pass to TokenPatternMatchInstanceInterface::compile
	/// \return NULL terminated array of strings
	virtual std::vector<std::string> getCompileOptions() const=0;

	/// \brief Create an instance to build the rules of a pattern matcher
	/// \return the pattern matcher instance
	virtual TokenPatternMatchInstanceInterface* createInstance() const=0;
};

} //namespace
#endif


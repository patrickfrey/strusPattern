/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief StrusStream interface for creating an automaton for detecting tokens defined as regular expressions in text
/// \file "charRegexMatchInterface.hpp"
#ifndef _STRUS_STREAM_CHAR_REGEX_MATCH_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_CHAR_REGEX_MATCH_INTERFACE_HPP_INCLUDED
#include <vector>
#include <string>

namespace strus
{
/// \brief Forward declaration
class CharRegexMatchInstanceInterface;

/// \brief Interface for creating an automaton for detecting tokens defined as regular expressions in text
class CharRegexMatchInterface
{
public:
	/// \brief Destructor
	virtual ~CharRegexMatchInterface(){}

	/// \brief Get the list of option names you can pass to CharRegexMatchInstanceInterface::compile
	/// \return NULL terminated array of strings
	virtual std::vector<std::string> getCompileOptions() const=0;

	/// \brief Create an instance to build the regular expressions for a term matcher
	/// \return the term matcher instance
	virtual CharRegexMatchInstanceInterface* createInstance() const=0;
};

} //namespace
#endif


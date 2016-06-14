/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief StrusStream interface to load pattern definitions from source
/// \file "patternMatchProgramInterface.hpp"
#ifndef _STRUS_STREAM_PATTERN_MATCH_PROGRAM_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_PROGRAM_INTERFACE_HPP_INCLUDED

namespace strus {

/// \brief Forward declaration
class PatternMatchProgramInstanceInterface;
/// \brief Forward declaration
class TokenPatternMatchInterface;
/// \brief Forward declaration
class CharRegexMatchInterface;


/// \brief StrusStream interface to load pattern definitions from source
class PatternMatchProgramInterface
{
public:
	/// \brief Destructor
	virtual ~PatternMatchProgramInterface(){}

	/// \brief Create an instance to load the rules of a pattern matcher from source
	/// \return the pattern matcher program instance
	virtual PatternMatchProgramInstanceInterface* createInstance() const=0;
};

} //namespace
#endif


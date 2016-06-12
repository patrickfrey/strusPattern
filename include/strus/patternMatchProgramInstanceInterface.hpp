/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief StrusStream interface to instance loading pattern definitions from source
/// \file "patternMatchProgramInstanceInterface.hpp"
#ifndef _STRUS_STREAM_PATTERN_MATCH_PROGRAM_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_PROGRAM_INSTANCE_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus {

/// \brief StrusStream interface to load pattern definitions from source
class PatternMatchProgramInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~PatternMatchProgramInstanceInterface(){}

	/// \brief Load the rules of a pattern matcher from a source file
	/// \return true on success, false on failure
	virtual bool load( const std::string& source)=0;

	/// \brief Check for unresolved symbols and compile the automatons defined by the sources loaded
	/// \return true on success, false on failure
	virtual bool compile()=0;
};

} //namespace
#endif


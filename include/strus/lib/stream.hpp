/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the strus stream library implementing pattern matching of rules built from document terms
/// \file error.hpp
#ifndef _STRUS_STREAM_LIB_HPP_INCLUDED
#define _STRUS_STREAM_LIB_HPP_INCLUDED
#include <cstdio>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class CharRegexMatchInterface;
/// \brief Forward declaration
class TokenPatternMatchInterface;
/// \brief Forward declaration
class PatternMatchProgramInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Create the interface for regular expression matching on text
CharRegexMatchInterface* createCharRegexMatch_standard(
		ErrorBufferInterface* errorhnd);

/// \brief Create the interface for pattern matching on a stream of tokens
TokenPatternMatchInterface* createTokenPatternMatch_standard(
		ErrorBufferInterface* errorhnd);

/// \brief Create the interface for loading programs from source that define patterns to match
PatternMatchProgramInterface* createPatternMatchProgram_standard(
		const TokenPatternMatchInterface* tpm,
		const CharRegexMatchInterface* crm,
		ErrorBufferInterface* errorhnd);

}//namespace
#endif


/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the strus standard pattern matching library with a lexer based on hyperscan
/// \file pattern.hpp
#ifndef _STRUS_PATTERN_LIB_HPP_INCLUDED
#define _STRUS_PATTERN_LIB_HPP_INCLUDED
#include <cstdio>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class PatternLexerInterface;
/// \brief Forward declaration
class PatternMatcherInterface;
/// \brief Forward declaration
class TokenMarkupInstanceInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Create the interface for regular expression matching on text based on hyperscan
PatternLexerInterface* createPatternLexer_std(
		ErrorBufferInterface* errorhnd);

/// \brief Create the interface for pattern matching on a regular language with tokens as alphabet
PatternMatcherInterface* createPatternMatcher_std(
		ErrorBufferInterface* errorhnd);

}//namespace
#endif


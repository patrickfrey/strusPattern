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
class PatternLexerInterface;
/// \brief Forward declaration
class PatternMatcherInterface;
/// \brief Forward declaration
class TokenMarkupInstanceInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Create the interface for regular expression matching on text
PatternLexerInterface* createPatternLexer_stream(
		ErrorBufferInterface* errorhnd);

/// \brief Create the interface for pattern matching on a stream of tokens
PatternMatcherInterface* createPatternMatcher_stream(
		ErrorBufferInterface* errorhnd);

}//namespace
#endif


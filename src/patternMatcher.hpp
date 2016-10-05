/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of a pattern matcher detecting patterns of tokens in a document stream
/// \file "patternMatcher.hpp"
#ifndef _STRUS_PATTERN_MATCHER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_PATTERN_MATCHER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/patternMatcherInterface.hpp"

namespace strus
{
/// \brief Forward declaration
class PatternMatcherInstanceInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Implementation of an automaton builder for detecting patterns of tokens in a document stream
class PatternMatcher
	:public PatternMatcherInterface
{
public:
	explicit PatternMatcher( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PatternMatcher(){}

	virtual std::vector<std::string> getCompileOptions() const;
	virtual PatternMatcherInstanceInterface* createInstance() const;

	virtual const char* getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;
};

} //namespace
#endif

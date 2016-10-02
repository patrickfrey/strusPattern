/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of detecting tokens defined as regular expressions on text
/// \file "patternLexer.hpp"
#ifndef _STRUS_STREAM_PATTERN_LEXER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_LEXER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/patternLexerInterface.hpp"

namespace strus {

///\brief Forward declaration
class ErrorBufferInterface;

/// \brief Object for creating an automaton for detecting tokens defined as regular expressions in text
/// \note Based on the Intel hyperscan library as backend.
class PatternLexer
	:public PatternLexerInterface
{
public:
	explicit PatternLexer( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~PatternLexer(){}

	virtual std::vector<std::string> getCompileOptions() const;
	virtual PatternLexerInstanceInterface* createInstance() const;

private:
	ErrorBufferInterface* m_errorhnd;
};

}//namespace
#endif




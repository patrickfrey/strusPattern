/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\file "regexExpressions.hpp"
///\brief Some functions to handle regex expressions
#ifndef _STRUS_STREAM_REGEX_EXPRESSIONS_HPP_INCLUDED
#define _STRUS_STREAM_REGEX_EXPRESSIONS_HPP_INCLUDED
#include <string>
#include <vector>

namespace strus
{

bool isRegularExpression( const std::string& src);

std::vector<std::string> getRegexPrefixes( const std::string& expr);
std::vector<std::string> getRegexSuffixes( const std::string& expr);

}
#endif


/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Result format string printer
/// \file "resultFormat.hpp"
#ifndef _STRUS_PATTERN_RESULT_FORMAT_HPP_INCLUDED
#define _STRUS_PATTERN_RESULT_FORMAT_HPP_INCLUDED
#include "strus/patternMatcherInterface.hpp"
#include "strus/analyzer/patternMatcherResultItem.hpp"

namespace strus
{

struct ResultFormatElement
{
	enum Op {Variable,String};
	Op op;
	const char* value;
	const char* separator;
};

class ResultFormatContext
{
public:
	ResultFormatContext(){}

	const char* map( const ResultFormatElement* fmt, std::size_t nofItems, const PatternMatcherResultItem* items);

private:
	struct Impl;
	Impl* m_impl;
};

class ResultFormatTable
{
public:
	ResultFormatTable(){}

	/// \brief Create a format string representation out of its source
	/// \note Substituted elements are addressed as identifiers in curly brackets '{' '}'
	/// \note Escaping of '{' and '}' is done with backslash '\', e.g. "\{" or "\}"
	/// \return a {Variable,NULL} terminated array of elements
	const ResultFormatElement* createFormat( const char* src);

private:
	struct Impl;
	Impl* m_impl;
};

}//namespace
#endif

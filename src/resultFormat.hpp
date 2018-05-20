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

/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class DebugTraceContextInterface;

typedef struct ResultFormat ResultFormat;

class ResultFormatContext
{
public:
	explicit ResultFormatContext( ErrorBufferInterface* errorhnd_);
	~ResultFormatContext();

	/// \brief Map a result to a string
	const char* map( const ResultFormat* fmt, std::size_t nofItems, const analyzer::PatternMatcherResultItem* items);

private:
	ErrorBufferInterface* m_errorhnd;
	DebugTraceContextInterface* m_debugtrace;
	struct Impl;
	Impl* m_impl;
};

class ResultFormatVariableMap
{
public:
	virtual ~ResultFormatVariableMap(){}
	virtual const char* getVariable( const std::string& name)=0;
};

class ResultFormatTable
{
public:
	ResultFormatTable( ErrorBufferInterface* errorhnd_, ResultFormatVariableMap* variableMap_);
	~ResultFormatTable();

	/// \brief Create a format string representation out of its source
	/// \note Substituted elements are addressed as identifiers in curly brackets '{' '}'
	/// \note Escaping of '{' and '}' is done with backslash '\', e.g. "\{" or "\}"
	/// \return a {Variable,NULL} terminated array of elements
	const ResultFormat* createResultFormat( const char* src);

private:
	ErrorBufferInterface* m_errorhnd;
	ResultFormatVariableMap* m_variableMap;
	struct Impl;
	Impl* m_impl;
};

struct ResultChunk
{
	const char* value;
	std::size_t valuesize;
	int start_seg;
	int start_pos;
	int end_seg;
	int end_pos;

	static bool parseNext( ResultChunk& result, char const*& src);
};

}//namespace
#endif

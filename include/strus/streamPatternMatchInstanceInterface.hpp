/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for building the automaton for detecting patterns in a document stream
/// \file "streamPatternMatchInstanceInterface.hpp"
#ifndef _STRUS_STREAM_PATTERN_MATCH_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_INSTANCE_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Forward declaration
class StreamPatternMatchContextInterface;

/// \brief Interface for building the automaton for detecting patterns in a document stream
class StreamPatternMatchInstanceInterface
{
public:
	virtual ~StreamPatternMatchInstanceInterface(){}

	virtual void declareTerm( const std::string& name, unsigned int termid)=0;

	virtual void pushTerm( unsigned int termid)=0;

	virtual void pushExpression(
			const char* operation,
			std::size_t argc, int range, unsigned int cardinality)=0;

	/// \brief Attaches a variable to the top expression or term on the stack.
	/// \param[in] name_ name of the variable attached
	/// \remark The stack is not changed
	virtual void attachVariable( const std::string& name_)=0;

	virtual void closePattern( const std::string& name_);

	virtual StreamPatternMatchContextInterface* createContext() const;
};

//namespace
#endif


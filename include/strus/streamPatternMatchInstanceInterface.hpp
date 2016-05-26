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
	/// \brief Destructor
	virtual ~StreamPatternMatchInstanceInterface(){}

	/// \brief Get or create a numeric identifier for name that can be used for pushTerm(unsigned int)
	/// \param[in] type type name of the term
	/// \param[in] value value string of the term
	/// \return the numeric identifier assigned to the term specified
	virtual unsigned int getTermId( const std::string& type, const std::string& value)=0;

	/// \brief Push a term on the stack
	/// \param[in] termid term identifier
	virtual void pushTerm( unsigned int termid)=0;

	/// \brief Take the topmost elements from the stack, build an expression out of them and replace the argument elements with the created element on the stack
	/// \param[in] operation identifier of the operation to perform as string
	/// \param[in] argc number of arguments of this operation
	/// \param[in] range position proximity range of the expression
	/// \param[in] cardinality required size of matching results (e.g. minimum number of elements of any input subset selection that builds a result) (0 for use default)
	/// \note The operation identifiers should if possible correspond to the names used for the standard posting join operators in the strus core query evaluation
	virtual void pushExpression(
			const char* operation,
			std::size_t argc, unsigned int range, unsigned int cardinality)=0;

	/// \brief Push a reference to a pattern on the stack
	/// \param[in] name name of the referenced pattern
	virtual void pushPattern( const std::string& name)=0;

	/// \brief Attaches a variable to the top expression or term on the stack.
	/// \param[in] name name of the variable attached
	/// \param[in] weight of the variable attached
	/// \remark The stack is not changed
	/// \note The weight attached is not used by the pattern matching itself. It is just forwarded to the result produced with this variable assignment.
	virtual void attachVariable( const std::string& name, float weight)=0;

	/// \brief Create a pattern that can be referenced by the given name and can be declared as part of the result
	/// \param[in] name name of the pattern and the result if declared as visible
	/// \param[in] visible true, if the pattern result should be exported (be visible in the final result)
	virtual void closePattern( const std::string& name, bool visible)=0;

	/// \brief Create the context to process a document with the pattern matcher
	/// \return the pattern matcher context
	/// \remark The context cannot be reset. So the context has to be recreated for every processed unit (document)
	virtual StreamPatternMatchContextInterface* createContext() const=0;
};

} //namespace
#endif


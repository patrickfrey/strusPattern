/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for building the automaton for detecting patterns of tokens in a document stream
/// \file "tokenPatternMatchInstanceInterface.hpp"
#ifndef _STRUS_STREAM_TOKEN_PATTERN_MATCH_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_TOKEN_PATTERN_MATCH_INSTANCE_INTERFACE_HPP_INCLUDED
#include "strus/stream/tokenPatternMatchOptions.hpp"
#include <string>

namespace strus
{

/// \brief Forward declaration
class TokenPatternMatchContextInterface;

/// \brief Interface for building the automaton for detecting patterns of tokens in a document stream
class TokenPatternMatchInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~TokenPatternMatchInstanceInterface(){}

	/// \brief Define a relative document term frequency used for optimization of the automaton
	/// \param[in] termid term identifier
	/// \param[in] df document frequency (only compared relatively, value between 0 and a virtual collection size)
	virtual void defineTermFrequency( unsigned int termid, double df)=0;

	/// \brief Push a term on the stack
	/// \param[in] termid term identifier
	virtual void pushTerm( unsigned int termid)=0;

	///\brief Join operations (same meaning as in query evaluation)
	enum JoinOperation
	{
		OpSequence,		///< The argument patterns must appear in the specified (strict) order (ordinal span) within a specified proximity range of ordinal positions for the completion of the rule.
		OpSequenceImm,		///< Same as OpSequence, but does not allow any gap in between the elements.
		OpSequenceStruct,	///< The argument patterns must appear in the specified (strict) order (ordinal span) within a specified proximity range of ordinal positions for the completion of the rule without a structure element appearing before the last argument pattern needed for then completion of the rule.
		OpWithin,		///< The argument patterns must appear within a specified proximity range of ordinal positions without overlapping ordinal spans for the completion of the rule.
		OpWithinStruct,		///< The argument patterns must appear within a specified proximity range of ordinal positions without overlapping ordinal spans for the completion of the rule without a structure element appearing before the last element for then completion of the rule.
		OpAny,			///< At least one of the argument patterns must appear for the completion of the rule.
		OpAnd			///< All of the argument patterns must appear for the completion of the rule at the same ordinal position.
	};

	/// \brief Take the topmost elements from the stack, build an expression out of them and replace the argument elements with the created element on the stack
	/// \param[in] operation identifier of the operation to perform as string
	/// \param[in] argc number of arguments of this operation
	/// \param[in] range position proximity range of the expression
	/// \param[in] cardinality specifies a result dimension requirement (e.g. minimum number of elements of any input subset selection that builds a result) (0 for use default). Interpretation depends on operation, but in most cases it specifies the required size for a valid result.
	/// \note The operation identifiers should if possible correspond to the names used for the standard posting join operators in the query evaluation of the strus core.
	virtual void pushExpression(
			JoinOperation operation,
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
	virtual void definePattern( const std::string& name, bool visible)=0;

	/// \brief Compile all patterns defined
	/// \param[in] opt optimization options
	/// \note Tries to optimize the program if possible by setting initial key events of the programs to events that are relative rare
	virtual bool compile( const stream::TokenPatternMatchOptions& opt)=0;

	/// \brief Create the context to process a document with the pattern matcher
	/// \return the pattern matcher context
	/// \remark The context cannot be reset. So the context has to be recreated for every processed unit (document)
	virtual TokenPatternMatchContextInterface* createContext() const=0;
};

} //namespace
#endif



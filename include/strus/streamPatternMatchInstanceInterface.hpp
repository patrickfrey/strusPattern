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
		OpSequence,		///< A subset specified by cardinality of the trigger events must appear in the specified order for then completion of the rule (objects must not overlap)
		OpSequenceStruct,	///< A subset specified by cardinality of the trigger events must appear in the specified order without a structure element appearing before the last element for then completion of the rule (objects must not overlap)
		OpWithin,		///< A subset specified by cardinality of the trigger events must appear for then completion of the rule (objects must not overlap)
		OpWithinStruct,		///< A subset specified by cardinality of the trigger events must appear without a structure element appearing before the last element for then completion of the rule (objects must not overlap)
		OpAny			///< Any of the trigger events leads for the completion of the rule
	};

	/// \brief Take the topmost elements from the stack, build an expression out of them and replace the argument elements with the created element on the stack
	/// \param[in] operation identifier of the operation to perform as string
	/// \param[in] argc number of arguments of this operation
	/// \param[in] range position proximity range of the expression
	/// \param[in] cardinality specifies a result dimension requirement (e.g. minimum number of elements of any input subset selection that builds a result) (0 for use default). Interpretation depends on operation, but in most cases it specifies the required size for a valid result.
	/// \note The operation identifiers should if possible correspond to the names used for the standard posting join operators in the strus core query evaluation
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
	virtual void closePattern( const std::string& name, bool visible)=0;

	/// \brief Structure with options for optimization of the pattern evaluation program
	struct OptimizeOptions
	{
		float stopwordOccurrenceFactor;		///< The bias for the factor nof programs with a specific keyword divided by the number of programs defined that decides wheter we try to find another key event for the program
		float weightFactor;
		uint32_t maxRange;			///< Maximum proximity range a program must have in order to be triggered by an alternative key event

		///\brief Constructor
		OptimizeOptions()
			:stopwordOccurrenceFactor(0.01f),weightFactor(10.0f),maxRange(5){}
		OptimizeOptions( float stopwordOccurrenceFactor_, float weightFactor_, uint32_t maxRange_)
			:stopwordOccurrenceFactor(stopwordOccurrenceFactor_),weightFactor(weightFactor_),maxRange(maxRange_){}
		///\brief Copy constructor
		OptimizeOptions( const OptimizeOptions& o)
			:stopwordOccurrenceFactor(o.stopwordOccurrenceFactor),weightFactor(o.weightFactor),maxRange(o.maxRange){}
	};
	/// \brief Try to optimize the program by setting initial key events of the programs to events that are relative rare 
	virtual void optimize( const OptimizeOptions& opt)=0;

	/// \brief Create the context to process a document with the pattern matcher
	/// \return the pattern matcher context
	/// \remark The context cannot be reset. So the context has to be recreated for every processed unit (document)
	virtual StreamPatternMatchContextInterface* createContext() const=0;
};

} //namespace
#endif


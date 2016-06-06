/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for building the automaton for detecting terms defined as regular expressions
/// \file "streamTermMatchInstanceInterface.hpp"
#ifndef _STRUS_STREAM_TERM_MATCH_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_TERM_MATCH_INSTANCE_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Forward declaration
class StreamTermMatchContextInterface;

/// \brief Interface for building the automaton for detecting terms defined as regular expressions
class StreamTermMatchInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~StreamTermMatchInstanceInterface(){}

	/// \enum PositionBind
	/// \brief Determines how document positions are assigned to terms
	enum PositionBind
	{
		BindContent,		///< An element in the document that gets an own position assigned
		BindSuccessor,		///< An element in the document that gets the position of the succeding content element assigned
		BindPredecessor		///< An element in the document that gets the position of the preceding content element assigned
	};

	/// \brief Define a pattern for the text matcher
	/// \param[in] id identifier given to the tokens matching the pattern, 0 if the pattern is invisible
	/// \param[in] expression expression string of the pattern
	/// \param[in] level weight of the pattern. A pattern causes the suppressing of all tokens of lower level that are completely covered by one token of this pattern
	/// \param[in] posbind determines how the oridinal position of the result term is assigned, wheter it gets an own position, is bound to the predecessor position or is bound to the successor position
	virtual void definePattern( unsigned int id, const std::string& expression, unsigned int level, PositionBind posbind)=0;

	/// \brief Define a symbol, an instance of a pattern, that gets a different id
	/// \param[in] id identifier given to the result tokens, 0 if the result token is invisible
	/// \param[in] patternid identifier of the pattern this symbol belongs to
	/// \param[in] name name (value string) of the symbol
	virtual void defineSymbol( unsigned int id, unsigned int patternid, const std::string& name)=0;

	/// \brief Compile all patterns defined with 'getPatternId(const std::string&)'
	/// \return true on success, false on error (error reported in error buffer)
	/// \remark This function has to be called in order to make the patterns active, resp. before calling 'createContext()'
	virtual bool compile()=0;

	/// \brief Create the context to process a chunk of text with the text matcher
	/// \return the term matcher context
	/// \remark The context cannot be reset. So the context has to be recreated for every processed unit (document)
	virtual StreamTermMatchContextInterface* createContext() const=0;
};

} //namespace
#endif


/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure with options for optimization of a token pattern match automaton
/// \file "tokenPatternMatchOptimizeOptions.hpp"
#ifndef _STRUS_STREAM_TOKEN_PATTERN_MATCH_OPTIMIZATION_OPTIONS_HPP_INCLUDED
#define _STRUS_STREAM_TOKEN_PATTERN_MATCH_OPTIMIZATION_OPTIONS_HPP_INCLUDED
#include <vector>

namespace strus {
namespace stream {

/// \brief Structure with options for optimization of a token pattern match automaton
struct TokenPatternMatchOptimizeOptions
{
	float stopwordOccurrenceFactor;		///< The bias for the factor number of programs with a specific keyword divided by the number of programs defined that decides wheter we try to find another key event for the program.
	float weightFactor;			///< factor of weight an alternative key event of a pattern must exceed to be considered as alternative.
	uint32_t maxRange;			///< Maximum proximity range a program must have in order to be triggered by an alternative key event.

	///\brief Default constructor
	TokenPatternMatchOptimizeOptions()
		:stopwordOccurrenceFactor(0.01f),weightFactor(10.0f),maxRange(5){}
	///\brief Constructor
	TokenPatternMatchOptimizeOptions( float stopwordOccurrenceFactor_, float weightFactor_, uint32_t maxRange_)
		:stopwordOccurrenceFactor(stopwordOccurrenceFactor_),weightFactor(weightFactor_),maxRange(maxRange_){}
	///\brief Copy constructor
	TokenPatternMatchOptimizeOptions( const TokenPatternMatchOptimizeOptions& o)
		:stopwordOccurrenceFactor(o.stopwordOccurrenceFactor),weightFactor(o.weightFactor),maxRange(o.maxRange){}
};

}} //namespace
#endif


/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STREAM_PATTERN_MATCH_CONTEXT_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_CONTEXT_INTERFACE_HPP_INCLUDED
#include "strus/stream/patternMatchResult.hpp"
#include <vector>

namespace strus
{

/// \brief Interface for detecting patterns in one document
class StreamPatternMatchContextInterface
{
public:
	/// \brief Destructor
	virtual ~StreamPatternMatchContextInterface(){}

	/// \brief Feed the next input term
	/// \brief termid term identifier
	/// \brief ordpos 'ordinal position' term counting position for term proximity measures.
	/// \brief origpos original position in the source. Not interpreted by the pattern matching (except for joining spans) but can be used by the caller to identify the source term.
	/// \brief origsize original size in the source. Not interpreted by the pattern matching (except for joining spans) but can be used by the caller to identify the source term span.
	/// \remark The input terms must be fed in ascending order of 'ordpos'
	virtual void putInput(
			unsigned int termid,
			unsigned int ordpos,
			unsigned int origpos,
			unsigned int origsize)=0;

	/// \brief Get the list of matches detected in the current document
	/// \return the list of matches
	virtual std::vector<stream::PatternMatchResult> fetchResults() const=0;

	/// \brief Some statistics for global pattern matching analysis
	class Statistics
	{
	public:
		/// \brief Statistics item
		class Item
		{
		public:
			/// \brief Name of the item
			const char* name() const	{return m_name;}
			/// \brief Value of the item
			double value() const		{return m_value;}

			Item( const char* name_, double value_)
				:m_name(name_),m_value(value_){}
			Item( const Item& o)
				:m_name(o.m_name),m_value(o.m_value){}

			void setValue( double value_)	{m_value = value_;}
		private:
			const char* m_name;
			double m_value;
		};
		/// \brief Constructor
		Statistics(){}
		/// \brief Copy constructor
		Statistics( const Statistics& o)
			:m_items(o.m_items){}

		/// \brief Define statistics item
		void define( const char* name, double value)
		{
			m_items.push_back( Item( name, value));
		}

		/// \brief Get all items defined
		const std::vector<Item>& items() const	{return m_items;}

	private:
		std::vector<Item> m_items;
	};

	/// \brief Get some statistics for global pattern matching analysis
	/// \param[out] the statistics data gathered during processing
	virtual Statistics getStatistics() const=0;
};

} //namespace
#endif


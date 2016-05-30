/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Object descriping the statistics of a pattern match run for runtime analysis
/// \file "patternMatchStatistics.hpp"
#ifndef _STRUS_STREAM_PATTERN_MATCH_STATISTICS_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCH_STATISTICS_HPP_INCLUDED
#include <vector>

namespace strus {
namespace stream {

/// \brief Some statistics for global pattern matching analysis
class PatternMatchStatistics
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
	PatternMatchStatistics(){}
	/// \brief Copy constructor
	PatternMatchStatistics( const PatternMatchStatistics& o)
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

}} //namespace
#endif


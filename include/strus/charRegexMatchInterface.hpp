/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for creating an automaton for detecting tokens defined as regular expressions in text
/// \file "charRegexMatchInterface.hpp"
#ifndef _STRUS_STREAM_CHAR_REGEX_MATCH_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_CHAR_REGEX_MATCH_INTERFACE_HPP_INCLUDED
#include <vector>
#include <string>

namespace strus
{
/// \brief Forward declaration
class CharRegexMatchInstanceInterface;

/// \brief Interface for creating an automaton for detecting tokens defined as regular expressions in text
class CharRegexMatchInterface
{
public:
	/// \brief Destructor
	virtual ~CharRegexMatchInterface(){}

	/// \brief Options to stear regular expression automaton
	/// \remark Available options depend on implementation
	/// \note For the hyperscan backend, the following options are available:
	///		"CASELESS", "DOTALL", "MULTILINE", "ALLOWEMPTY", "UCP"
	/// \note The options HS_FLAG_UTF8 and HS_FLAG_SOM_LEFTMOST are set implicitely always
	class Options
	{
	public:
		/// \brief Constructor
		Options(){}
		/// \brief Copy constructor
		Options( const Options& o)
			:m_opts(o.m_opts){}
		/// \brief Build operator
		Options& operator()( const char* opt)
		{
			m_opts.push_back( opt);
			return *this;
		}

		typedef std::vector<std::string>::const_iterator const_iterator;
		const_iterator begin() const	{return m_opts.begin();}
		const_iterator end() const	{return m_opts.end();}

	private:
		std::vector<std::string> m_opts;	///< list of option strings
	};

	/// \brief Create an instance to build the regular expressions for a term matcher
	/// \return the term matcher instance
	virtual CharRegexMatchInstanceInterface* createInstance( const Options& opts) const=0;
};

} //namespace
#endif


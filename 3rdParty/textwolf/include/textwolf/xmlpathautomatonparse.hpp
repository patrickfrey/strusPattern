/*
---------------------------------------------------------------------
    The template library textwolf implements an input iterator on
    a set of XML path expressions without backward references on an
    STL conforming input iterator as source. It does no buffering
    or read ahead and is dedicated for stream processing of XML
    for a small set of XML queries.
    Stream processing in this context refers to processing the
    document without buffering anything but the current result token
    processed with its tag hierarchy information.

    Copyright (C) 2010,2011,2012,2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3.0 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of textwolf can be found at 'http://github.com/patrickfrey/textwolf'
	For documentation see 'http://patrickfrey.github.com/textwolf'

--------------------------------------------------------------------
*/
/// \file textwolf/xmlpathautomatonparse.hpp
/// \brief Parser to create a path expression selector automaton from a source (list of path expression in abbreviated syntax of xpath)

#ifndef __TEXTWOLF_XML_PATH_AUTOMATON_PARSE_HPP__
#define __TEXTWOLF_XML_PATH_AUTOMATON_PARSE_HPP__
#include "textwolf/xmlpathautomaton.hpp"
#include "textwolf/charset.hpp"
#include "textwolf/cstringiterator.hpp"
#include <limits>
#include <string>
#include <vector>
#include <cstring>
#include <cstddef>
#include <stdexcept>

namespace textwolf {

///\class XMLPathSelectAutomatonParser
///\tparam SrcCharSet character set of the automaton definition source
///\tparam AtmCharSet character set of the token defintions of the automaton
///\brief Automaton to define XML path expressions and assign types (int values) to them
template <class SrcCharSet=charset::UTF8, class AtmCharSet=charset::UTF8>
class XMLPathSelectAutomatonParser :public XMLPathSelectAutomaton<AtmCharSet>
{
public:
	typedef XMLPathSelectAutomaton<AtmCharSet> ThisAutomaton;
	typedef typename ThisAutomaton::PathElement PathElement;
	typedef XMLPathSelectAutomatonParser This;
	typedef TextScanner<CStringIterator,SrcCharSet> SrcScanner;

public:
	///\brief Constructor
	XMLPathSelectAutomatonParser(){}
	virtual ~XMLPathSelectAutomatonParser(){}

	int addExpression( int typeidx, const char* esrc, std::size_t esrcsize)
	{
		std::string idstrings;
		CStringIterator pitr( esrc, esrcsize);
		SrcScanner pp( m_srccharset, pitr);
		std::vector<std::size_t> idref;
		std::size_t id;

		char const* xx = (char const*)std::memchr( esrc, ':', esrcsize);
		while (xx)
		{
			std::size_t xpos = xx - esrc;
			if (xpos + 1 < esrcsize && xx[1] == ':')
			{
				return xpos+1;
			}
			xx = (char const*)std::memchr( xx+1, ':', esrcsize - (xpos+1));
		}
		for (; *pp; skipSpaces( pp))
		{
			switch (*pp)
			{
				case '/':
				case '*':
				case '@':
				case '~':
				case '=':
				case '[':
				case ']':
				case ',':
					++pp;
					continue;
				case '"':
				case '\'':
					id = parseValue( pp, idstrings);
					idref.push_back( id);
					continue;
				case '(':
					while (*pp != 0 && *pp != ')') pp++;
					if (*pp == 0) return pp.getPosition()+1;
					++pp;
					continue;
				default:
					if (pp.control() == Undef || pp.control() == Any)
					{
						id = parseIdentifier( pp, idstrings);
						idref.push_back( id);
					}
					else
					{
						return pp.getPosition()+1;
					}
			}
		}
		typename std::vector<std::size_t>::const_iterator di = idref.begin(), de = idref.end();

		CStringIterator itr( esrc, esrcsize);
		SrcScanner src( m_srccharset, itr);
		PathElement expr( this);
		for (; *src; skipSpaces( src))
		{
			switch (*src)
			{
				case '@':
				{
					if (di == de) return src.getPosition()+1;
					++src;
					skipIdentifier( src);
					expr.selectAttribute( getIdentifier( *di++, idstrings));
					continue;
				}
				case '/':
				{
					++src;
					if (*src == '/')
					{
						expr --;
						++src;
						if (*src == '@')
						{
							++src;
							if (*src == '*')
							{
								++src;
								expr.selectAttribute( 0);
							}
							else
							{
								if (di == de) return src.getPosition()+1;
								skipIdentifier( src);
								expr.selectAttribute( getIdentifier( *di++, idstrings));
							}
						}
						else if (*src == '(')
						{
							continue;
						}
						else
						{
							if (*src == '*')
							{
								++src;
								expr.selectTag( 0);
							}
							else
							{		
								if (di == de) return src.getPosition()+1;
								skipIdentifier( src);
								expr.selectTag( getIdentifier( *di++, idstrings));
							}
						}
					}
					else
					{
						if (*src == '@')
						{
							++src;
							if (*src == '*')
							{
								++src;
								expr.selectAttribute( 0);
							}
							else
							{
								if (di == de) return src.getPosition()+1;
								skipIdentifier( src);
								expr.selectAttribute( getIdentifier( *di++, idstrings));
							}
						}
						else if (*src == '(')
						{
							continue;
						}
						else
						{
							if (*src == '*')
							{
								++src;
								expr.selectTag( 0);
							}
							else
							{
								if (di == de) return src.getPosition()+1;
								skipIdentifier( src);
								expr.selectTag( getIdentifier( *di++, idstrings));
							}
						}
					}
					continue;
				}
				case '~':
				{
					++src;
					expr.selectCloseTag();
					continue;
				}
				case '[':
				{
					++src; skipSpaces( src);
					if (*src == '@')
					{
						++src; skipSpaces( src);
						// Attribute condition:
						skipIdentifier( src);
						skipSpaces( src);
						if (di == de) return src.getPosition()+1;
						const char* attrname = getIdentifier( *di++, idstrings);
						if (*src != '=') return src.getPosition()+1;
						++src; skipSpaces( src);
						skipValue( src);
						skipSpaces( src);
						const char* attrval = getIdentifier( *di++, idstrings);
						if (*src != ']') return src.getPosition()+1;
						expr.ifAttribute( attrname, attrval);
						++src;
					}
					else
					{
						// Range
						skipIdentifier( src);
						skipSpaces( src);
						if (di == de) return src.getPosition()+1;
						const char* range_start_str = getIdentifier( *di++, idstrings);
						int range_start = parseNum( range_start_str);
						if (range_start < 0 || range_start_str[0]) return src.getPosition()+1;

						if (*src == ',')
						{
							++src; skipSpaces( src);
							if (*src == ']')
							{
								expr.FROM( range_start);
								++src;
							}
							else
							{
								skipIdentifier( src);
								skipSpaces( src);
								if (di == de) return src.getPosition()+1;
								const char* range_end_str = getIdentifier( *di++, idstrings);
								int range_end = parseNum( range_end_str);
								if (range_end < 0 || range_end_str[0]) return src.getPosition()+1;
								++src; skipSpaces( src);
								if (*src != ']') return src.getPosition()+1;
								expr.RANGE( range_start, range_end);
								++src;
							}
						}
						else if (*src == ']')
						{
							expr.INDEX( range_start);
							++src;
						}
						else
						{
							return src.getPosition()+1;
						}
					}
					continue;
				}
				case '(':
					++src;
					skipSpaces( src);
					if (*src != ')') return src.getPosition()+1;
					++src;
					expr.selectContent();
					skipSpaces( src);
					if (*src) return src.getPosition()+1;
					continue;

				default:
					return src.getPosition()+1;
			}
		}
		expr.assignType( typeidx);
		return 0;
	}

private:
	static void skipSpaces( SrcScanner& src)
	{
		for (; src.control() == Space; ++src);
	}

	static int parseNum( char const*& src)
	{
		std::string num;
		for (; *src>='0' && *src<='9';++src) num.push_back( *src);
		if (num.size() == 0 || num.size() > 8) return -1;
		return std::atoi( num.c_str());
	}

	static bool isIdentifierChar( SrcScanner& src)
	{
		if (src.control() == Undef || src.control() == Any)
		{
			if (*src == (unsigned char)'*') return false;
			if (*src == (unsigned char)'~') return false;
			if (*src == (unsigned char)'/') return false;
			if (*src == (unsigned char)'(') return false;
			if (*src == (unsigned char)')') return false;
			if (*src == (unsigned char)'@') return false;
			return true;
		}
		return false;
	}

	std::size_t parseIdentifier( SrcScanner& src, std::string& idstrings)
	{
		std::size_t rt = idstrings.size();
		for (; isIdentifierChar(src); ++src)
		{
			m_atmcharset.print( *src, idstrings);
		}
		m_atmcharset.print( 0, idstrings);
		return rt;
	}

	std::size_t parseValue( SrcScanner& src, std::string& idstrings)
	{
		std::size_t rt = idstrings.size();
		if (*src == '"' || *src == '\'')
		{
			unsigned char eb = *src;
			for (++src; *src && *src != eb; ++src)
			{
				m_atmcharset.print( *src, idstrings);
			}
			if (*src) ++src;
		}
		else
		{
			for (; isIdentifierChar(src); ++src)
			{
				m_atmcharset.print( *src, idstrings);
			}
		}
		m_atmcharset.print( 0, idstrings);
		return rt;
	}

	static void skipIdentifier( SrcScanner& src)
	{
		for (; isIdentifierChar(src); ++src){}
	}

	static void skipValue( SrcScanner& src)
	{
		if (*src == '"' || *src == '\'')
		{
			unsigned char eb = *src;
			for (++src; *src && *src != eb; ++src){}
			if (*src) ++src;
		}
		else
		{
			for (; isIdentifierChar(src); ++src){}
		}
	}

	const char* getIdentifier( std::size_t idx, const std::string& idstrings) const
	{
		return idstrings.c_str() + idx;
	}

private:
	AtmCharSet m_atmcharset;
	SrcCharSet m_srccharset;
};

} //namespace
#endif

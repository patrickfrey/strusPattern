/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\file "regexExpressions.cpp"
///\brief Some functions to handle regex expressions
#include "regexExpressions.hpp"
#include "internationalization.hpp"
#include "errorUtils.hpp"
#include "textwolf/charset_utf8.hpp"
#include <limits>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <new>

using namespace strus;

static bool isLiteral( char ch)
{
	static const char* regexchr = ".|*?+(){}[]^$";
	return 0==std::strchr( regexchr, ch);
}

static textwolf::charset::UTF8::CharLengthTab g_charLengthTab;

static std::size_t characterByteLen( char ch)
{
	std::size_t chrlen = g_charLengthTab[ (unsigned char)ch];
	if (chrlen == 0)
	{
		throw strus::runtime_error(_TXT("error in expression: literal is not an UTF-8 character"));
	}
	return chrlen;
}

static std::vector<std::string> getPrefixProduct( const std::vector<std::string>& v1, const std::vector<std::string>& v2)
{
	std::vector<std::string> rt;
	std::vector<std::string>::const_iterator
		xi = v1.begin(), xe = v1.end();
	std::vector<std::string>::const_iterator
		ri = v2.begin(), re = v2.end();
	for (; ri != re; ++ri)
	{
		for (; xi != xe; ++xi)
		{
			rt.push_back( *xi + *ri);
		}
	}
	return rt;
}

static std::vector<std::string> getPrefixProduct( const std::vector<std::string>& v1, const std::string& elem)
{
	std::vector<std::string> rt;
	std::vector<std::string>::const_iterator
		xi = v1.begin(), xe = v1.end();
	for (; xi != xe; ++xi)
	{
		rt.push_back( *xi + elem);
	}
	return rt;
}

static void skipBracket( char eb, char const*& si, const char* se)
{
	for(; si < se; ++si)
	{
		if (*si == eb) return;
		if (*si == '\\')
		{
			++si;
		}
		else if (*si == '[')
		{
			++si;
			skipBracket( ']', si, se);
		}
		else if (*si == '(')
		{
			++si;
			skipBracket( ')', si, se);
		}
		else if (*si == '{')
		{
			++si;
			skipBracket( '}', si, se);
		}
	}
}

static void skipExpression( char const*& si, const char* se)
{
	for(; si < se; ++si)
	{
		if (!isLiteral(*si))
		{
			if (*si == '|' || *si == ')')
			{
				return;
			}
			if (*si == '[')
			{
				++si;
				skipBracket( ']', si, se);
			}
			else if (*si == '(')
			{
				++si;
				skipBracket( ')', si, se);
			}
			else if (*si == '{')
			{
				++si;
				skipBracket( '}', si, se);
			}
		}
	}
}

static std::vector<std::string> getPrefixes( char const*& si, const char* se, bool& usedSkipExpr)
{
	std::vector<std::string> rt;
	rt.push_back( std::string());

	std::string prefix;
	while (si < se)
	{
		if (*si == '\\')
		{
			skipExpression( si, se);
			usedSkipExpr = true;
		}
		else if (isLiteral(*si))
		{
			std::size_t chrlen = characterByteLen( *si);
			if (si + chrlen < se && (si[chrlen] == '*' || si[chrlen] == '?' || si[chrlen] == '{'))
			{
				si += chrlen+1;
				skipExpression( si, se);
				usedSkipExpr = true;
			}
			else
			{
				prefix.append( si, chrlen);
				si += chrlen;
				if (si < se && *si == '+')
				{
					skipExpression( si, se);
					usedSkipExpr = true;
				}
			}
		}
		else if (*si == '(')
		{
			++si;
			if (!prefix.empty())
			{
				rt = getPrefixProduct( rt, prefix);
				prefix.clear();
			}
			std::vector<std::string> follow = getPrefixes( si, se, usedSkipExpr);
			if (si >= se || *si != ')')
			{
				throw strus::runtime_error(_TXT("missing close bracket ')' in expression"));
			}
			else if (si+1 < se && (si[1] == '*' || si[1] == '?' || si[1] == '{'))
			{
				si += 2;
				skipExpression( si, se);
				usedSkipExpr = true;
			}
			else
			{
				++si;
				rt = getPrefixProduct( rt, follow);
				if (si+1 < se && si[1] == '+')
				{
					skipExpression( si, se);
					usedSkipExpr = true;
				}
			}
		}
		else if (*si == ')')
		{
			if (!prefix.empty())
			{
				rt = getPrefixProduct( rt, prefix);
				prefix.clear();
			}
			return rt;
		}
		else if (*si == '|')
		{
			rt.push_back( std::string());
			++si;
		}
		else if (*si == '[' && si+1 < se && si[1] == '[')
		{
			//... excluding [[...]] because cannot handle collating elements or character classes properly yet
			skipExpression( si, se);
			usedSkipExpr = true;
		}
		else if (*si == '[')
		{
			++si;
			if (si < se && *si == '^')
			{
				//... excluding negation of character set
				skipExpression( si, se);
				usedSkipExpr = true;
			}
			else
			{
				std::vector<std::string> alt;
				std::size_t chrlen;
				for (; si < se && *si != ']'; si += chrlen)
				{
					if (*si == '\\')
					{
						++si;
						if (si == se)
						{
							throw strus::runtime_error(_TXT("unexpected end of expression"));
						}
					}
					chrlen = characterByteLen( *si);
					alt.push_back( std::string( si, chrlen));
				}
				if (!alt.empty())
				{
					if (si+1 < se && (si[1] == '*' || si[1] == '?' || si[1] == '{'))
					{
						si += 2;
						skipExpression( si, se);
						usedSkipExpr = true;
					}
					else
					{
						++si;
						rt = getPrefixProduct( rt, alt);
						if (si+1 < se && si[1] == '+')
						{
							skipExpression( si, se);
							usedSkipExpr = true;
						}
					}
				}
			}
		}
		else
		{
			skipExpression( si, se);
			usedSkipExpr = true;
		}
	}
	return rt;
}


bool strus::isRegularExpression( const std::string& src)
{
	char const* si = src.c_str();
	for (; *si; ++si)
	{
		if (!isLiteral( *si)) return true;
	}
	return false;
}

std::vector<std::string> strus::getRegexPrefixes( const std::string& expr)
{
	bool usedSkipExpr = false;
	char const* si = expr.c_str();
	const char* se = si + expr.size();
	std::vector<std::string> rt = getPrefixes( si, se, usedSkipExpr);
	std::vector<std::string>::const_iterator ri = rt.begin(), re = rt.end();
	for (; ri != re; ++ri)
	{
		if (ri->empty()) return std::vector<std::string>();
	}
	return rt;
}

std::vector<std::string> getRegexSuffixes( const std::string& expr)
{
	std::vector<std::string> rt;
	char const* si = expr.c_str();
	const char* se = si + expr.size();
	for (; si != se; ++si)
	{
		if (*si == '{')
		{
			skipBracket( '}', si, se);
		}
		else if (*si == '[')
		{
			bool usedSkipExpr = false;
			char const* sa = si;
			rt = getPrefixes( sa, se, usedSkipExpr);
			if (!usedSkipExpr) break;
			rt.clear();
			skipBracket( ']', si, se);
		}
		else if (*si == '(')
		{
			bool usedSkipExpr = false;
			char const* sa = si;
			std::vector<std::string> rt = getPrefixes( sa, se, usedSkipExpr);
			if (!usedSkipExpr) break;
			rt.clear();
			skipBracket( ')', si, se);
		}
	}
	std::vector<std::string>::const_iterator ri = rt.begin(), re = rt.end();
	for (; ri != re; ++ri)
	{
		if (ri->empty()) return std::vector<std::string>();
	}
	return rt;
}


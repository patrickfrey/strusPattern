/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "lexems.hpp"
#include "internationalization.hpp"
#include <string>
#include <vector>
#include <cstdarg>
#include <sstream>
#include <iostream>
#include <stdexcept>

using namespace strus;
using namespace strus::parser;

bool parser::is_UNSIGNED( const char* src)
{
	char const* cc = src;
	if (!isDigit( *cc)) return false;
	for (++cc; isDigit( *cc); ++cc){}
	if (*cc == '.' || isAlnum(*cc)) return false;
	return true;
}

bool parser::is_INTEGER( const char* src)
{
	char const* cc = src;
	if (isDash(*cc))
	{
		++cc;
	}
	return is_UNSIGNED( cc);
}

bool parser::is_FLOAT( const char* src)
{
	char const* cc = src;
	if (isDash(*cc))
	{
		++cc;
	}
	if (!isDigit( *cc)) return false;
	for (++cc; isDigit( *cc); ++cc){}
	if (isAlnum(*cc)) return false;
	return true;
}

bool parser::isEqual( const std::string& id, const char* idstr)
{
	char const* si = id.c_str();
	char const* di = idstr;
	for (; *si && *di && ((*si|32) == (*di|32)); ++si,++di){}
	return !*si && !*di;
}

std::string parser::parse_IDENTIFIER( char const*& src)
{
	std::string rt;
	while (isAlnum( *src)) rt.push_back( *src++);
	skipSpaces( src);
	return rt;
}

std::string parser::parse_TEXTWORD( char const*& src)
{
	std::string rt;
	while (isTextChar( *src)) rt.push_back( *src++);
	skipSpaces( src);
	return rt;
}

std::string parser::parse_STRING( char const*& src)
{
	std::string rt;
	char eb = *src++;
	while (*src != eb)
	{
		if (*src == '\0' || *src == '\n') throw strus::runtime_error(_TXT("unterminated string %c...%c"), eb, eb);
		if (*src == '\\')
		{
			src++;
			if (*src == '\0' || *src == '\n') throw strus::runtime_error(_TXT("unterminated string %c...%c"), eb, eb);
		}
		rt.push_back( *src++);
	}
	++src;
	skipSpaces( src);
	return rt;
}

std::string parser::parse_REGEX( char const*& src)
{
	std::string rt;
	char eb = *src++;
	while (*src != eb)
	{
		if (*src == '\0' || *src == '\n') throw strus::runtime_error(_TXT("unterminated string %c...%c"), eb, eb);
		if (*src == '\\')
		{
			rt.push_back( *src++);
			if (*src == '\0' || *src == '\n') throw strus::runtime_error(_TXT("unterminated string %c...%c"), eb, eb);
		}
		rt.push_back( *src++);
	}
	++src;
	skipSpaces( src);
	return rt;
}

unsigned int parser::parse_UNSIGNED( char const*& src)
{
	unsigned int rt = 0;
	while (isDigit( *src))
	{
		unsigned int vv = (rt * 10) + (*src - '0');
		if (vv <= rt) throw strus::runtime_error(_TXT("index out of range"));
		rt = vv;
		++src;
	}
	skipSpaces( src);
	return rt;
}

unsigned int parser::parse_UNSIGNED1( char const*& src)
{
	unsigned int rt = parse_UNSIGNED( src);
	if (rt == 0) throw strus::runtime_error(_TXT("positive unsigned integer expected"));
	return rt;
}

double parser::parse_FLOAT( char const*& src)
{
	unsigned int digitsAllowed = 9;
	double rt = 0.0;
	if (*src == '-')
	{
		++src;
		rt = -1.0;
	}
	while (isDigit( *src) && digitsAllowed)
	{
		rt = (rt * 10.0) + (*src - '0');
		++src;
		--digitsAllowed;
	}
	if (isDot( *src))
	{
		float div = 1.0;
		++src;
		while (isDigit( *src))
		{
			div /= 10.0;
			rt += (*src - '0') * div;
			++src;
		}
	}
	if (!digitsAllowed)
	{
		throw strus::runtime_error(_TXT("floating point number out of range"));
	}
	skipSpaces( src);
	return rt;
}

char parser::parse_OPERATOR( char const*& src)
{
	char rt = *src++;
	skipSpaces( src);
	return rt;
}

int parser::parse_INTEGER( char const*& src)
{
	int rt = 0;
	int prev = 0;
	if (!*src) throw strus::runtime_error(_TXT("integer expected"));
	bool neg = false;
	if (*src == '-')
	{
		++src;
		neg = true;
	}
	if (!(*src >= '0' && *src <= '9')) throw strus::runtime_error(_TXT("integer expected"));

	for (; *src >= '0' && *src <= '9'; ++src)
	{
		rt = (rt * 10) + (*src - '0');
		if (prev > rt) throw strus::runtime_error(_TXT("integer number out of range"));
		prev = rt;
	}
	if (isAlpha(*src)) throw strus::runtime_error(_TXT("integer expected"));

	skipSpaces( src);
	if (neg)
	{
		return -rt;
	}
	else
	{
		return rt;
	}
}

static int checkKeyword( std::string id, int nn, va_list argp)
{
	for (int ii=0; ii<nn; ++ii)
	{
		const char* keyword = va_arg( argp, const char*);
		if (isEqual( id, keyword))
		{
			return ii;
		}
	}
	return -1;
}

static std::string keywordList( va_list argp, int nn)
{
	std::ostringstream msg;
	for (int ii=0; ii<nn; ++ii)
	{
		const char* keyword = va_arg( argp, const char*);
		if (ii > 0) msg << " ,";
		msg << "'" << keyword << "'";
	}
	return msg.str();
}

int parser::parse_KEYWORD( char const*& src, unsigned int nn, ...)
{
	char const* src_bk = src;
	va_list argp;
	va_start( argp, nn);

	std::string id = parse_IDENTIFIER( src);
	va_start( argp, nn);

	int ii = checkKeyword( id, nn, argp);
	if (ii < 0)
	{
		src = src_bk;
		va_start( argp, nn);
		std::string kw( keywordList( argp, nn));
		throw strus::runtime_error(_TXT("unknown keyword '%s', one of %s expected"), id.c_str(), kw.c_str());
	}
	va_end( argp);
	return ii;
}

int parser::parse_KEYWORD( unsigned int& duplicateflags, char const*& src, unsigned int nn, ...)
{
	char const* src_bk = src;
	va_list argp;
	va_start( argp, nn);

	if (nn > sizeof(unsigned int)*8) throw std::logic_error("too many arguments (parse_KEYWORD)");

	std::string id = parse_IDENTIFIER( src);
	va_start( argp, nn);

	int ii = checkKeyword( id, nn, argp);
	if (ii < 0)
	{
		src = src_bk;
		va_start( argp, nn);
		std::string kw( keywordList( argp, nn));
		throw strus::runtime_error(_TXT("unknown keyword '%s', one of %s expected"), id.c_str(), kw.c_str());
	}
	va_end( argp);
	if ((duplicateflags & (1 << ii))!= 0)
	{
		throw strus::runtime_error( _TXT( "duplicate definition of '%s'"), id.c_str());
	}
	duplicateflags |= (1 << ii);
	return ii;
}


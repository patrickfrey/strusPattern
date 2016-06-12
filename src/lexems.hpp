/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_PROGRAM_LOADER_LEXEMS_HPP_INCLUDED
#define _STRUS_PROGRAM_LOADER_LEXEMS_HPP_INCLUDED
#include <string>

namespace strus {
namespace parser {

static inline bool isAlpha( char ch)
{
	return ((ch|32) <= 'z' && (ch|32) >= 'a') || (ch) == '_';
}
static inline bool isDigit( char ch)
{
	return (ch <= '9' && ch >= '0');
}
static inline bool isOr( char ch)
{
	return (ch == '|');
}
static inline bool isSlash( char ch)
{
	return (ch == '/');
}
static inline bool isExp( char ch)
{
	return (ch == '^');
}
static inline bool isAlnum( char ch)
{
	return isAlpha(ch) || isDigit(ch);
}
static inline bool isTextChar( char ch)
{
	return isAlnum(ch) || (unsigned char)ch >= 128;
}
static inline bool isDash( char ch)
{
	return ch == '-';
}
static inline bool isAssign( char ch)
{
	return ch == '=';
}
static inline bool isComma( char ch)
{
	return (ch == ',');
}
static inline bool isDot( char ch)
{
	return ch == '.';
}
static inline bool isColon( char ch)
{
	return ch == ':';
}
static inline bool isSemiColon( char ch)
{
	return ch == ';';
}
static inline bool isOpenSquareBracket( char ch)
{
	return ch == '[';
}
static inline bool isCloseSquareBracket( char ch)
{
	return ch == ']';
}
static inline bool isOpenOvalBracket( char ch)
{
	return ch == '(';
}
static inline bool isLeftArrow( const char* si)
{
	return si[0] == '<' && si[1] == '-';
}
static inline bool isRightArrow( const char* si)
{
	return si[0] == '-' && si[1] == '>';
}
static inline bool isCloseOvalBracket( char ch)
{
	return ch == ')';
}
static inline bool isOpenCurlyBracket( char ch)
{
	return ch == '{';
}
static inline bool isCloseCurlyBracket( char ch)
{
	return ch == '}';
}
static inline bool isAsterisk( char ch)
{
	return ch == '*';
}
static inline bool isPercent( char ch)
{
	return ch == '%';
}
static inline bool isStringQuote( char ch)
{
	return ch == '\'' || ch == '"';
}
static inline bool isSpace( char ch)
{
	return ch == ' ' || ch == '\t' || ch == '\n';
}
static inline void skipToEoln( char const*& src)
{
	while (*src && *src != '\n') ++src;
}
static inline void skipSpaces( char const*& src)
{
	for (;;)
	{
		while (isSpace( *src)) ++src;
		if (*src == '#')
		{
			++src;
			skipToEoln( src);
		}
		else
		{
			break;
		}
	}
}
bool is_INTEGER( const char* src);
bool is_UNSIGNED( const char* src);
bool is_FLOAT( const char* src);
bool isEqual( const std::string& id, const char* idstr);
std::string parse_IDENTIFIER( char const*& src);
std::string parse_TEXTWORD( char const*& src);
std::string parse_STRING( char const*& src);
std::string parse_REGEX( char const*& src);
unsigned int parse_UNSIGNED( char const*& src);
unsigned int parse_UNSIGNED1( char const*& src);
double parse_FLOAT( char const*& src);
char parse_OPERATOR( char const*& src);
int parse_INTEGER( char const*& src);
int parse_KEYWORD( char const*& src, unsigned int nof, ...);
int parse_KEYWORD( unsigned int& duplicateflags, char const*& src, unsigned int nof, ...);

}}//namespace
#endif

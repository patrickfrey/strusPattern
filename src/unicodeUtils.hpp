/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of code pages for various character sets for lexers that have no full unicode support
/// \file "oneByteCharMap.hpp"
#ifndef _STRUS_PATTERN_CODEPAGES_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_PATTERN_CODEPAGES_IMPLEMENTATION_HPP_INCLUDED
#include <string>
#include <vector>
#include <cwchar>

namespace strus {

class OneByteCharMap
{
public:
	OneByteCharMap()
		:value(),posar(){}

	void init( const char* src, std::size_t srcsize);

	std::string value;
	std::vector<std::size_t> posar;
};

struct WCharString
{
public:
	WCharString( const char* src, std::size_t srcsize);
	~WCharString();

	wchar_t* str() const					{return m_ptr;}
	std::size_t size() const				{return m_size;}
	std::size_t origpos( std::size_t wcharpos) const	{return wcharpos ? m_posbuf[ wcharpos-1]:0;}

private:
	wchar_t* m_ptr;
	std::size_t* m_pos;
	std::size_t m_size;
	std::size_t m_posbuf[ 1024];
	wchar_t m_charbuf[ 1024];
};

}//namespace
#endif




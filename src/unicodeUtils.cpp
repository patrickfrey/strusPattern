/*
 * Copyright (c) 2017 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Mapping of UTF-8 character set encodings to a one byte artificial charset encoding as hash
#include "unicodeUtils.hpp"
#include "textwolf/charset.hpp"
#include "textwolf/sourceiterator.hpp"
#include "textwolf/textscanner.hpp"
#include "textwolf/cstringiterator.hpp"
#include "textwolf/staticbuffer.hpp"
#include "internationalization.hpp"

using namespace strus;

void OneByteCharMap::init( const char* src, std::size_t srcsize)
{
	typedef textwolf::TextScanner<textwolf::SrcIterator,textwolf::charset::UTF8> TextScanner;
	textwolf::charset::UTF8 utf8;
	textwolf::SrcIterator srcitr( src, srcsize, 0);
	TextScanner itr( utf8, srcitr);
	textwolf::UChar ch;

	value.clear();
	posar.clear();

	posar.push_back( 0);
	while ((ch = *itr) != 0)
	{
		++itr;
		if (ch <= 127)
		{
			value.push_back( ch);
		}
		else
		{
			value.push_back( 128 + (ch % 128));
		}
		posar.push_back( itr.getPosition());
	}
}

template <class CharSet>
static void printString( const CharSet& charset, textwolf::StaticBuffer& outbuf, std::size_t* posar, const char* src, std::size_t srcsize, int sizeofwchar)
{
	typedef textwolf::TextScanner<textwolf::SrcIterator,textwolf::charset::UTF8> TextScanner;
	textwolf::charset::UTF8 utf8;
	textwolf::SrcIterator srcitr( src, srcsize, 0);
	TextScanner itr( utf8, srcitr);
	textwolf::UChar ch;

	std::size_t chidx = 0;
	while ((ch = *itr) != 0)
	{
		++itr;
		std::size_t pi = outbuf.size();
		charset.print( ch, outbuf);
		std::size_t pe = outbuf.size();
		for (; pi < pe; pi += sizeofwchar)
		{
			posar[ chidx++] = itr.getPosition();
		}
	}
}

WCharString::WCharString( const char* src, std::size_t srcsize)
{
	static const short be_le = 1;
	if (srcsize >= (sizeof( m_charbuf) / sizeof( m_charbuf[0])))
	{
		m_ptr = (wchar_t*)malloc( (1+srcsize) * sizeof(wchar_t));
		m_pos = (std::size_t*)malloc( (1+srcsize) * sizeof(std::size_t));
		if (!m_ptr || !m_pos)
		{
			if (m_ptr) std::free( m_ptr);
			if (m_pos) std::free( m_pos);
			throw std::bad_alloc();
		}
	}
	else
	{
		m_ptr = m_charbuf;
		m_pos = m_posbuf;
	}
	textwolf::StaticBuffer outbuf( (char*)m_ptr, srcsize * sizeof(wchar_t));

	if (((const char*)&be_le)[0])
	{
		/// ... Little endian
		if (sizeof(wchar_t) == 4)
		{
			textwolf::charset::UCS4<textwolf::charset::ByteOrder::LE> outcharset;
			printString( outcharset, outbuf, m_pos, src, srcsize, sizeof(wchar_t));
		}
		else
		{
			textwolf::charset::UTF16<textwolf::charset::ByteOrder::LE> outcharset;
			printString( outcharset, outbuf, m_pos, src, srcsize, sizeof(wchar_t));
		}
	}
	else
	{
		/// ... Big endian
		if (sizeof(wchar_t) == 4)
		{
			textwolf::charset::UCS4<textwolf::charset::ByteOrder::BE> outcharset;
			printString( outcharset, outbuf, m_pos, src, srcsize, sizeof(wchar_t));
		}
		else
		{
			textwolf::charset::UTF16<textwolf::charset::ByteOrder::BE> outcharset;
			printString( outcharset, outbuf, m_pos, src, srcsize, sizeof(wchar_t));
		}
	}
	m_size = outbuf.size() / sizeof(wchar_t);
	m_ptr[ m_size] = 0;
}

WCharString::~WCharString()
{
	if (m_pos && m_pos != m_posbuf) std::free( m_pos);
	if (m_ptr && m_ptr != m_charbuf) std::free( m_ptr);
}



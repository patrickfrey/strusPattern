/*
 * Copyright (c) 2017 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Mapping of UTF-8 character set encodings to a one byte artificial charset encoding as hash
#include "oneByteCharMap.hpp"
#include "textwolf/charset.hpp"
#include "textwolf/sourceiterator.hpp"
#include "textwolf/textscanner.hpp"
#include "textwolf/cstringiterator.hpp"
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


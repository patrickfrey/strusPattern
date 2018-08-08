/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of code pages for various character sets for lexers that have no full unicode support
/// \file "codepages.hpp"
#ifndef _STRUS_PATTERN_CODEPAGES_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_PATTERN_CODEPAGES_IMPLEMENTATION_HPP_INCLUDED
#include "strus/base/stdint.h"
#include <vector>
#include <map>

namespace strus {

/// \brief 
class CodePageMap
{
public:
	CodePageMap()
	{
		init();
	}

	struct Character
	{
		unsigned char codepage;
		unsigned char chr;

		Character( unsigned char codepage_, unsigned char chr_)
			:codepage(codepage_),chr(chr_){}
		Character()
			:codepage(0),chr(0){}
		Character( const Character& o)
			:codepage(o.codepage),chr(o.chr){}
	};

	Character get( uint32_t uchr) const
	{
		std::map<uint32_t,std::size_t>::const_iterator mi = m_pgmap.lower_bound( uchr);
		if (mi == m_pgmap.end())
		{
			return Character();
		}
		else
		{
			const CodePage& cpg = m_pgar[ mi->second];
			if (uchr - cpg.pgstart >= (uint32_t)(cpg.destend - cpg.deststart))
			{
				return Character();
			}
			unsigned char ofs = uchr - cpg.pgstart;
			return Character( mi->id, cpg.deststart + ofs);
		}
	}

private:
	enum CodePages
	{
		Ascii=0,
		Greek,
		Cyrillic,
		Hebrew,
		Arabic,
	};

	void addCodePage( unsigned char id_, unsigned char deststart_, unsigned char destend_, uint32_t pgstart_)
	{
		m_pgmap[ pgstart_] = m_pgar.size();
		m_pgar.push_back( CodePage( id_, deststart_, pgstart_));
	}
	void init()
	{
		addCodePage( Greek, 0x80, 0xFF, 0x370);
		addCodePage( Cyrillic, 0x80, 0xD0, 0x0400);
		addCodePage( Cyrillic, 0xD0, 0xFF, 0x048A);
		addCodePage( Hebrew, 0x80, 0xFF, 0x0590);
		Arabic
	}

	struct CodePage
	{
		unsigned char id;
		unsigned char deststart;
		unsigned char destend;
		uint32_t pgstart;

		CodePage( const CodePage& o)
			:id(o.id),deststart(o.deststart),destend(o.destend),pgstart(o.pgstart){}
		CodePage( unsigned char id_, unsigned char deststart_, unsigned char destend_, uint32_t pgstart_)
			:id(id_),deststart(deststart_),destend(destend_),pgstart(pgstart_){}
		CodePage()
			:id(0),deststart(0),destend(0),pgstart(0){}
	};
	std::vector<CodePage> m_pgar;
	std::map<uint32_t,std::size_t> m_pgmap;
};

}//namespace
#endif




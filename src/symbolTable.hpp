/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRING_MAP_HPP_INCLUDED
#define _STRUS_STRING_MAP_HPP_INCLUDED
#include "strus/base/crc32.hpp"
#include <boost/unordered_map.hpp>
#include <map>
#include <list>
#include <vector>
#include <string>
#include <cstring>
#include <limits>

namespace strus
{

class StringMapKeyBlock
{
public:
	enum {DefaultSize = 16300};

public:
	explicit StringMapKeyBlock( std::size_t blksize_=DefaultSize);
	StringMapKeyBlock( const StringMapKeyBlock& o);
	~StringMapKeyBlock();

	const char* allocKey( const std::string& key);
	const char* allocKey( const char* key, std::size_t keylen);

private:
	char* m_blk;
	std::size_t m_blksize;
	std::size_t m_blkpos;
};

class StringMapKeyBlockList
{
public:
	StringMapKeyBlockList(){}
	StringMapKeyBlockList( const StringMapKeyBlockList& o)
		:m_ar(o.m_ar){}

	const char* allocKey( const char* key, std::size_t keylen);
	void clear();

private:
	std::list<StringMapKeyBlock> m_ar;
};


class SymbolTable
{
private:
	struct MapKeyLess
	{
		bool operator()( char const *a, char const *b) const
		{
			return std::strcmp( a, b) < 0;
		}
	};
	struct MapKeyEqual
	{
		bool operator()( char const *a, char const *b) const
		{
			return std::strcmp( a, b) == 0;
		}
	};
	struct HashFunc{
		int operator()( const char * str)const
		{
			return utils::Crc32::calc( str);
		}
	};

	typedef boost::unordered_map<const char*,uint32_t,HashFunc,MapKeyEqual> Map;

public:
	SymbolTable(){}

	uint32_t getOrCreate( const std::string& key);

	uint32_t get( const std::string& key) const;

	const char* key( const uint32_t& value) const;

private:
	SymbolTable( const SymbolTable&){}	///> non copyable
	void operator=( const SymbolTable&){}	///> non copyable

private:
	Map m_map;
	std::vector<const char*> m_invmap;
	StringMapKeyBlockList m_keystring_blocks;
};

}//namespace
#endif



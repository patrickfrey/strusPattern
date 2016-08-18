/*
 * Copyright (c) 2016 Patrick P. Frey
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
	struct Key
	{
		const char* str;
		std::size_t len;

		Key()
			:str(0),len(0){}
		Key( const char* str_, std::size_t len_)
			:str(str_),len(len_){}
		Key( const Key& o)
			:str(o.str),len(o.len){}
	};
	struct MapKeyLess
	{
		bool operator()( const Key& a, const Key& b) const
		{
			if (a.len < b.len) return true;
			if (a.len > b.len) return false;
			return std::memcmp( a.str, b.str, a.len) < 0;
		}
	};
	struct MapKeyEqual
	{
		bool operator()( const Key& a, const Key& b) const
		{
			return a.len == b.len && std::memcmp( a.str, b.str, a.len) == 0;
		}
	};
	struct HashFunc{
		int operator()( const Key& key)const
		{
			return utils::Crc32::calc( key.str, key.len);
		}
	};

	typedef boost::unordered_map<Key,uint32_t,HashFunc,MapKeyEqual> Map;
	typedef boost::unordered_map<Key,uint32_t,HashFunc,MapKeyEqual>::const_iterator const_iterator;

public:
	SymbolTable(){}

	uint32_t getOrCreate( const std::string& key);
	uint32_t getOrCreate( const char* key, std::size_t keysize);

	uint32_t get( const std::string& key) const;
	uint32_t get( const char* key, std::size_t keysize) const;

	const char* key( const uint32_t& value) const;

	const_iterator begin() const
	{
		return m_map.begin();
	}
	const_iterator end() const
	{
		return m_map.end();
	}
	std::size_t size() const
	{
		return m_invmap.size();
	}

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



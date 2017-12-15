/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Some utility functions that are centralised to control dependencies to boost
#ifndef _STRUS_UTILS_HPP_INCLUDED
#define _STRUS_UTILS_HPP_INCLUDED
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

namespace strus {
namespace utils {

std::string tolower( const char* val);
std::string tolower( const std::string& val);
std::string trim( const std::string& val);
bool caseInsensitiveEquals( const std::string& val1, const std::string& val2);
bool caseInsensitiveStartsWith( const std::string& val, const std::string& prefix);
int toint( const std::string& val);
std::string tostring( int val);

void aligned_free( void *ptr);
void* aligned_malloc( std::size_t size, std::size_t alignment);

template<typename Key, typename Elem>
class UnorderedMap
	:public boost::unordered_map<Key,Elem>
{
public:
	UnorderedMap(){}
	UnorderedMap( const UnorderedMap& o)
		:boost::unordered_map<Key,Elem>(){}
};

template <class X>
class SharedPtr
	:public boost::shared_ptr<X>
{
public:
	SharedPtr( X* ptr)
		:boost::shared_ptr<X>(ptr){}
	SharedPtr( const SharedPtr& o)
		:boost::shared_ptr<X>(o){}
	SharedPtr()
		:boost::shared_ptr<X>(){}
};

}} //namespace
#endif


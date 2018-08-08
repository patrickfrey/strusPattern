/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\brief Definition of a basic POD type table structure for the rule matcher automaton
#ifndef _STRUS_PATTERN_POD_STRUCT_TABLE_BASE_HPP_INCLUDED
#define _STRUS_PATTERN_POD_STRUCT_TABLE_BASE_HPP_INCLUDED
#include "strus/base/stdint.h"
#include "podStructArrayBase.hpp"
#include "internationalization.hpp"
#include "errorUtils.hpp"
#include <limits>
#include <stdexcept>
#include <new>
#include <set>

#undef STRUS_CHECK_FREE_ITEMS
#undef STRUS_CHECK_USED_ITEMS

namespace strus
{

template <typename ELEMTYPE, typename SIZETYPE, class FREELISTTYPE, unsigned int BASEADDR>
class PodStructTableBase
	:public PodStructArrayBase<ELEMTYPE,SIZETYPE,BASEADDR>
{
public:
	typedef PodStructArrayBase<ELEMTYPE,SIZETYPE,BASEADDR> Parent;

	PodStructTableBase()
#ifdef STRUS_CHECK_FREE_ITEMS
		:m_free_elemtab()
#else
		:m_freelistidx(0)
#endif
#ifdef STRUS_CHECK_USED_ITEMS
		,m_used_size(0)
#endif
	{}

	PodStructTableBase( const PodStructTableBase& o)
#ifdef STRUS_CHECK_FREE_ITEMS
		:Parent(o),m_free_elemtab(o.m_free_elemtab)
#else
		:Parent(o),m_freelistidx(o.m_freelistidx)
#endif
#ifdef STRUS_CHECK_USED_ITEMS
		,m_used_size(o.m_used_size)
#endif
	{}

	ELEMTYPE& operator[]( SIZETYPE idx)
	{
#ifdef STRUS_CHECK_FREE_ITEMS
		typename std::set<SIZETYPE>::const_iterator fi = m_free_elemtab.find( idx);
		if (fi != m_free_elemtab.end())
		{
			throw strus::runtime_error( _TXT("write of element disposed (%s)"), "PodStructTableBase");
		}
#endif
		return Parent::operator []( idx);
	}

	const ELEMTYPE& operator[]( SIZETYPE idx) const
	{
#ifdef STRUS_CHECK_FREE_ITEMS
		typename std::set<SIZETYPE>::const_iterator fi = m_free_elemtab.find( idx);
		if (fi != m_free_elemtab.end())
		{
			throw strus::runtime_error( _TXT("read of element disposed (%s)"), "PodStructTableBase");
		}
#endif
		return Parent::operator []( idx);
	}

	SIZETYPE add( const ELEMTYPE& elem)
	{
#ifdef STRUS_CHECK_USED_ITEMS
		++m_used_size;
#endif
#ifndef STRUS_CHECK_FREE_ITEMS
		if (m_freelistidx)
		{
			SIZETYPE newidx = m_freelistidx-1;
			m_freelistidx = ((FREELISTTYPE*)(void*)(&(*this)[ newidx]))->next;
			(*this)[ newidx] = elem;
			return newidx;
		}
		else
#endif
		{
			return Parent::add( elem);
		}
	}

	void remove( SIZETYPE idx)
	{
#ifdef STRUS_USE_BASEADDR
		if (idx < BASEADDR)
		{
			throw std::runtime_error( _TXT("removing illegal element from table"));
		}
#endif
#ifdef STRUS_CHECK_FREE_ITEMS
		m_free_elemtab.insert( idx);
#else
		((FREELISTTYPE*)(void*)(&(*this)[ idx]))->next = m_freelistidx;
		m_freelistidx = idx+1;
#endif
#ifdef STRUS_CHECK_USED_ITEMS
		--m_used_size;
#endif
	}

#ifdef STRUS_CHECK_USED_ITEMS
	SIZETYPE used_size() const
	{
		return m_used_size;
	}
#else
	SIZETYPE used_size() const
	{
		return Parent::size();
	}
#endif
	void checkTable() const
	{
#ifndef STRUS_CHECK_FREE_ITEMS
		// Check freelist:
		SIZETYPE fi = m_freelistidx;
		while (fi)
		{
			fi = ((FREELISTTYPE*)(void*)(&(*this)[ fi-1]))->next;
#ifdef STRUS_USE_BASEADDR
			if (fi && fi < BASEADDR)
			{
				throw std::runtime_error( _TXT("check table freelist failed"));
			}
#endif
		}
#endif
	}

	void clear()
	{
		Parent::clear();
#ifdef STRUS_CHECK_FREE_ITEMS
		m_free_elemtab.clear();
#else
		m_freelistidx = 0;
#endif
#ifdef STRUS_CHECK_USED_ITEMS
		m_used_size = 0;
#endif
	}

	bool exists( SIZETYPE idx) const
	{
#ifdef STRUS_USE_BASEADDR
		idx -= BASEADDR;
#endif
#ifdef STRUS_CHECK_FREE_ITEMS
		return (idx < Parent::size() && m_free_elemtab.find( idx) == m_free_elemtab.end());
#else
		return (idx < Parent::size());
#endif
	}

private:
#ifdef STRUS_CHECK_FREE_ITEMS
	std::set<SIZETYPE> m_free_elemtab;
#else
	SIZETYPE m_freelistidx;
#endif
#ifdef STRUS_CHECK_USED_ITEMS
	SIZETYPE m_used_size;
#endif
};

}//namespace
#endif


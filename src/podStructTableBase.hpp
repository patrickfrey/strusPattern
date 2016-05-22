/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\brief Definition of a basic POD type table structure for the rule matcher automaton
#ifndef _STRUS_STREAM_POD_STRUCT_TABLE_BASE_HPP_INCLUDED
#define _STRUS_STREAM_POD_STRUCT_TABLE_BASE_HPP_INCLUDED
#include "strus/base/stdint.h"
#include "podStructArrayBase.hpp"
#include "internationalization.hpp"
#include "errorUtils.hpp"
#include <limits>
#include <stdexcept>
#include <new>
#include <set>

#define STRUS_CHECK_FREE_ITEMS

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
	{}

	PodStructTableBase( const PodStructTableBase& o)
#ifdef STRUS_CHECK_FREE_ITEMS
		:Parent(o),m_free_elemtab(o.m_free_elemtab)
#else
		:Parent(o),m_freelistidx(o.m_freelistidx)
#endif
	{}

#ifdef STRUS_CHECK_FREE_ITEMS
	ELEMTYPE& operator[]( SIZETYPE idx)
	{
		typename std::set<SIZETYPE>::const_iterator fi = m_free_elemtab.find( idx);
		if (fi != m_free_elemtab.end()) throw strus::runtime_error( _TXT("write of element disposed (PodStructArrayBase)"));
		return Parent::operator []( idx);
	}
#endif

#ifdef STRUS_CHECK_FREE_ITEMS
	const ELEMTYPE& operator[]( SIZETYPE idx) const
	{
		typename std::set<SIZETYPE>::const_iterator fi = m_free_elemtab.find( idx);
		if (fi != m_free_elemtab.end()) throw strus::runtime_error( _TXT("read of element disposed (PodStructArrayBase)"));
		return Parent::operator []( idx);
	}
#endif

	SIZETYPE add( const ELEMTYPE& elem)
	{
#ifndef STRUS_CHECK_FREE_ITEMS
		if (m_freelistidx)
		{
			SIZETYPE newidx = m_freelistidx-1;
			m_freelistidx = ((FREELISTTYPE*)(void*)(&(*this)[ m_freelistidx-1]))->next;
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
#ifdef STRUS_CHECK_FREE_ITEMS
		m_free_elemtab.insert( idx);
#else
		((FREELISTTYPE*)(void*)(&(*this)[ idx]))->next = m_freelistidx;
		m_freelistidx = idx+1;
#endif
	}

private:
	void reset(){}		//< forbid usage of inherited method

private:
#ifdef STRUS_CHECK_FREE_ITEMS
	std::set<SIZETYPE> m_free_elemtab;
#else
	SIZETYPE m_freelistidx;
#endif
};

}//namespace
#endif


/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\brief Definition of a pool of basic POD stack structures for the rule matcher automaton
#ifndef _STRUS_STREAM_POD_STACK_POOL_BASE_HPP_INCLUDED
#define _STRUS_STREAM_POD_STACK_POOL_BASE_HPP_INCLUDED
#include "strus/base/stdint.h"
#include "podStructArrayBase.hpp"
#include "podStructTableBase.hpp"
#include <limits>
#include <stdexcept>
#include <new>

namespace strus
{

template <typename ELEMTYPE, typename SIZETYPE>
struct PodStackElement
{
	ELEMTYPE value;
	SIZETYPE next;

	Element( ELEMTYPE value_, SIZETYPE next_)
		:value(value_),next(next_){}
	Element( const Element& o)
		:value(o.value),next(o.next){}
};

template <typename ELEMTYPE, typename SIZETYPE>
class PodStackPoolBase
	:private PodStructTableBase<PodStackElement<ELEMTYPE,SIZETYPE>,SIZETYPE>
{
public:
	typedef PodStructTableBase<PodStackElement<ELEMTYPE,SIZETYPE>,SIZETYPE> Parent;

	PodStackPoolBase(){}
	PodStackPoolBase( const PodStackPoolBase& o) :Parent(o){}

	void push( SIZETYPE& stk, const ELEMTYPE& elem)
	{
		PodStackElement<ELEMTYPE,SIZETYPE> elem( elem, stk);
		stk = Parent::add( elem)+1;
	}

	void remove( SIZETYPE stk)
	{
		if (stk == 0) return;
		if (stk > m_size)
		{
			throw std::runtime_error( "bad stack index (remove)");
		}
		SIZETYPE idx = stk;
		while (idx)
		{
			SIZETYPE nextidx = (*this)[ idx-1].next;
			Parent::remove( idx-1);
			idx = nextidx;
		}
	}

	bool pop( SIZETYPE& stk, ELEMTYPE& elem)
	{
		if (stk == 0) return false;
		if (stk > m_size)
		{
			throw std::runtime_error( "bad stack index (remove)");
		}
		elem = (*this)[ stk-1].value;
		SIZETYPE nextidx = (*this)[ idx-1].next;
		Parent::remove( stk-1);
		stk = nextidx;
		return true;
	}

	bool next( SIZETYPE& stk, ELEMTYPE& elem) const
	{
		if (stk == 0) return false;
		if (stk > m_size)
		{
			throw std::runtime_error( "bad stack index (nextElement)");
		}
		elem = m_ar[ stk-1].value;
		stk = m_ar[ stk-1].next;
	}
};

}//namespace
#endif


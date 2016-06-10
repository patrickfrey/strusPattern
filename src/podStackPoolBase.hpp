/*
 * Copyright (c) 2016 Patrick P. Frey
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
#include "internationalization.hpp"
#include "errorUtils.hpp"
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

	PodStackElement( ELEMTYPE value_, SIZETYPE next_)
		:value(value_),next(next_){}
	PodStackElement( const PodStackElement& o)
		:value(o.value),next(o.next){}
};

template <typename ELEMTYPE, typename SIZETYPE, unsigned int BASEADDR>
class PodStackPoolBase
	:private PodStructTableBase<PodStackElement<ELEMTYPE,SIZETYPE>,SIZETYPE,PodStackElement<ELEMTYPE,SIZETYPE>,BASEADDR>
{
public:
	typedef PodStructTableBase<PodStackElement<ELEMTYPE,SIZETYPE>,SIZETYPE,PodStackElement<ELEMTYPE,SIZETYPE>,BASEADDR> Parent;

	PodStackPoolBase(){}
	PodStackPoolBase( const PodStackPoolBase& o) :Parent(o){}

	void push( SIZETYPE& stk, const ELEMTYPE& elem)
	{
		PodStackElement<ELEMTYPE,SIZETYPE> listelem( elem, stk);
		stk = Parent::add( listelem)+1;
	}

	void remove( SIZETYPE stk)
	{
		if (stk == 0) return;
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
		elem = (*this)[ stk-1].value;
		SIZETYPE nextidx = (*this)[ stk-1].next;
		Parent::remove( stk-1);
		stk = nextidx;
		return true;
	}

	void set( SIZETYPE stk, const ELEMTYPE& elem) const
	{
		if (stk == 0 || stk > Parent::size()) throw strus::runtime_error(_TXT( "illegal list access (set)"));
		(*this)[ stk-1].value = elem;
	}

	bool next( SIZETYPE& stk, ELEMTYPE& elem) const
	{
		if (stk == 0) return false;
		elem = (*this)[ stk-1].value;
		stk = (*this)[ stk-1].next;
		return true;
	}
	const ELEMTYPE* nextptr( SIZETYPE& stk) const
	{
		if (stk == 0) return 0;
		const ELEMTYPE* rt = &(*this)[ stk-1].value;
		stk = (*this)[ stk-1].next;
		return rt;
	}
	void check( SIZETYPE idx) const
	{
		(*this)[ idx-1];
	}
};

}//namespace
#endif


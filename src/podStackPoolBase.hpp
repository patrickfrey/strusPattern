/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\brief Definition of a pool of basic POD stack structures for the rule matcher automaton
#ifndef _STRUS_PATTERN_POD_STACK_POOL_BASE_HPP_INCLUDED
#define _STRUS_PATTERN_POD_STACK_POOL_BASE_HPP_INCLUDED
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

#undef STRUS_LOWLEVEL_DEBUG
#undef STRUS_LOWLEVEL_DEBUG_CHECK_CIRCULAR

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
#ifdef STRUS_LOWLEVEL_DEBUG
		if (stk != 0 && !Parent::exists( stk-1))
		{
			throw strus::runtime_error( "%s", _TXT( "illegal list access (push)"));
		}
#endif
		PodStackElement<ELEMTYPE,SIZETYPE> listelem( elem, stk);
		SIZETYPE new_idx = Parent::add( listelem);
		stk = new_idx+1;
#ifdef STRUS_LOWLEVEL_DEBUG_CHECK_CIRCULAR
		checkCircular( stk);
#endif
	}

	void remove( SIZETYPE stk)
	{
		if (stk == 0) return;
#ifdef STRUS_LOWLEVEL_DEBUG
		if (!Parent::exists( stk-1))
		{
			throw strus::runtime_error( _TXT( "illegal list access (%s)"), "remove");
		}
#ifdef STRUS_LOWLEVEL_DEBUG_CHECK_CIRCULAR
		checkCircular( stk);
#endif
#endif
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
#ifdef STRUS_LOWLEVEL_DEBUG
		if (!Parent::exists( stk-1))
		{
			throw strus::runtime_error( _TXT( "illegal list access (%s)"), "pop");
		}
#endif
		elem = (*this)[ stk-1].value;
		SIZETYPE nextidx = (*this)[ stk-1].next;
		Parent::remove( stk-1);
		stk = nextidx;
#ifdef STRUS_LOWLEVEL_DEBUG_CHECK_CIRCULAR
		checkCircular( stk);
#endif
		return true;
	}

	void set( SIZETYPE stk, const ELEMTYPE& elem)
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		if (!Parent::exists( stk-1))
		{
			throw strus::runtime_error( _TXT( "illegal list access (%s)"), "set");
		}
#endif
		(*this)[ stk-1].value = elem;
	}

	bool next( SIZETYPE& stk, ELEMTYPE& elem) const
	{
		if (stk == 0) return false;
#ifdef STRUS_LOWLEVEL_DEBUG
		if (!Parent::exists( stk-1))
		{
			throw strus::runtime_error( _TXT( "illegal list access (%s)"), "next");
		}
#endif
		elem = (*this)[ stk-1].value;
		stk = (*this)[ stk-1].next;
		return true;
	}
	const ELEMTYPE* nextptr( SIZETYPE& stk) const
	{
		if (stk == 0) return 0;
#ifdef STRUS_LOWLEVEL_DEBUG
		if (!Parent::exists( stk-1))
		{
			throw strus::runtime_error( _TXT( "illegal list access (%s)"), "nextptr");
		}
#endif
		const ELEMTYPE* rt = &(*this)[ stk-1].value;
		stk = (*this)[ stk-1].next;
		return rt;
	}
	void check( SIZETYPE idx) const
	{
		(*this)[ idx-1];
	}

	void checkTable()
	{
		Parent::checkTable();
	}

	void clear()
	{
		Parent::clear();
	}

private:
	void checkCircular( SIZETYPE idx) const
	{
		std::size_t ii = 0, ie = Parent::size();
		SIZETYPE si = idx;
		for (; ii!=ie && si; ++ii)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
		if (!Parent::exists( si-1))
		{
			throw strus::runtime_error( _TXT( "illegal list access (%s)"), "checkCircular");
		}
#endif
			si = (*this)[ si-1].next;
			if (si == idx || si == 0) break;
		}
		if (ii != ie && si != 0)
		{
			throw strus::runtime_error( "%s", _TXT( "internal: circular list"));
		}
	}
};

}//namespace
#endif


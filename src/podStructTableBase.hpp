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

namespace strus
{

template <typename ELEMTYPE, typename SIZETYPE, class FREELISTTYPE, unsigned int BASEADDR>
class PodStructTableBase
	:public PodStructArrayBase<ELEMTYPE,SIZETYPE,BASEADDR>
{
public:
	typedef PodStructArrayBase<ELEMTYPE,SIZETYPE,BASEADDR> Parent;

	PodStructTableBase()
		:m_freelistidx(0)
	{}

	PodStructTableBase( const PodStructTableBase& o)
		:Parent(o),m_freelistidx(o.m_freelistidx)
	{}

	SIZETYPE add( const ELEMTYPE& elem)
	{
		if (m_freelistidx)
		{
			SIZETYPE newidx = m_freelistidx-1;
			m_freelistidx = ((FREELISTTYPE*)(void*)(&(*this)[ m_freelistidx-1]))->next;
			(*this)[ newidx] = elem;
			return newidx;
		}
		else
		{
			return Parent::add( elem);
		}
	}

	void remove( SIZETYPE idx)
	{
		((FREELISTTYPE*)(void*)(&(*this)[ idx]))->next = m_freelistidx;
		m_freelistidx = idx+1;
	}

private:
	void reset(){}		//< forbid usage of inherited method

private:
	SIZETYPE m_freelistidx;
};

}//namespace
#endif


/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\brief Definition of a basic POD type array structure for the rule matcher automaton
#ifndef _STRUS_STREAM_POD_STRUCT_ARRAY_BASE_HPP_INCLUDED
#define _STRUS_STREAM_POD_STRUCT_ARRAY_BASE_HPP_INCLUDED
#include "strus/base/stdint.h"
#include <limits>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <new>

namespace strus
{

template <typename ELEMTYPE, typename SIZETYPE>
class PodStructArrayBase
{
public:
	PodStructArrayBase()
		:m_ar(0),m_allocsize(0),m_size(0)
	{}

	PodStructArrayBase( const PodStructArrayBase& o)
		:m_ar(0),m_allocsize(0),m_size(o.m_size)
	{
		if (o.m_allocsize)
		{
			expand( o.m_allocsize);
			std::memcpy( m_ar, o.m_ar, m_size * sizeof(*m_ar));
		}
	}

	SIZETYPE add( const ELEMTYPE& elem)
	{
		if (m_size == m_allocsize)
		{
			if (m_allocsize >= (std::numeric_limits<SIZETYPE>::max()/2))
			{
				throw std::bad_alloc();
			}
			expand( m_allocsize?(m_allocsize*2):BlockSize);
		}
		SIZETYPE newidx = m_size++;
		m_ar[ newidx] = elem;
		return newidx;
	}

	const ELEMTYPE& operator[]( SIZETYPE idx) const
	{
		if (idx >= m_size) throw std::runtime_error( "array bound read (PodStructArrayBase)");
		return m_ar[idx];
	}
	ELEMTYPE& operator[]( SIZETYPE idx)
	{
		if (idx >= m_size) throw std::runtime_error( "array bound write (PodStructArrayBase)");
		return m_ar[idx];
	}
	SIZETYPE size() const
	{
		return m_size;
	}
	void reset()
	{
		m_size = 0;
	}

private:
	void expand( SIZETYPE newallocsize)
	{
		if (m_size > newallocsize)
		{
			throw std::logic_error( "illegal call of PodStructArrayBase::expand");
		}
		ELEMTYPE* ar_ = (ELEMTYPE*)std::realloc( m_ar, newallocsize * sizeof(*m_ar));
		if (!ar_) throw std::bad_alloc();

		m_ar = ar_;
		m_allocsize = newallocsize;
	}

private:
	enum {BlockSize=128};
	ELEMTYPE* m_ar;
	SIZETYPE m_allocsize;
	SIZETYPE m_size;
	SIZETYPE m_freelistidx;
};

}//namespace
#endif


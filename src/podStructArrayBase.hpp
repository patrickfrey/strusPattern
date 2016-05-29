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
#include "internationalization.hpp"
#include "errorUtils.hpp"
#include <limits>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <new>

#define STRUS_USE_BASEADDR

namespace strus
{

template <typename ELEMTYPE, typename SIZETYPE, unsigned int BASEADDR>
class PodStructArrayBase
{
public:
	PodStructArrayBase( ELEMTYPE* ar_, SIZETYPE allocsize_)
		:m_ar(ar_),m_allocsize(allocsize_),m_size(0),m_allocated(false)
	{}
	PodStructArrayBase()
		:m_ar(0),m_allocsize(0),m_size(0),m_allocated(false)
	{}
	~PodStructArrayBase()
	{
		if (m_allocated) std::free( m_ar);
	}

	PodStructArrayBase( const PodStructArrayBase& o)
		:m_ar(0),m_allocsize(o.m_allocsize),m_size(o.m_size),m_allocated(false)
	{
		if (o.m_allocsize)
		{
			expand( m_allocsize);
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
#ifdef STRUS_USE_BASEADDR
		return newidx + BASEADDR;
#else
		return newidx;
#endif
	}

	const ELEMTYPE& operator[]( SIZETYPE idx) const
	{
#ifdef STRUS_USE_BASEADDR
		idx -= BASEADDR;
#endif
		if (idx >= m_size)
		{
			throw strus::runtime_error( _TXT("array bound read (PodStructArrayBase)"));
		}
		return m_ar[idx];
	}
	ELEMTYPE& operator[]( SIZETYPE idx)
	{
#ifdef STRUS_USE_BASEADDR
		idx -= BASEADDR;
#endif
		if (idx >= m_size)
		{
			throw strus::runtime_error( _TXT("array bound write (PodStructArrayBase)"));
		}
		return m_ar[idx];
	}
	ELEMTYPE* reserve( SIZETYPE addsize)
	{
		if (m_allocsize < m_size + addsize)
		{
			expand( m_size + addsize);
		}
		return m_ar + m_size;
	}
	void commit_reserved( SIZETYPE addsize)
	{
		if (m_allocsize < m_size + addsize)
		{
			throw strus::runtime_error( _TXT("array bound write on reserved area (PodStructArrayBase)"));
		}
		m_size += addsize;
	}

	SIZETYPE size() const
	{
		return m_size;
	}
	SIZETYPE first() const
	{
#ifdef STRUS_USE_BASEADDR
		return BASEADDR;
#else
		return 0;
#endif
	}
	void reset()
	{
		m_size = 0;
	}

	class const_iterator
	{
	public:
		const_iterator( const ELEMTYPE* itr_)
			:m_itr(itr_){}
		const_iterator& operator++()
		{
			++m_itr;
			return *this;
		}
		const_iterator& operator++(int)
		{
			const_iterator rt( m_itr);
			++m_itr;
			return rt;
		}
		const ELEMTYPE& operator*() const
		{
			return *m_itr;
		}
		const ELEMTYPE* operator->() const
		{
			return m_itr;
		}
		bool operator==( const const_iterator& o) const {return m_itr == o.m_itr;}
		bool operator!=( const const_iterator& o) const {return m_itr != o.m_itr;}
		bool operator<=( const const_iterator& o) const {return m_itr <= o.m_itr;}
		bool operator<( const const_iterator& o) const  {return m_itr < o.m_itr;}
		bool operator>=( const const_iterator& o) const {return m_itr >= o.m_itr;}
		bool operator>( const const_iterator& o) const  {return m_itr > o.m_itr;}

	private:
		const ELEMTYPE* m_itr;
	};

	const_iterator begin() const	{return const_iterator(m_ar);}
	const_iterator end() const	{return const_iterator(m_ar+m_size);}

private:
	void expand( SIZETYPE newallocsize)
	{
		if (m_size > newallocsize)
		{
			throw std::logic_error( "illegal call of PodStructArrayBase::expand");
		}
		ELEMTYPE* ar_;
		if (m_allocated)
		{
			ar_ = (ELEMTYPE*)std::realloc( m_ar, newallocsize * sizeof(*m_ar));
			if (!ar_) throw std::bad_alloc();
		}
		else
		{
			ar_ = (ELEMTYPE*)std::malloc( newallocsize * sizeof(*m_ar));
			if (!ar_) throw std::bad_alloc();
			std::memcpy( ar_, m_ar, m_size * sizeof(*m_ar));
			m_allocated = true;
		}
		m_ar = ar_;
		m_allocsize = newallocsize;
	}

private:
	enum {BlockSize=128};
	ELEMTYPE* m_ar;
	SIZETYPE m_allocsize;
	SIZETYPE m_size;
	bool m_allocated;
};

}//namespace
#endif


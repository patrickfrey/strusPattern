/*
---------------------------------------------------------------------
    The template library textwolf implements an input iterator on
    a set of XML path expressions without backward references on an
    STL conforming input iterator as source. It does no buffering
    or read ahead and is dedicated for stream processing of XML
    for a small set of XML queries.
    Stream processing in this Object refers to processing the
    document without buffering anything but the current result token
    processed with its tag hierarchy information.

    Copyright (C) 2010,2011,2012,2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3.0 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of textwolf can be found at 'http://github.com/patrickfrey/textwolf'
	For documentation see 'http://patrickfrey.github.com/textwolf'

--------------------------------------------------------------------
*/
/// \file textwolf/istreamiterator.hpp
/// \brief Definition of iterators for textwolf on an input stream class

#ifndef __TEXTWOLF_ISTREAM_ITERATOR_HPP__
#define __TEXTWOLF_ISTREAM_ITERATOR_HPP__
#include "textwolf/exception.hpp"
#include "textwolf/position.hpp"
#include <iostream>
#include <fstream>
#include <iterator>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <stdint.h>

/// \namespace textwolf
/// \brief Toplevel namespace of the library
namespace textwolf {

/// \class IStream
/// \brief Input stream interface
class IStream
{
public:
	virtual ~IStream(){}
	virtual std::size_t read( void* buf, std::size_t bufsize)=0;
	virtual int errorcode() const=0;
};

/// \class StdInputStream
/// \brief Input stream implementation based on std::istream
class StdInputStream
	:public IStream
{
public:
	StdInputStream( std::istream& input_)
		:m_input(&input_),m_errno(0)
	{
		m_input->unsetf( std::ios::skipws);
		m_input->exceptions ( std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit );
	}
	StdInputStream( const StdInputStream& o)
		:m_input(o.m_input),m_errno(o.m_errno){}

	virtual ~StdInputStream(){}
	virtual std::size_t read( void* buf, std::size_t bufsize)
	{
		try
		{
			m_errno = 0;
			m_input->read( (char*)buf, bufsize);
			return m_input->gcount();
		}
		catch (const std::istream::failure& err)
		{
			if (m_input->eof())
			{
				m_errno = 0;
				return m_input->gcount();
			}
			else
			{
				m_errno = errno;
				return 0;
			}
		}
		catch (...)
		{
			m_errno = errno;
			return 0;
		}
	}

	virtual int errorcode() const
	{
		return m_errno;
	}

private:
	std::istream* m_input;
	int m_errno;
};


/// \class IStreamIterator
/// \brief Input iterator on an STL input stream
class IStreamIterator
	:public throws_exception
{
public:
	/// \brief Default constructor
	IStreamIterator(){}
	/// \brief Destructor
	~IStreamIterator()
	{
		std::free(m_buf);
	}

	/// \brief Constructor
	/// \param [in] input input to iterate on
	IStreamIterator( IStream* input, std::size_t bufsize=8192)
		:m_input(input),m_buf((char*)std::malloc(bufsize)),m_bufsize(bufsize),m_readsize(0),m_readpos(0),m_abspos(0)
	{
		if (!m_buf) throw std::bad_alloc();
		fillbuf();
	}

	/// \brief Copy constructor
	/// \param [in] o iterator to copy
	IStreamIterator( const IStreamIterator& o)
		:m_input(o.m_input),m_buf((char*)std::malloc(o.m_bufsize)),m_bufsize(o.m_bufsize),m_readsize(o.m_readsize),m_readpos(o.m_readpos),m_abspos(o.m_abspos)
	{
		if (!m_buf) throw std::bad_alloc();
		std::memcpy( m_buf, o.m_buf, o.m_readsize);
	}

	/// \brief Element access
	/// \return current character
	inline char operator* ()
	{
		return (m_readpos < m_readsize)?m_buf[m_readpos]:0;
	}

	/// \brief Pre increment
	inline IStreamIterator& operator++()
	{
		if (m_readpos+1 >= m_readsize)
		{
			fillbuf();
		}
		else
		{
			++m_readpos;
		}
		return *this;
	}

	int operator - (const IStreamIterator& o) const
	{
		return (int)m_readpos - o.m_readpos;
	}

	PositionIndex position() const
	{
		return m_abspos + m_readpos;
	}

private:
	bool fillbuf()
	{
		m_abspos += m_readsize;
		m_readsize = m_input->read( m_buf, m_bufsize);
		m_readpos = 0;
		if (m_input->errorcode()) throw exception( FileReadError);
		return true;
	}

private:
	IStream* m_input;
	char* m_buf;
	std::size_t m_bufsize;
	std::size_t m_readsize;
	std::size_t m_readpos;
	PositionIndex m_abspos;
};

}//namespace
#endif

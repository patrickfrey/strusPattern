/*
---------------------------------------------------------------------
    The template library textwolf implements an input iterator on
    a set of XML path expressions without backward references on an
    STL conforming input iterator as source. It does no buffering
    or read ahead and is dedicated for stream processing of XML
    for a small set of XML queries.
    Stream processing in this context refers to processing the
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
/// \file textwolf/position.hpp
/// \brief Definition of position number in source
#ifndef __TEXTWOLF_POSITION_HPP__
#define __TEXTWOLF_POSITION_HPP__

#ifdef BOOST_VERSION
#include <boost/cstdint.hpp>
namespace textwolf {
	/// \typedef Position
	/// \brief Source position index type
	typedef boost::uint64_t PositionIndex;
}//namespace
#else
#ifdef _MSC_VER
#pragma warning(disable:4290)
#include <BaseTsd.h>
namespace textwolf {
	/// \typedef Position
	/// \brief Source position index type
	typedef DWORD64 PositionIndex;
}//namespace
#else
#include <stdint.h>
namespace textwolf {
	/// \typedef Position
	/// \brief Source position index type
	typedef uint64_t PositionIndex;
}//namespace
#endif
#endif

#endif


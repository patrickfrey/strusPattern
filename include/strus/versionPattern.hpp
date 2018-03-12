/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Version of the strus stream library
/// \file versionStream.hpp
#ifndef _STRUS_VERSION_STREAM_HPP_INCLUDED
#define _STRUS_VERSION_STREAM_HPP_INCLUDED

/// \brief strus toplevel namespace
namespace strus
{

/// \brief Version number of strus stream
#define STRUS_STREAM_VERSION (\
	0 * 1000000\
	+ 15 * 10000\
	+ 6\
)

/// \brief Major version number of strus stream
#define STRUS_STREAM_VERSION_MAJOR 0
/// \brief Minor version number of strus stream
#define STRUS_STREAM_VERSION_MINOR 15

/// \brief The version of the strus stream library
#define STRUS_STREAM_VERSION_STRING "0.15.6"

}//namespace
#endif

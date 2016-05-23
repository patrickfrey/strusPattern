/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the strus stream library
/// \file libstrus_stream.cpp
#include "strus/lib/stream.hpp"
#include "strus/errorBufferInterface.hpp"
#include "streamPatternMatch.hpp"
#include "strus/streamPatternMatchInterface.hpp"
#include "strus/base/dll_tags.hpp"
#include "internationalization.hpp"
#include "errorUtils.hpp"

using namespace strus;
static bool g_intl_initialized = false;

DLL_PUBLIC StreamPatternMatchInterface* strus::createStreamPatternMatch_standard( ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return new StreamPatternMatch( errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error stream pattern match interface: %s"), *errorhnd, 0);
}


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
#include "tokenPatternMatch.hpp"
#include "charRegexMatch.hpp"
#include "patternMatchProgram.hpp"
#include "strus/base/dll_tags.hpp"
#include "internationalization.hpp"
#include "errorUtils.hpp"

using namespace strus;
static bool g_intl_initialized = false;

DLL_PUBLIC TokenPatternMatchInterface* strus::createTokenPatternMatch_standard( ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return new TokenPatternMatch( errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating token pattern match interface: %s"), *errorhnd, 0);
}

DLL_PUBLIC CharRegexMatchInterface* strus::createCharRegexMatch_standard( ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return new CharRegexMatch( errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating char regex match interface: %s"), *errorhnd, 0);
}

DLL_PUBLIC PatternMatchProgramInterface* strus::createPatternMatchProgram_standard(
		const TokenPatternMatchInterface* tpm,
		const CharRegexMatchInterface* crm,
		ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return new PatternMatchProgram( tpm, crm, errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating the standard pattern match program interface: %s"), *errorhnd, 0);
}



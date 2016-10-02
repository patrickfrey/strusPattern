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
#include "tokenMarkup.hpp"
#include "patternMatcher.hpp"
#include "patternLexer.hpp"
#include "patternMatcherProgram.hpp"
#include "strus/base/dll_tags.hpp"
#include "internationalization.hpp"
#include "errorUtils.hpp"

using namespace strus;
static bool g_intl_initialized = false;

DLL_PUBLIC PatternMatcherInterface* strus::createPatternMatcher_stream( ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return new PatternMatcher( errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating token pattern match interface: %s"), *errorhnd, 0);
}

DLL_PUBLIC PatternLexerInterface* strus::createPatternLexer_stream( ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return new PatternLexer( errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating char regex match interface: %s"), *errorhnd, 0);
}

DLL_PUBLIC TokenMarkupInstanceInterface* strus::createTokenMarkupInstance_stream( ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return new TokenMarkupInstance( errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating char regex match interface: %s"), *errorhnd, 0);
}


DLL_PUBLIC PatternMatcherProgramInterface* strus::createPatternMatcherProgram_stream(
		const PatternMatcherInterface* tpm,
		const PatternLexerInterface* crm,
		ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return new PatternMatcherProgram( tpm, crm, errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating the standard pattern match program interface: %s"), *errorhnd, 0);
}



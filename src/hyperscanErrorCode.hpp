/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_LEVELDB_ERRORCODES_HPP_INCLUDED
#define _STRUS_LEVELDB_ERRORCODES_HPP_INCLUDED
#include "strus/errorCodes.hpp"
#include "hs_common.h"

static strus::ErrorCode hyperscanErrorCode( const strus::ErrorOperation& errop, hs_error_t hs_error)
{
	strus::ErrorCause cause = strus::ErrorCauseUnknown;

	switch (hs_error)
	{
		case HS_INVALID:		cause = strus::ErrorCauseInvalidArgument; break;
		case HS_NOMEM:			cause = strus::ErrorCauseOutOfMem; break;
		case HS_SCAN_TERMINATED:	cause = strus::ErrorCauseUnknown; break;
		case HS_COMPILER_ERROR:		cause = strus::ErrorCauseSyntax; break;
		case HS_DB_VERSION_ERROR:	cause = strus::ErrorCauseVersionMismatch; break;
		case HS_DB_PLATFORM_ERROR:	cause = strus::ErrorCausePlatformIncompatibility; break;
		case HS_DB_MODE_ERROR:		cause = strus::ErrorCauseNotImplemented; break;
		case HS_BAD_ALIGN:		cause = strus::ErrorCauseInvalidArgument; break;
		case HS_BAD_ALLOC:		cause = strus::ErrorCauseLogicError; break;
		case HS_SCRATCH_IN_USE:		cause = strus::ErrorCauseLogicError; break;
		case HS_ARCH_ERROR:		cause = strus::ErrorCausePlatformRequirements; break;
		case HS_INSUFFICIENT_SPACE:	cause = strus::ErrorCauseBufferOverflow; break;
	}
	return strus::ErrorCode(strus::StrusComponentPattern,errop,cause);
}
#endif


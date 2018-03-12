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

static strus::ErrorCode hyperscanErrorCode( hs_error_t hs_error)
{
	strus::ErrorCode cause = strus::ErrorCodeUnknown;

	switch (hs_error)
	{
		case HS_INVALID:		cause = strus::ErrorCodeInvalidArgument; break;
		case HS_NOMEM:			cause = strus::ErrorCodeOutOfMem; break;
		case HS_SCAN_TERMINATED:	cause = strus::ErrorCodeUnknown; break;
		case HS_COMPILER_ERROR:		cause = strus::ErrorCodeSyntax; break;
		case HS_DB_VERSION_ERROR:	cause = strus::ErrorCodeVersionMismatch; break;
		case HS_DB_PLATFORM_ERROR:	cause = strus::ErrorCodePlatformIncompatibility; break;
		case HS_DB_MODE_ERROR:		cause = strus::ErrorCodeNotImplemented; break;
		case HS_BAD_ALIGN:		cause = strus::ErrorCodeInvalidArgument; break;
		case HS_BAD_ALLOC:		cause = strus::ErrorCodeLogicError; break;
		case HS_SCRATCH_IN_USE:		cause = strus::ErrorCodeLogicError; break;
		case HS_ARCH_ERROR:		cause = strus::ErrorCodePlatformRequirements; break;
		case HS_INSUFFICIENT_SPACE:	cause = strus::ErrorCodeBufferOverflow; break;
	}
	return cause;
}
#endif


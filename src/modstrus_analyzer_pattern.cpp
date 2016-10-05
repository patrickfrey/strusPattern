/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/lib/pattern.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/dll_tags.hpp"
#include "strus/analyzerModule.hpp"
#include "internationalization.hpp"
#include "errorUtils.hpp"
#include "hs.h"

static const strus::PatternLexerConstructor lexer =
{
	"stream", strus::createPatternLexer_stream
};

static const strus::PatternMatcherConstructor matcher =
{
	"stream", strus::createPatternMatcher_stream
};

static const char* intel_hyperscan_license =
	" Copyright (c) 2015, Intel Corporation\n\n"
	" Redistribution and use in source and binary forms, with or without\n"
	" modification, are permitted provided that the following conditions are met:\n"
	"  * Redistributions of source code must retain the above copyright notice,\n"
	"    this list of conditions and the following disclaimer.\n"
	"  * Redistributions in binary form must reproduce the above copyright\n"
	"    notice, this list of conditions and the following disclaimer in the\n"
	"    documentation and/or other materials provided with the distribution.\n"
	"  * Neither the name of Intel Corporation nor the names of its contributors\n"
	"    may be used to endorse or promote products derived from this software\n"
	"    without specific prior written permission.\n\n"
	" THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
	" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
	" IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
	" ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE\n"
	" LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n"
	" CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n"
	" SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n"
	" INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n"
	" CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n"
	" ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n"
	" POSSIBILITY OF SUCH DAMAGE.\n";

#define xstr(a) str(a)
#define str(a) #a

static const char* intel_hyperscan_version =
	""  str(HS_VERSION_STRING) "\n\tCopyright (c) 2015, Intel Corporation";


extern "C" DLL_PUBLIC strus::AnalyzerModule entryPoint;

strus::AnalyzerModule entryPoint( lexer, matcher, intel_hyperscan_version, intel_hyperscan_license);





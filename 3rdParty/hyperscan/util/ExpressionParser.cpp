
#line 1 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util/ExpressionParser.rl"
/*
 * Copyright (c) 2015, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "ExpressionParser.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "ue2common.h"
#include "hs_compile.h"


using std::string;

namespace { // anon

enum ParamKey {
    PARAM_NONE,
    PARAM_MIN_OFFSET,
    PARAM_MAX_OFFSET,
    PARAM_MIN_LENGTH
};


#line 58 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util//ExpressionParser.cpp"
static const char _ExpressionParser_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 7, 1, 
	8, 2, 6, 0
};

static const char _ExpressionParser_key_offsets[] = {
	0, 0, 2, 4, 6, 7, 8, 9, 
	10, 11, 12, 13, 14, 15, 17, 22, 
	25, 26, 27, 29, 30, 31, 32, 33, 
	34, 35, 36, 37, 38, 39, 50
};

static const char _ExpressionParser_trans_keys[] = {
	32, 109, 32, 109, 97, 105, 120, 95, 
	111, 102, 102, 115, 101, 116, 61, 48, 
	57, 32, 44, 125, 48, 57, 32, 44, 
	125, 110, 95, 108, 111, 101, 110, 103, 
	116, 104, 102, 102, 115, 101, 116, 56, 
	72, 76, 105, 109, 115, 123, 79, 80, 
	86, 87, 0
};

static const char _ExpressionParser_single_lengths[] = {
	0, 2, 2, 2, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 0, 3, 3, 
	1, 1, 2, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 7, 0
};

static const char _ExpressionParser_range_lengths[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 1, 1, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 2, 0
};

static const char _ExpressionParser_index_offsets[] = {
	0, 0, 3, 6, 9, 11, 13, 15, 
	17, 19, 21, 23, 25, 27, 29, 34, 
	38, 40, 42, 45, 47, 49, 51, 53, 
	55, 57, 59, 61, 63, 65, 75
};

static const char _ExpressionParser_indicies[] = {
	1, 2, 0, 3, 4, 0, 5, 6, 
	0, 7, 0, 8, 0, 9, 0, 10, 
	0, 11, 0, 12, 0, 13, 0, 14, 
	0, 15, 0, 16, 0, 17, 18, 20, 
	19, 0, 17, 18, 20, 0, 21, 0, 
	22, 0, 23, 24, 0, 25, 0, 26, 
	0, 27, 0, 28, 0, 29, 0, 30, 
	0, 31, 0, 32, 0, 33, 0, 34, 
	0, 35, 35, 35, 35, 35, 35, 36, 
	35, 35, 0, 0, 0
};

static const char _ExpressionParser_trans_targs[] = {
	0, 2, 3, 2, 3, 4, 16, 5, 
	6, 7, 8, 9, 10, 11, 12, 13, 
	14, 15, 1, 14, 30, 17, 18, 19, 
	24, 20, 21, 22, 23, 12, 25, 26, 
	27, 28, 12, 29, 1
};

static const char _ExpressionParser_trans_actions[] = {
	15, 13, 13, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 9, 0, 
	17, 0, 5, 1, 5, 0, 0, 0, 
	0, 0, 0, 0, 0, 11, 0, 0, 
	0, 0, 7, 3, 0
};

static const char _ExpressionParser_eof_actions[] = {
	0, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 0, 0
};

static const int ExpressionParser_start = 29;
static const int ExpressionParser_first_final = 29;
static const int ExpressionParser_error = 0;

static const int ExpressionParser_en_main = 29;


#line 103 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util/ExpressionParser.rl"


} // namespace

static
void initExt(hs_expr_ext *ext) {
    memset(ext, 0, sizeof(*ext));
    ext->max_offset = MAX_OFFSET;
}

bool readExpression(const std::string &input, std::string &expr,
                    unsigned int *flags, hs_expr_ext *ext,
                    bool *must_be_ordered) {
    assert(flags);
    assert(ext);

    // Init flags and ext params.
    *flags = 0;
    initExt(ext);
    if (must_be_ordered) {
        *must_be_ordered = false;
    }

    // Extract expr, which is easier to do in straight C++ than with Ragel.
    if (input.empty() || input[0] != '/') {
        return false;
    }
    size_t end = input.find_last_of('/');
    if (end == string::npos || end == 0) {
        return false;
    }
    expr = input.substr(1, end - 1);

    // Use a Ragel scanner to handle flags and params.
    const char *p = input.c_str() + end + 1;
    const char *pe = input.c_str() + input.size();
    UNUSED const char *eof = pe;
    UNUSED const char *ts = p, *te = p;
    int cs;
    UNUSED int act;

    assert(p);
    assert(pe);

    // For storing integers as they're scanned.
    u64a num = 0;
    enum ParamKey key = PARAM_NONE;

    
#line 196 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util//ExpressionParser.cpp"
	{
	cs = ExpressionParser_start;
	}

#line 201 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util//ExpressionParser.cpp"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _ExpressionParser_trans_keys + _ExpressionParser_key_offsets[cs];
	_trans = _ExpressionParser_index_offsets[cs];

	_klen = _ExpressionParser_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _ExpressionParser_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _ExpressionParser_indicies[_trans];
	cs = _ExpressionParser_trans_targs[_trans];

	if ( _ExpressionParser_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _ExpressionParser_actions + _ExpressionParser_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 57 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util/ExpressionParser.rl"
	{
        num = (num * 10) + ((*p) - '0');
    }
	break;
	case 1:
#line 61 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util/ExpressionParser.rl"
	{
        switch ((*p)) {
            case 'i': *flags |= HS_FLAG_CASELESS; break;
            case 's': *flags |= HS_FLAG_DOTALL; break;
            case 'm': *flags |= HS_FLAG_MULTILINE; break;
            case 'H': *flags |= HS_FLAG_SINGLEMATCH; break;
            case 'O':
                if (must_be_ordered) {
                    *must_be_ordered = true;
                }
                break;
            case 'V': *flags |= HS_FLAG_ALLOWEMPTY; break;
            case 'W': *flags |= HS_FLAG_UCP; break;
            case '8': *flags |= HS_FLAG_UTF8; break;
            case 'P': *flags |= HS_FLAG_PREFILTER; break;
            case 'L': *flags |= HS_FLAG_SOM_LEFTMOST; break;
            default: {p++; goto _out; }
        }
    }
	break;
	case 2:
#line 81 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util/ExpressionParser.rl"
	{
        switch (key) {
            case PARAM_MIN_OFFSET:
                ext->flags |= HS_EXT_FLAG_MIN_OFFSET;
                ext->min_offset = num;
                break;
            case PARAM_MAX_OFFSET:
                ext->flags |= HS_EXT_FLAG_MAX_OFFSET;
                ext->max_offset = num;
                break;
            case PARAM_MIN_LENGTH:
                ext->flags |= HS_EXT_FLAG_MIN_LENGTH;
                ext->min_length = num;
                break;
            case PARAM_NONE:
            default:
                // No key specified, syntax invalid.
                return false;
        }
    }
	break;
	case 3:
#line 153 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util/ExpressionParser.rl"
	{ key = PARAM_MIN_OFFSET; }
	break;
	case 4:
#line 154 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util/ExpressionParser.rl"
	{ key = PARAM_MAX_OFFSET; }
	break;
	case 5:
#line 155 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util/ExpressionParser.rl"
	{ key = PARAM_MIN_LENGTH; }
	break;
	case 6:
#line 157 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util/ExpressionParser.rl"
	{num = 0;}
	break;
	case 7:
#line 158 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util/ExpressionParser.rl"
	{ key = PARAM_NONE; }
	break;
	case 8:
#line 163 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util/ExpressionParser.rl"
	{ return false; }
	break;
#line 350 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util//ExpressionParser.cpp"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _ExpressionParser_actions + _ExpressionParser_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 8:
#line 163 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util/ExpressionParser.rl"
	{ return false; }
	break;
#line 370 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util//ExpressionParser.cpp"
		}
	}
	}

	_out: {}
	}

#line 168 "/home/patrick/Projects/github/strusStream/3rdParty/hyperscan/util/ExpressionParser.rl"


    DEBUG_PRINTF("expr='%s', flags=%u\n", expr.c_str(), *flags);

    return (cs != ExpressionParser_error) && (p == pe);
}

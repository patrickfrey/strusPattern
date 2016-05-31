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

/** \file
 * \brief Platform specific popcount functions
 */

#ifndef POPCOUNT_H_075D843B4545B6
#define POPCOUNT_H_075D843B4545B6

#include "ue2common.h"

// We have a native popcount where the compiler has defined __POPCNT__.
#if defined(__POPCNT__)
#define HAVE_POPCOUNT_INSTR
#endif

#if defined(_WIN32) && defined(__AVX__) // TODO: fix win preproc
#define HAVE_POPCOUNT_INSTR
#define __builtin_popcount __popcnt
#define __builtin_popcountll __popcnt64
#endif

static really_inline
u32 popcount32(u32 x) {
#if defined(HAVE_POPCOUNT_INSTR)
    // Single-instruction builtin.
    return (u32)__builtin_popcount(x);
#else
    // Fast branch-free version from bit-twiddling hacks as most Intel
    // processors do not have a POPCNT instruction.
    x -= (x >> 1) & 0x55555555;
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    return (((x + (x >> 4)) & 0xf0f0f0f) * 0x1010101) >> 24;
#endif
}

static really_inline
u32 popcount64(u64a x) {
#if defined(HAVE_POPCOUNT_INSTR)
    // Single-instruction builtin.
    return (u32)__builtin_popcountll(x);
#elif defined(ARCH_X86_64)
    // Fast branch-free version from bit-twiddling hacks as most Intel
    // processors do not have a POPCNT instruction.
    x -= (x >> 1) & 0x5555555555555555;
    x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
    x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f;
    return (x * 0x0101010101010101) >> 56;
#else
    // Synthesise from two 32-bit cases.
    return popcount32(x >> 32) + popcount32(x);
#endif
}

#endif /* POPCOUNT_H_075D843B4545B6 */


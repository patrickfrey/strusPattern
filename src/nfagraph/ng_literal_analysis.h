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
 * \brief Literal analysis and scoring.
 */

#ifndef NG_LITERAL_ANALYSIS_H
#define NG_LITERAL_ANALYSIS_H

#include <set>
#include <vector>

#include "ng_holder.h"
#include "util/ue2string.h"

namespace ue2 {

#define NO_LITERAL_AT_EDGE_SCORE  10000000ULL

/* Score for special-to-special edges */
#define INVALID_EDGE_CAP 100000000ULL

class NGHolder;

/**
 * Fetch the literal set for a given vertex, returning it in \p s. Note: does
 * NOT take into account any constraints due to streaming mode requirements.
 *
 * if only_first_encounter is requested, the output set may drop literals
 * generated by revisiting the destination vertex.
 */
std::set<ue2_literal> getLiteralSet(const NGHolder &g, const NFAVertex &v,
                                    bool only_first_encounter = true);
std::set<ue2_literal> getLiteralSet(const NGHolder &g, const NFAEdge &e);

/** Score all the edges in the given graph, returning them in \p scores indexed
 * by edge_index. */
std::vector<u64a> scoreEdges(const NGHolder &h);

/** Returns a score for a literal set. Lower scores are better. */
u64a scoreSet(const std::set<ue2_literal> &s);

/** Compress a literal set to fewer literals. */
u64a compressAndScore(std::set<ue2_literal> &s);

bool splitOffLeadingLiteral(const NGHolder &g, ue2_literal *lit_out,
                            NGHolder *rhs);

bool splitOffAnchoredLeadingLiteral(const NGHolder &g, ue2_literal *lit_out,
                            NGHolder *rhs);

bool getTrailingLiteral(const NGHolder &g, ue2_literal *lit_out);

} // namespace ue2

#endif

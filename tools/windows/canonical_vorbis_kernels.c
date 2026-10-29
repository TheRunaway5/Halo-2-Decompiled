/*
 * Source-readable Vorbis binary32 arithmetic seams.
 *
 * Algorithm: libvorbis 1.3.7, lib/mdct.c:346-393 and lib/block.c:774-803.
 * Source archive SHA-256:
 * b33cc4934322bcbf6efcbacf49e3ca01aadbea4114ec9589d1b1e9d20f72954b
 * Operation order: retained Linux libvorbis.so.0.4.9, SHA-256:
 * 8b2d8cbebd41533bcc00150000ae98d7d171e8caa257bcb6f7ee7eea80ea5849
 *
 * Compile with contraction and reassociation disabled (-ffp-contract=off,
 * no -ffast-math). Only explicit fmaf calls have one final rounding.
 * These helpers do not require AVX, FMA instructions, or Linux struct layouts.
 *
 * Derived from the Xiph.Org Foundation's BSD-licensed Vorbis algorithm:
 * Copyright (c) 2002-2020 Xiph.org Foundation.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Xiph.org Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "canonical_vorbis_kernels.h"

#include <float.h>
#include <math.h>

#if FLT_RADIX != 2 || FLT_MANT_DIG != 24 || FLT_MAX_EXP != 128
#error "Canonical Vorbis arithmetic requires IEEE binary32 float"
#endif
#if defined(FLT_EVAL_METHOD) && FLT_EVAL_METHOD != 0
#error "Canonical Vorbis arithmetic requires binary32 intermediate evaluation"
#endif
#ifdef __FAST_MATH__
#error "Canonical Vorbis arithmetic must preserve rounding and reassociation"
#endif

void h2_canonical_vorbis_bitreverse_pair(
    const float x0[2], const float x1[2], const float twiddle[2],
    float low[2], float high[2]) {
    const float difference1 = x0[1] - x1[1];
    const float sum0 = x0[0] + x1[0];
    const float sum1 = x0[1] + x1[1];
    const float difference0 = x0[0] - x1[0];
    const float product1 = difference1 * twiddle[1];
    const float product0 = difference1 * twiddle[0];
    const float rotated0 = fmaf(sum0, twiddle[0], product1);
    const float rotated1 = fmaf(sum0, twiddle[1], -product0);

    /* mdct_backward+0x250..0x271: the half multiply contracts here too. */
    low[0] = fmaf(sum1, 0.5f, rotated0);
    high[0] = fmaf(sum1, 0.5f, -rotated0);
    low[1] = fmaf(difference0, 0.5f, rotated1);
    high[1] = fmaf(-difference0, 0.5f, rotated1);
}

void h2_canonical_vorbis_overlap(
    float *pcm, const float *incoming, const float *window, int count) {
    for (int i = 0; i < count; ++i) {
        /* blockin+0x510: incoming product rounds before the overlap FMA. */
        const float product = incoming[i] * window[i];
        pcm[i] = fmaf(pcm[i], window[count - i - 1], product);
    }
}

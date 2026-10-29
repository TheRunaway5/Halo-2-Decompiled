/*
 * Readable scalar operation graph for libvorbis 1.3.7 inverse MDCT.
 * Derived from the same BSD-licensed source and pinned Linux compiler output
 * identified in canonical_vorbis_kernels.c. That file contains the license.
 *
 * The 32-point kernel retains the canonical reassociation across the upstream
 * 32/16/8-point inlined butterflies. Every temporary is a rounded binary32
 * operation; every fmaf is a single final rounding. No ISA emulation is used.
 * Compile with -ffp-contract=off and without -ffast-math.
 */
#include "canonical_vorbis_kernels.h"
#include <math.h>
#ifdef __FAST_MATH__
#error "Canonical Vorbis arithmetic must preserve rounding and reassociation"
#endif

static void butterfly32(float x[32]) {
    const float x30 = x[30];
    const float x14 = x[14];
    const float difference30_14 = x30 - x14;
    const float x31 = x[31];
    const float x15 = x[15];
    const float difference31_15 = x31 - x15;
    const float sum30_14 = x30 + x14;
    const float sum31_15 = x31 + x15;
    const float x28 = x[28];
    const float x12 = x[12];
    const float difference28_12 = x28 - x12;
    const float x29 = x[29];
    const float x13 = x[13];
    const float difference29_13 = x29 - x13;
    const float sum28_12 = x28 + x12;
    const float sum29_13 = x29 + x13;
    const float stage0 = difference29_13 * 0x1.87de2ap-2f;
    const float stage1 = fmaf(difference28_12, 0x1.d906bcp-1f, -stage0);
    const float stage2 = difference29_13 * 0x1.d906bcp-1f;
    const float stage3 = fmaf(difference28_12, 0x1.87de2ap-2f, stage2);
    const float x26 = x[26];
    const float x10 = x[10];
    const float difference26_10 = x26 - x10;
    const float x27 = x[27];
    const float x11 = x[11];
    const float stage4 = difference26_10 - x11;
    const float sum26_10 = x26 + x10;
    const float sum27_11 = x27 + x11;
    const float stage5 = difference26_10 - x27;
    const float stage6 = x11 + stage5;
    const float stage7 = x27 + stage4;
    const float x24 = x[24];
    const float x8 = x[8];
    const float difference24_8 = x24 - x8;
    const float x25 = x[25];
    const float x9 = x[9];
    const float difference25_9 = x25 - x9;
    const float sum24_8 = x24 + x8;
    const float sum25_9 = x25 + x9;
    const float stage8 = difference25_9 * 0x1.d906bcp-1f;
    const float stage9 = fmaf(difference24_8, 0x1.87de2ap-2f, -stage8);
    const float stage10 = difference24_8 * 0x1.d906bcp-1f;
    const float stage11 = fmaf(difference25_9, 0x1.87de2ap-2f, stage10);
    const float x22 = x[22];
    const float x6 = x[6];
    const float stage12 = difference31_15 - x6;
    const float x7 = x[7];
    const float x23 = x[23];
    const float stage13 = difference30_14 - x23;
    const float sum22_6 = x22 + x6;
    const float sum7_23 = x7 + x23;
    const float x4 = x[4];
    const float x20 = x[20];
    const float difference4_20 = x4 - x20;
    const float x5 = x[5];
    const float x21 = x[21];
    const float difference5_21 = x5 - x21;
    const float sum4_20 = x4 + x20;
    const float sum5_21 = x5 + x21;
    const float stage14 = difference4_20 * 0x1.87de2ap-2f;
    const float stage15 = fmaf(difference5_21, 0x1.d906bcp-1f, stage14);
    const float stage16 = difference4_20 * 0x1.d906bcp-1f;
    const float stage17 = stage3 + stage16;
    const float stage18 = fmaf(difference5_21, 0x1.87de2ap-2f, -stage16);
    const float x2 = x[2];
    const float x18 = x[18];
    const float x3 = x[3];
    const float x19 = x[19];
    const float difference3_19 = x3 - x19;
    const float stage19 = difference3_19 - x18;
    const float sum2_18 = x2 + x18;
    const float sum3_19 = x3 + x19;
    const float stage20 = x2 + stage19;
    const float stage21 = stage20 * 0x1.6a09e6p-1f;
    const float stage22 = difference3_19 - x2;
    const float stage23 = x18 + stage22;
    const float stage24 = stage23 * 0x1.6a09e6p-1f;
    const float x0 = x[0];
    const float x16 = x[16];
    const float difference0_16 = x0 - x16;
    const float x1 = x[1];
    const float x17 = x[17];
    const float difference1_17 = x1 - x17;
    const float sum0_16 = x0 + x16;
    const float sum1_17 = x1 + x17;
    const float stage25 = difference0_16 * 0x1.d906bcp-1f;
    const float stage26 = fmaf(difference1_17, 0x1.87de2ap-2f, stage25);
    const float stage27 = stage8 + stage26;
    const float stage28 = difference0_16 * 0x1.87de2ap-2f;
    const float stage29 = fmaf(difference1_17, 0x1.d906bcp-1f, -stage28);
    const float stage30 = stage29 - stage11;
    const float stage31 = stage9 + stage26;
    const float stage32 = stage11 + stage29;
    const float stage33 = fmaf(-difference24_8, 0x1.87de2ap-2f, stage30);
    const float stage34 = stage27 + stage33;
    const float stage35 = stage30 - stage26;
    const float stage36 = stage9 + stage35;
    const float stage37 = fmaf(stage6, 0x1.6a09e6p-1f, stage21);
    const float stage38 = fmaf(stage7, 0x1.6a09e6p-1f, stage24);
    const float stage39 = stage1 - stage15;
    const float stage40 = stage1 + stage15;
    const float stage41 = stage3 + stage18;
    const float stage42 = stage39 - stage3;
    const float stage43 = stage18 + stage42;
    const float stage44 = stage43 * 0x1.6a09e6p-1f;
    const float stage45 = fmaf(-difference5_21, 0x1.87de2ap-2f, stage39);
    const float stage46 = stage17 + stage45;
    const float stage47 = stage46 * 0x1.6a09e6p-1f;
    const float stage48 = difference30_14 - x7;
    const float stage49 = x23 + stage48;
    const float stage50 = stage24 + stage49;
    const float stage51 = difference31_15 - x22;
    const float stage52 = x6 + stage51;
    const float stage53 = fmaf(stage6, 0x1.6a09e6p-1f, stage52);
    const float stage54 = x7 + stage13;
    const float stage55 = x22 + stage12;
    const float stage56 = fmaf(-stage7, 0x1.6a09e6p-1f, stage50);
    const float stage57 = fmaf(stage7, 0x1.6a09e6p-1f, stage49);
    const float stage58 = stage57 - stage24;
    const float stage59 = fmaf(stage34, 0x1.6a09e6p-1f, stage44);
    const float stage60 = stage56 + stage59;
    const float stage61 = stage56 - stage59;
    const float stage62 = stage47 + stage58;
    const float stage63 = stage21 + stage52;
    const float stage64 = fmaf(-stage6, 0x1.6a09e6p-1f, stage63);
    const float stage65 = stage44 + stage64;
    const float stage66 = fmaf(-stage36, 0x1.6a09e6p-1f, stage62);
    const float stage67 = fmaf(stage36, 0x1.6a09e6p-1f, stage58);
    const float stage68 = stage67 - stage47;
    const float stage69 = fmaf(stage36, 0x1.6a09e6p-1f, stage47);
    const float stage70 = stage53 - stage21;
    const float stage71 = fmaf(-stage34, 0x1.6a09e6p-1f, stage65);
    const float stage72 = fmaf(stage34, 0x1.6a09e6p-1f, stage64);
    const float stage73 = stage72 - stage44;
    const float stage74 = stage69 + stage70;
    const float stage75 = stage70 - stage69;
    const float stage76 = stage37 + stage54;
    const float stage77 = stage54 - stage37;
    const float stage78 = stage31 + stage40;
    const float stage79 = stage40 - stage31;
    const float stage80 = stage76 + stage78;
    const float stage81 = stage76 - stage78;
    const float stage82 = stage41 - stage32;
    const float stage83 = stage55 - stage38;
    const float stage84 = stage77 + stage82;
    const float stage85 = stage32 - stage41;
    const float stage86 = stage77 + stage85;
    const float stage87 = stage32 + stage41;
    const float stage88 = stage38 + stage55;
    const float stage89 = stage79 + stage83;
    const float stage90 = stage31 - stage40;
    const float stage91 = stage83 + stage90;
    const float stage92 = stage87 + stage88;
    const float stage93 = stage88 - stage87;
    const float stage94 = sum1_17 - sum25_9;
    const float stage95 = stage94 - sum24_8;
    const float stage96 = sum24_8 + sum0_16;
    const float stage97 = sum25_9 + sum1_17;
    const float stage98 = sum0_16 + stage95;
    const float stage99 = stage94 - sum0_16;
    const float stage100 = sum24_8 + stage99;
    const float stage101 = sum26_10 + sum2_18;
    const float stage102 = sum27_11 + sum3_19;
    const float stage103 = sum28_12 - sum4_20;
    const float stage104 = stage103 - sum5_21;
    const float stage105 = sum28_12 + sum4_20;
    const float stage106 = sum29_13 + sum5_21;
    const float stage107 = stage103 - sum29_13;
    const float stage108 = sum5_21 + stage107;
    const float stage109 = stage108 * 0x1.6a09e6p-1f;
    const float stage110 = sum29_13 + stage104;
    const float stage111 = stage110 * 0x1.6a09e6p-1f;
    const float stage112 = sum30_14 - sum22_6;
    const float stage113 = stage112 - sum27_11;
    const float stage114 = sum31_15 - sum7_23;
    const float stage115 = stage114 - sum2_18;
    const float stage116 = sum30_14 + sum22_6;
    const float stage117 = sum31_15 + sum7_23;
    const float stage118 = sum3_19 + stage113;
    const float stage119 = stage112 - sum3_19;
    const float stage120 = sum27_11 + stage119;
    const float stage121 = fmaf(stage98, 0x1.6a09e6p-1f, stage109);
    const float stage122 = stage118 + stage121;
    const float stage123 = stage118 - stage121;
    const float stage124 = stage111 + stage120;
    const float stage125 = stage114 - sum26_10;
    const float stage126 = sum2_18 + stage125;
    const float stage127 = stage109 + stage126;
    const float stage128 = fmaf(-stage100, 0x1.6a09e6p-1f, stage124);
    const float stage129 = fmaf(stage100, 0x1.6a09e6p-1f, stage120);
    const float stage130 = stage129 - stage111;
    const float stage131 = fmaf(stage100, 0x1.6a09e6p-1f, stage111);
    const float stage132 = sum26_10 + stage115;
    const float stage133 = fmaf(-stage98, 0x1.6a09e6p-1f, stage127);
    const float stage134 = fmaf(stage98, 0x1.6a09e6p-1f, stage126);
    const float stage135 = stage134 - stage109;
    const float stage136 = stage131 + stage132;
    const float stage137 = stage132 - stage131;
    const float stage138 = stage101 + stage116;
    const float stage139 = stage116 - stage101;
    const float stage140 = stage96 + stage105;
    const float stage141 = stage138 + stage140;
    const float stage142 = stage138 - stage140;
    const float stage143 = stage139 - stage97;
    const float stage144 = stage117 - stage102;
    const float stage145 = stage144 - stage96;
    const float stage146 = stage106 + stage143;
    const float stage147 = stage139 - stage106;
    const float stage148 = stage97 + stage147;
    const float stage149 = stage97 + stage106;
    const float stage150 = stage102 + stage117;
    const float stage151 = stage105 + stage145;
    const float stage152 = stage144 - stage105;
    const float stage153 = stage96 + stage152;
    const float stage154 = stage149 + stage150;
    const float stage155 = stage150 - stage149;
    x[0] = stage66;
    x[1] = stage73;
    x[2] = stage68;
    x[3] = stage71;
    x[4] = stage61;
    x[5] = stage75;
    x[6] = stage60;
    x[7] = stage74;
    x[8] = stage84;
    x[9] = stage91;
    x[10] = stage86;
    x[11] = stage89;
    x[12] = stage81;
    x[13] = stage93;
    x[14] = stage80;
    x[15] = stage92;
    x[16] = stage128;
    x[17] = stage135;
    x[18] = stage130;
    x[19] = stage133;
    x[20] = stage123;
    x[21] = stage137;
    x[22] = stage122;
    x[23] = stage136;
    x[24] = stage146;
    x[25] = stage153;
    x[26] = stage148;
    x[27] = stage151;
    x[28] = stage142;
    x[29] = stage155;
    x[30] = stage141;
    x[31] = stage154;
}

/* Upstream first/generic stages share this four-pair rotation. */
static void butterfly_stage(const float *twiddle, float *x,
                            int points, int twiddle_step) {
    for (int offset = (points >> 1) - 8; offset >= 0; offset -= 8) {
        float *upper = x + (points >> 1) + offset;
        float *lower = x + offset;
        for (int i = 6; i >= 0; i -= 2) {
            const float real = upper[i] - lower[i];
            const float imaginary = upper[i + 1] - lower[i + 1];
            upper[i] += lower[i];
            upper[i + 1] += lower[i + 1];
            const float real_cosine = real * twiddle[0];
            lower[i] = fmaf(imaginary, twiddle[1], real_cosine);
            const float real_sine = real * twiddle[1];
            lower[i + 1] = fmaf(imaginary, twiddle[0], -real_sine);
            twiddle += twiddle_step;
        }
    }
}

static void butterflies(H2CanonicalVorbisMdctLookup *lookup,
                         float *x, int points) {
    int stages = lookup->log2n - 5;
    if (--stages > 0)
        butterfly_stage(lookup->trig, x, points, 4);
    for (int i = 1; --stages > 0; ++i)
        for (int j = 0; j < (1 << i); ++j)
            butterfly_stage(lookup->trig, x + (points >> i) * j,
                            points >> i, 4 << i);
    for (int j = 0; j < points; j += 32)
        butterfly32(x + j);
}

static void bitreverse(H2CanonicalVorbisMdctLookup *lookup, float *output) {
    const int *bit = lookup->bitrev;
    const float *twiddle = lookup->trig + lookup->n;
    float *low = output;
    float *high = output + (lookup->n >> 1);
    float *input = high;
    do {
        high -= 4;
        h2_canonical_vorbis_bitreverse_pair(
            input + bit[0], input + bit[1], twiddle, low, high + 2);
        h2_canonical_vorbis_bitreverse_pair(
            input + bit[2], input + bit[3], twiddle + 2, low + 2, high);
        bit += 4;
        twiddle += 4;
        low += 4;
    } while (low < high);
}

void h2_canonical_vorbis_mdct_backward(
    H2CanonicalVorbisMdctLookup *lookup, float *input, float *output) {
    const int half = lookup->n >> 1;
    const int quarter = lookup->n >> 2;
    const float *twiddle = lookup->trig + quarter;
    float *out = output + half + quarter;

    /* Canonical inverse pre-rotation, first and second quadrants. */
    for (int offset = half - 7; offset >= 0; offset -= 8) {
        const float *in = input + offset;
        out -= 4;
        const float product0 = in[0] * twiddle[2];
        out[0] = fmaf(-in[2], twiddle[3], -product0);
        const float product1 = in[2] * twiddle[2];
        out[1] = fmaf(in[0], twiddle[3], -product1);
        const float product2 = in[4] * twiddle[0];
        out[2] = fmaf(-in[6], twiddle[1], -product2);
        const float product3 = in[6] * twiddle[0];
        out[3] = fmaf(in[4], twiddle[1], -product3);
        twiddle += 4;
    }

    twiddle = lookup->trig + quarter;
    out = output + half + quarter;
    for (int offset = half - 8; offset >= 0; offset -= 8) {
        const float *in = input + offset;
        twiddle -= 4;
        const float product0 = in[6] * twiddle[2];
        out[0] = fmaf(in[4], twiddle[3], product0);
        const float product1 = in[6] * twiddle[3];
        out[1] = fmaf(in[4], twiddle[2], -product1);
        const float product2 = in[2] * twiddle[0];
        out[2] = fmaf(in[0], twiddle[1], product2);
        const float product3 = in[2] * twiddle[1];
        out[3] = fmaf(in[0], twiddle[0], -product3);
        out += 4;
    }

    butterflies(lookup, output + half, half);
    bitreverse(lookup, output);

    /* Final rotation, then the original signed/reflected output copies. */
    float *left = output + half + quarter;
    float *right = left;
    const float *in = output;
    twiddle = lookup->trig + half;
    do {
        left -= 4;
        for (int i = 0; i < 4; ++i) {
            const float imaginary_cosine = in[2 * i + 1] * twiddle[2 * i];
            left[3 - i] = fmaf(in[2 * i], twiddle[2 * i + 1],
                               -imaginary_cosine);
            const float imaginary_sine = in[2 * i + 1] * twiddle[2 * i + 1];
            right[i] = fmaf(-in[2 * i], twiddle[2 * i], -imaginary_sine);
        }
        right += 4;
        in += 8;
        twiddle += 8;
    } while (in < left);

    in = output + half + quarter;
    left = output + quarter;
    right = left;
    do {
        left -= 4;
        in -= 4;
        for (int i = 0; i < 4; ++i) {
            left[3 - i] = in[3 - i];
            right[i] = -in[3 - i];
        }
        right += 4;
    } while (right < in);

    in = output + half + quarter;
    left = output + half + quarter;
    right = output + half;
    do {
        left -= 4;
        for (int i = 0; i < 4; ++i)
            left[i] = in[3 - i];
        in += 4;
    } while (left > right);
}

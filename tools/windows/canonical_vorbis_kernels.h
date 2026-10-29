#ifndef HALO2_CANONICAL_VORBIS_KERNELS_H
#define HALO2_CANONICAL_VORBIS_KERNELS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Canonical binary32 seams for the portable loading decoder. */
typedef struct {
    int n;
    int log2n;
    float *trig;
    int *bitrev;
    float scale;
} H2CanonicalVorbisMdctLookup;

/* Use a validated libvorbis1.3.7 power-of-two lookup (64..8192 points).
 * Input contains n/2 floats and output has n floats; in-place is supported. */
void h2_canonical_vorbis_mdct_backward(
    H2CanonicalVorbisMdctLookup *lookup, float *input, float *output);

void h2_canonical_vorbis_bitreverse_pair(
    const float x0[2], const float x1[2], const float twiddle[2],
    float low[2], float high[2]);

void h2_canonical_vorbis_overlap(
    float *pcm, const float *incoming, const float *window, int count);

#ifdef __cplusplus
}
#endif

#endif

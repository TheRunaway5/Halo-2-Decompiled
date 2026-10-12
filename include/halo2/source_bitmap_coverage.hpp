#pragma once
#include <cstdint>

namespace halo2 {
struct SourceBitmapCoverage {
    float reciprocal{}; // Actual16B90 context+28, rounded DIVSS result.
    float adjustment{}; // Actual context+2C, x87 logarithm and literal multiplier.
};
// Complete pure16B90 coverage producer. The input is the actual caller metric,
// not a guessed screen-space size or a selected mip. Positive inputs use1/x;
// all other comparison outcomes use literal100000. A zero reciprocal rejects
// the unproved exceptional logarithm path. No residency is activated.
SourceBitmapCoverage source_bitmap_coverage(float actual_metric);
} // namespace halo2

#include "halo2/source_bitmap_coverage.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
SourceBitmapCoverage source_bitmap_coverage(float metric) {
    const float reciprocal=metric>0?1.f/metric:100000.f;
    if(reciprocal==0)
        throw std::invalid_argument("source bitmap coverage logarithm of zero requires separate source authority");
    // Source FLDLN2/FYL2X produces the natural logarithm. Its following
    // FMUL uses the stored float44A04C, rather than an exact host ln2.
    const double logarithm=std::log(double(reciprocal));
    const double scaled=logarithm*double(std::bit_cast<float>(0x3f317218u));
    return {reciprocal,float(scaled)};
}
} // namespace halo2

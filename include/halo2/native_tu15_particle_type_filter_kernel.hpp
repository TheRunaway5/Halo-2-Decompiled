#pragma once

namespace halo2::native_particle_type_filter_detail {
// TU1.5 17DE60: resolve the particle's salted object body, then compare the
// body's referenced definition's signed type against the caller's full word.
template<class Frame> void filter(Frame& frame) {
    frame.lookup_body();
    if(frame.has_body()&&frame.has_definition()) {
        frame.compare_definition_type();
        frame.return_comparison();
    } else frame.return_missing();
}
} // namespace halo2::native_particle_type_filter_detail

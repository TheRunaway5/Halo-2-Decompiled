#pragma once
#include "halo2/native_tu15_packed_rotation_kernel.hpp"

namespace halo2::native_codec_rotation_interpolation_detail {
// The view owns the lazy frame-backed inputs and observable SIMD registers.
// The normalization uses the same authored pairwise sum as packed rotation.
template<class View>void decode(View&view){
    const auto quaternion=view.interpolate(view.input());
    const auto norm=native_packed_rotation_detail::squared_norm(view,quaternion);
    const auto inverse=view.inverse(norm.value,quaternion,norm.opposite,norm.alternate);
    view.output(view.scale(quaternion,inverse));
}
} // namespace halo2::native_codec_rotation_interpolation_detail

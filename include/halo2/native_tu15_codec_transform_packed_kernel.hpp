#pragma once
#include "halo2/native_tu15_packed_rotation_kernel.hpp"

namespace halo2::native_codec_transform_packed_detail {
// The second private phase decodes the selected row and calls the genuine
// transform composition owner. Output ancestry and bitplane publication
// begin only after this phase's returned-child boundary.
template<class View>void compose_packed(View& view) {
    const auto quaternion=view.packed_quaternion();
    const auto norm=native_packed_rotation_detail::squared_norm(view,quaternion);
    const auto reciprocal=view.estimate(quaternion,norm);
    view.store_quaternion(view.multiply(quaternion,reciprocal));
    view.compose_transform();
}
} // namespace halo2::native_codec_transform_packed_detail

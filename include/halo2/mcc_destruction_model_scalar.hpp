#pragma once
#include "halo2/mcc_gameplay_unit_scalar.hpp"

namespace halo2 {
// One exact named vanilla model response tuple, after ordinary pointer
// normalization. Unknown shapes/tuples stay unchanged. Views must be stable
// checked owned metadata; no gameplay result or runtime datum is supplied.
std::optional<MccGameplayUnitScalarReceipt> convert_mcc_destruction_model_scalars(
    TagGroup,std::string_view,std::uint32_t,
    const std::function<std::span<std::byte>(std::uint32_t,std::size_t)>&);
}

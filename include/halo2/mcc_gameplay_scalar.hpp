#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <optional>
#include <string_view>

namespace halo2 {
enum class MccGameplayScalarStatus { Converted, AlreadyEquivalent, AuthoredTupleUnrecognized };
struct MccGameplayScalarField {
    std::uint32_t offset{},before{},after{};
    std::uint8_t bytes{};
};
struct MccGameplayScalarReceipt {
    std::string_view profile;
    MccGameplayScalarStatus status{};
    std::array<MccGameplayScalarField,3> fields{};
    std::uint8_t field_count{};
};
// Narrow, source-backed retail compatibility profiles, not a full gameplay
// conversion certificate. Only an exact group/name and complete known donor
// tuple may change. An already-equivalent tuple is idempotent; other authored
// values are preserved and returned as unrecognized. Other tag groups/names
// return no profile. Truncated recognized definitions fail before any write.
std::optional<MccGameplayScalarReceipt> convert_mcc_gameplay_scalar(
    TagGroup,std::string_view name,std::span<std::byte> definition);
} // namespace halo2

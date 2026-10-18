#pragma once
#include "halo2/cache.hpp"
#include <functional>

namespace halo2 {
enum class MccGameplayUnitScalarStatus { Converted, AlreadyEquivalent, AuthoredTupleUnrecognized };
struct MccGameplayUnitScalarField {
    std::uint32_t address{};
    std::array<std::uint32_t,3> before{},after{};
    std::uint8_t words{};
};
struct MccGameplayUnitScalarReceipt {
    MccGameplayUnitScalarStatus status{};
    std::array<MccGameplayUnitScalarField,4> fields{};
    std::uint8_t field_count{};
};
// Exact authored words in already-normalized original-layout bipd/vehi data.
// All selected fields and block counts form one guard; an unknown authored
// tuple stays unchanged. The reader must provide stable checked owned views.
// No shader, pointer, bound runtime slot or floating-point arithmetic changes.
std::optional<MccGameplayUnitScalarReceipt> convert_mcc_gameplay_unit_scalars(
    TagGroup,std::string_view name,std::uint32_t root,
    const std::function<std::span<std::byte>(std::uint32_t,std::size_t)>&);
} // namespace halo2

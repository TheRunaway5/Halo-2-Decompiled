#pragma once
#include "halo2/cache.hpp"
#include <functional>

namespace halo2 {
enum class MccDestructionModelContractStatus { Converted, AlreadyEquivalent, AuthoredTupleUnrecognized };
struct MccDestructionModelContractReceipt {
    MccDestructionModelContractStatus status{MccDestructionModelContractStatus::AuthoredTupleUnrecognized};
    std::array<std::uint32_t,3> addresses{},before{},after{};
};
// Exact named Hunter authored section contract, in stable owned metadata after
// ordinary pointer normalization. Restores the original flag and empty block
// descriptor together. The unreferenced donor response bytes stay untouched.
// Unknown tuples stay unchanged; no runtime damage or destruction is supplied.
std::optional<MccDestructionModelContractReceipt> convert_mcc_destruction_model_contract(
    TagGroup,std::string_view,std::uint32_t,
    const std::function<std::span<std::byte>(std::uint32_t,std::size_t)>&);
}

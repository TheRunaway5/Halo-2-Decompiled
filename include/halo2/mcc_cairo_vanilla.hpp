#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>
namespace halo2 {
class Cache;
struct MccCairoVanillaWrite {
    std::uint32_t source_token{};
    std::vector<std::byte> before,after;
};
struct MccCairoVanillaPlan {
    bool matched=false;
    std::string reason;
    std::vector<MccCairoVanillaWrite> writes;
    std::size_t retired_nodes{},script_ordinal_words{},scenario_ordinal_words{};
};
// A plan owns exact before/after bytes in the existing source address space.
// Validate EVERY write's original bytes/range before committing ANY write.
// Apply once during MCC cache initialization, before opcode/global conversion
// and gameplay consumers. Unknown or modified authoring produces no writes.
MccCairoVanillaPlan plan_mcc_cairo_vanilla(const Cache&);
// Read-only source injection permits constructor-independent plan validation.
// Returned views must remain valid for the duration of the call.
using MccCairoSourceReader=std::function<std::span<const std::byte>(std::uint32_t,std::size_t)>;
MccCairoVanillaPlan plan_mcc_cairo_vanilla(const Cache&,const MccCairoSourceReader&);
} // namespace halo2

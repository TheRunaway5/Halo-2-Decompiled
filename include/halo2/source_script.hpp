#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace halo2 {
// Retail2A29D0 advances the caller's primary RNG once and returns the literal
// zero-extended SHORT result. It does not initialize or own a random stream.
// Reversed bounds retain the original wrapping integer arithmetic.
std::uint16_t source_hs_random_range(std::uint32_t &primary_state,
                                   std::int16_t lower,std::int16_t upper);
// Retail207CF0 consumes a primary draw on each dispatch, including completion,
// and searches cyclically for the next unvisited expression. The original
// frame reserves one32-bit visited word; this admission covers0..32 children.
std::optional<unsigned> source_hs_begin_random(std::uint32_t &primary_state,
                                              unsigned count,std::uint32_t visited);
//20A130/20A170 convert a current name/object into a fresh one-element list.
//The actual list/reference pools, their GC and lifetime remain caller-owned.
//The original calls append even when allocation returns NONE; the supplied
//service must handle or reject that boundary rather than invent a list handle.
struct SourceHsObjectListServices {
    std::function<std::uint32_t()> allocate;
    std::function<void(std::uint32_t list,std::uint32_t object)> append;
};
std::uint32_t source_hs_object_list(std::uint32_t object,
                                  const SourceHsObjectListServices &);
std::uint32_t source_hs_named_object_list(std::int16_t name,
    std::span<const std::uint32_t> name_handles,const SourceHsObjectListServices &);
}

#pragma once
#include "halo2/cache.hpp"
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace halo2 {
struct SourceBspAcceleratorResult {
    bool complete{};
    std::string diagnostic;
};

// Original2DD930 ->2DC220, ordinary accelerated visibility query0. `tree`
// includes the literal48-byte header and caller-bounded bytecode at+30. The
// service reads header float4 at0; +20 is not interpreted as a code length.
// Planes are the exact ordered float4 values passed by44E90, including w.
// Existing banks append in place: direct terminal keys in `direct`, inclusive
// endpoint pairs in `ranges`. A required original hkArray growth, unsupported
// code/context or malformed bank returns incomplete and changes neither bank.
// Complete empty is a successful traversal and must not mean CPU fallback.
SourceBspAcceleratorResult source_bsp_accelerator_query0(
    std::span<const std::byte> tree, std::span<const ShaderVector> planes,
    std::vector<std::uint32_t>& direct, std::vector<std::uint32_t>& ranges,
    std::size_t direct_capacity=1024, std::size_t range_capacity=1024);

// Original2DDBD0 ->2DCE60 spherical query used by45560. Center/radius are
// the actual local query passed by44720; transforms do not scale the radius.
// Contained boxes append endpoint pairs; intersecting terminals append direct
// keys. Strict containment, SSE closest/farthest sums and x87 radius square
// retain the source boundaries and traversal order. The caller's two512-word
// stack lists cannot silently grow; incomplete queries leave both unchanged.
SourceBspAcceleratorResult source_bsp_accelerator_sphere(
    std::span<const std::byte> tree, Vec3 center, float radius,
    std::vector<std::uint32_t>& direct, std::vector<std::uint32_t>& ranges,
    std::size_t direct_capacity=512, std::size_t range_capacity=512);
} // namespace halo2

#include "halo2/source_renderer_cache_pools_tu15.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory, std::uint32_t address) {
    const auto bytes = memory.view(address, 4);
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i) value |= std::to_integer<std::uint32_t>(bytes[i]) << (i * 8);
    return value;
}
template<std::size_t N> void name(SourceRuntimeMemory& memory, std::uint32_t address,
    const std::array<char, N>& literal) {
    memory.mount(address, std::as_bytes(std::span(literal)), SourceRuntimeMemory::Access::ReadOnly,
        "TU1.5 renderer cache pool name with authored alignment padding");
}
NativeRuntimeAddress pool(const SourceRuntimeMemory& memory, std::uint32_t cell,
    std::uint32_t bytes, std::uint32_t count, std::uint32_t stride, bool embedded) {
    const auto base = word(memory, cell), header = base + (embedded ? 0x70u : 0u);
    if (!base || word(memory, header + 0x20) != count || word(memory, header + 0x24) != stride ||
        word(memory, header + 0x2c) != 0x64407440u || word(memory, header + 0x44) != header + 0x4cu ||
        word(memory, header + 0x48) != header + 0x4cu + count * stride)
        throw std::invalid_argument("Original renderer cache pool is not fully constructed");
    memory.validate_mutable_view(base, bytes);
    const auto address = memory.native_address(base, bytes);
    if (address.value <= 0xffffffffu)
        throw std::invalid_argument("Renderer cache pool lacks native64 backing");
    return address;
}
}
void mount_native_tu15_renderer_cache_pools_data(SourceRuntimeMemory& memory) {
    memory.mount(0x474ce8, std::array{std::byte{0xec}, std::byte{0xab}, std::byte{0x44}, std::byte{}},
        SourceRuntimeMemory::Access::ReadOnly, "Source12560 authored default render vector binding");
    // Scalar startup already owns this shared immutable zero vector. Require
    // its complete original bytes instead of introducing an overlapping bank.
    if (!memory.is_read_only(0x44abec, 12) ||
        !std::ranges::all_of(memory.view(0x44abec, 12), [](auto byte) { return byte == std::byte{}; }))
        throw std::invalid_argument("Renderer success suffix shared default vector differs");
    memory.mount(0x4c3ad4, std::array<std::byte, 16>{}, SourceRuntimeMemory::Access::ReadWrite,
        "Source12560 cold default render vector and selector fields");
    // These exact extents are loader-zeroed in the authenticated TU1.5 image.
    for (const auto address : {0x4f2d24u, 0x4f2d5cu})
        memory.mount(address, std::array<std::byte, 8>{}, SourceRuntimeMemory::Access::ReadWrite,
            "TU1.5 renderer object and predicted-object cold pool bindings");
    for (const auto address : {0x4f2d34u, 0x4f2d6cu})
        memory.mount(address, std::array<std::byte, 4>{}, SourceRuntimeMemory::Access::ReadWrite,
            "TU1.5 renderer cache cold pool binding");
    name(memory, 0x45f218, std::array<char, 20>{"xbox texture cache"});
    name(memory, 0x45f22c, std::array<char, 24>{"xbox predicted texture"});
    name(memory, 0x45f244, std::array<char, 16>{"xbox texture"});
    name(memory, 0x45f254, std::array<char, 20>{"xbox geometry cache"});
    name(memory, 0x45f268, std::array<char, 24>{"xbox predicted geometry"});
    name(memory, 0x45f280, std::array<char, 16>{"xbox geometry"});
}
NativeTu15RendererCachePoolViews native_tu15_renderer_cache_pool_views(const SourceRuntimeMemory& memory) {
    const std::array<std::pair<std::uint32_t, std::uint32_t>, 6> extents{{
        {word(memory, 0x4f2d5c), 0x188c}, {word(memory, 0x4f2d60), 0xb68},
        {word(memory, 0x4f2d6c), 0x30fc}, {word(memory, 0x4f2d24), 0x508c},
        {word(memory, 0x4f2d28), 0x6a8}, {word(memory, 0x4f2d34), 0x30fc}}};
    for (unsigned i = 0; i < extents.size(); ++i)
        for (unsigned j = 0; j < i; ++j)
            if (extents[i].first < std::uint64_t(extents[j].first) + extents[j].second &&
                extents[j].first < std::uint64_t(extents[i].first) + extents[i].second)
                throw std::invalid_argument("Independent renderer cache pool allocations overlap");
    return {pool(memory, 0x4f2d5c, 0x188c, 0x200, 0xc, false),
        pool(memory, 0x4f2d60, 0xb68, 0x15e, 8, false),
        pool(memory, 0x4f2d6c, 0x30fc, 0x200, 0x18, true),
        pool(memory, 0x4f2d24, 0x508c, 0x200, 0x28, false),
        pool(memory, 0x4f2d28, 0x6a8, 0xc8, 8, false),
        pool(memory, 0x4f2d34, 0x30fc, 0x200, 0x18, true)};
}
}

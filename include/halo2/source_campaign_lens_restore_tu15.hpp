#pragma once
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace halo2 {
// TU1.5 original126640/21B920 allocates the3FE000 high-address saved
// arena.126A70/21BCF0 reads that arena; these static renderer fields are
// outside it. The actual live renderer retains its corresponding GPU pages,
// query generations and SDK overlays. No default/full-RAM restore uses this.
inline constexpr std::array<std::pair<std::uint32_t,std::size_t>,3>
    source_campaign_live_lens_regions_tu15{{{0x52dfd8u,0x40c0u},
        {0x532098u,4u},{0x4922c0u,8u}}};

inline void source_require_live_campaign_lens_tu15(const SourceRuntimeMemory &memory) {
    for(const auto &[address,bytes]:source_campaign_live_lens_regions_tu15) {
        (void)memory.view(address,bytes);
        if(memory.is_read_only(address,bytes))
            throw std::invalid_argument("Live campaign lens state requires its complete writable source owners");
    }
}

// Called only while enclosing authority preparation holds both the source
// executor lock and staging barrier. Validate every span before writing any
// replacement bytes; a failed/cancelled preparation never changes the live
// bank. Fixed16KB staging avoids copying the multi-GB authority checkpoint.
inline void source_stage_live_campaign_lens_tu15(const SourceRuntimeMemory &current,
    SourceRuntimeMemory &replacement) {
    if(&current==&replacement)
        throw std::invalid_argument("Live campaign lens adoption requires a distinct prepared replacement");
    source_require_live_campaign_lens_tu15(current);
    for(const auto &[address,bytes]:source_campaign_live_lens_regions_tu15)
        replacement.validate_mutable_view(address,bytes);
    std::array<std::byte,0x40cc> retained;
    std::size_t offset{};
    for(const auto &[address,bytes]:source_campaign_live_lens_regions_tu15) {
        const auto source=current.view(address,bytes);
        std::copy(source.begin(),source.end(),retained.begin()+offset);offset+=bytes;
    }
    offset=0;
    for(const auto &[address,bytes]:source_campaign_live_lens_regions_tu15) {
        replacement.write(address,std::span(retained).subspan(offset,bytes));offset+=bytes;
    }
    source_require_live_campaign_lens_tu15(replacement);
    offset=0;
    for(const auto &[address,bytes]:source_campaign_live_lens_regions_tu15) {
        const auto expected=std::span(retained).subspan(offset,bytes);
        if(!std::ranges::equal(current.view(address,bytes),expected) ||
            !std::ranges::equal(replacement.view(address,bytes),expected))
            throw std::invalid_argument("Live campaign lens staging changed its protected source or replacement bytes");
        offset+=bytes;
    }
}
}

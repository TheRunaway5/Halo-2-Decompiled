#pragma once
#include "halo2/tags/types.hpp"
#include <cstddef>
#include <span>

namespace halo2 {
// The caller resolves both full salted identities from the same Cache and
// supplies their complete payloads. This checks the shared authored layout;
// it does not construct native resources or publish cache/source pointers.
// Per-frame bitmap rows and actual decoded 2D images remain caller checks;
// MCC's 168-byte bitmap rows are not treated as Xbox's 116-byte records.
struct SourceEffectTagInputTu15 {
    const Tag& definition;
    std::span<const std::byte> definition_payload;
    const Tag& bitmap;
    std::span<const std::byte> bitmap_payload;
};
struct SourceContrailTagLayoutTu15 {
    TagHandle definition,bitmap;
    std::uint16_t render_type{},properties{},blend{},fade{},sampler_flags{};
    std::uint32_t bitmap_images{};
    TagBlock point_states;
};
struct SourceDecalTagLayoutTu15 {
    TagHandle definition,bitmap,next_decal;
    std::uint16_t layer{};
    std::uint32_t bitmap_images{};
};
// Ordinary primary-bitmap subset consumed by SourceContrailScene. Secondary
// maps remain unadmitted. Point states are the exact count*64 bytes resolved
// from definition+E8/EC, not an inferred inline or rewritten state bank.
SourceContrailTagLayoutTu15 qualify_source_contrail_tag_layout_tu15(
    const SourceEffectTagInputTu15&,std::span<const std::byte> point_states);
SourceDecalTagLayoutTu15 qualify_source_decal_tag_layout_tu15(
    const SourceEffectTagInputTu15&);
// The caller retains rasterizer slot3/4 identity; this authenticates class0
// code and its complete declaration against the existing TU1.5 translations.
void qualify_source_effect_vertex_program_tu15(TagGroup effect_group,
    unsigned classification,std::span<const std::byte> code,
    std::span<const std::uint16_t> semantics);
}

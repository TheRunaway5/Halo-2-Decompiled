#pragma once
#include "halo2/source_bsp_admission.hpp"
#include "halo2/source_shader_lod.hpp"
#include "halo2/source_transparent_parts.hpp"
#include "halo2/source_view_projection.hpp"
#include <map>
#include <set>

namespace halo2 {
struct SourceBspShaderView {
    const SourceViewProjectionBank &camera;
    const SourceBspAdmission &admission;
    std::int16_t outer_top{},outer_bottom{};
    unsigned local_players{};
    bool view_modifier{};
    std::optional<float> detail_multiplier;
    // Owned frame snapshot of actual186E70; empty only for independent/static callers.
    std::span<const std::byte> breakable_visibility;
};
struct SourceBspShaderPart {
    int geometry{}; // >=0 cluster; <=-2 authored instance ordinal.
    unsigned section{},part{},shader_lod{},material{};
    std::uint8_t coverage_byte{};
    std::uint8_t instance_alpha=255;
};
// Actual45E19 material gate, before shader LOD selection. Include hidden parts
// outside this view's admission so untouched/fallback bindings cannot resurrect them.
std::set<std::pair<int,unsigned>> source_bsp_broken_parts(const SourceTransparentBspParts&,
    std::span<const std::byte>,std::size_t bsp);
// Resident ordinary BSP shader banks. Each view selects its own authored part
// bindings without rewriting another player's materials. Infinite water keeps
// its explicit LOD0 service; model shaders have a separate producer.
class SourceBspShaderScene {
public:
    SourceBspShaderScene(const Cache &,Mesh &,std::size_t bsp);
    std::vector<SourceBspShaderPart> select(std::vector<Vertex> &,
        const SourceBspShaderView &) const;
private:
    std::size_t bsp_index_{};
    SourceBspAdmissionBsp geometry_;
    SourceTransparentBspParts parts_;
    std::map<std::uint32_t,SourceShaderLodDefinition> shaders_;
    std::vector<std::vector<unsigned>> materials_;
    std::vector<std::uint32_t> source_shaders_;
    std::map<std::pair<int,unsigned>,unsigned> source_materials_;
};
}

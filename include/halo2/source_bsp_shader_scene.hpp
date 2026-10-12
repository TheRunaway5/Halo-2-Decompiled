#pragma once
#include "halo2/source_bsp_admission.hpp"
#include "halo2/source_shader_lod.hpp"
#include "halo2/source_transparent_parts.hpp"
#include "halo2/source_view_projection.hpp"
#include <map>

namespace halo2 {
struct SourceBspShaderView {
    const SourceViewProjectionBank &camera;
    const SourceBspAdmission &admission;
    std::int16_t outer_top{},outer_bottom{};
    unsigned local_players{};
    bool view_modifier{};
    std::optional<float> detail_multiplier;
};
struct SourceBspShaderPart {
    int geometry{}; // >=0 cluster; <=-2 authored instance ordinal.
    unsigned section{},part{},shader_lod{},material{};
    std::uint8_t coverage_byte{};
    std::uint8_t instance_alpha=255;
};
// Resident ordinary BSP shader banks. Each view selects its own authored part
// bindings without rewriting another player's materials. Infinite water keeps
// its explicit LOD0 service; model shaders have a separate producer.
class SourceBspShaderScene {
public:
    SourceBspShaderScene(const Cache &,Mesh &,std::size_t bsp);
    std::vector<SourceBspShaderPart> select(std::vector<Vertex> &,
        const SourceBspShaderView &) const;
private:
    SourceBspAdmissionBsp geometry_;
    SourceTransparentBspParts parts_;
    std::map<std::uint32_t,SourceShaderLodDefinition> shaders_;
    std::vector<std::vector<unsigned>> materials_;
    std::vector<std::uint32_t> source_shaders_;
    std::map<std::pair<int,unsigned>,unsigned> source_materials_;
};
}

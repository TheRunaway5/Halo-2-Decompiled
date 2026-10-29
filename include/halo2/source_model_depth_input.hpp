#pragma once
#include "halo2/source_world_depth_input.hpp"
#include "halo2/source_shadow_raw_draw.hpp"
#include "halo2/source_shadow_raw_plan.hpp"
#include "halo2/source_model_depth_selection.hpp"
#include <map>

namespace halo2 {
// A synchronous completed-camera/target owner plus genuine native Apply
// geometry/palette. The fixed native CPU prefix supplies WORLD positions;
// an offline ordered-DAG proof admits their subsequent native class0 shader.
// No floating-point position matching or generic skinning chooses a vertex.
class SourceModelDepthInput {
public:
    std::span<const Vertex> vertices() const {return vertices_;}
    bool admits_draw(const SourceBspDrawKey&) const;
    void validate_vertices(const RendererTargets::Use&,std::span<const Vertex>) const;
    void validate_receivers(const SourceShadowRawPlan&) const;
    void validate_draw(const RendererTargets::Use&,const SourceBspDrawKey&,std::size_t count) const;
    // Expanded child passes must retain the exact admitted position program
    // and actual root material; their original pixel/texture producer stays
    // with the ordinary renderer.
    bool admits_material(unsigned root,const Material&) const;
    std::size_t model_vertices()const {return model_vertices_;}
private:
    friend SourceModelDepthInput source_model_depth_input_tu15(const SourceWorldDepthInput&,
        const RendererTargets::Use&,std::span<const Vertex>,std::span<const SourceModelDepthSelection>,
        std::span<const SourceShadowRawDraw* const>,std::span<const Material>);
    explicit SourceModelDepthInput(const SourceWorldDepthInput& depth):depth_(&depth){}
    const SourceWorldDepthInput* depth_{};
    std::vector<Vertex> vertices_;
    std::map<SourceBspDrawKey,std::size_t> draws_;
    struct MaterialOwner {const Material* material{};std::vector<std::byte> program;
        std::uint32_t shader{},lod{};ForwardRasterState raster;
        std::optional<SourceVertexDeclaration> declaration;
        SourceWorldVertexInputTransform transform{};
        std::map<unsigned,SourceVertexDeclaration> classes;};
    std::map<unsigned,MaterialOwner> materials_;
    struct ReceiverOwner {const SourceShadowRawDraw* draw;MenuWorldSourceBinding source;};
    std::vector<ReceiverOwner> receivers_;
    std::size_t model_vertices_{};
};
SourceModelDepthInput source_model_depth_input_tu15(const SourceWorldDepthInput&,
    const RendererTargets::Use&,std::span<const Vertex>,std::span<const SourceModelDepthSelection>,
    std::span<const SourceShadowRawDraw* const>,std::span<const Material>);
}

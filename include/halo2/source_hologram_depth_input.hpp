#pragma once
#include "halo2/source_hologram_group.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/world_material_uniform.hpp"

namespace halo2 {
// Synchronous capability for one actual423B0 group/body owner. It retains the
// exact native skin prefix, primitive occurrences and current camera. It
// grants no shadow Apply ownership and changes no material/raster state.
// The renderer binds its actual physical target extent at admission. This
// synchronous borrowed group/capability must not escape the current frame.
class SourceHologramDepthInput {
public:
    std::span<const Vertex> vertices()const {return vertices_;}
    std::span<const std::uint32_t> source_vertices()const {return source_vertices_;}
    bool admits_draw(const SourceBspDrawKey&)const;
    void validate_vertices(std::span<const Vertex>)const;
    void validate_group(const SourceHologramGroupDraw&)const;
    void validate_draw(const SourceHologramGroupDraw&,const SourceHologramGroupPart&,
        const SourceBspDrawKey&,std::size_t,const Material&,const WorldMaterialUniform&,
        const PackedVertexProgramBank&,ShaderVector forward,ShaderVector eye,ShaderVector viewport,
        const ForwardRasterState& actual_pipeline_state,unsigned actual_blend,bool actual_stencil)const;
    const SourceHologramGroupDraw& group()const {return *group_;}
private:
    friend SourceHologramDepthInput source_hologram_depth_input(
        const SourceHologramGroupDraw&,std::uint32_t,std::span<const Vertex>,std::array<std::uint32_t,2>);
    explicit SourceHologramDepthInput(const SourceHologramGroupDraw& group,std::uint32_t object,
        std::array<std::uint32_t,2> surface)
        :group_(&group),expected_(group),object_(object),physical_surface_(surface){}
    const SourceHologramGroupDraw* group_{};
    SourceHologramGroupDraw expected_;
    std::uint32_t object_{};
    std::array<std::uint32_t,2> physical_surface_{};
    std::vector<Vertex> vertices_;
    std::vector<std::uint32_t> source_vertices_;
    std::map<SourceBspDrawKey,std::size_t> draws_;
};
SourceHologramDepthInput source_hologram_depth_input(
    const SourceHologramGroupDraw&,std::uint32_t,std::span<const Vertex>,std::array<std::uint32_t,2> physical_surface);
}

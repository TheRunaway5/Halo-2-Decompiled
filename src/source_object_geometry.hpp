#pragma once
#include "halo2/source_object_view.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_prt.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/appearance.hpp"
#include "halo2/model.hpp"
#include "halo2/source_transparent_parts.hpp"
#include <array>

namespace halo2 {
//Resident geometry/material union shared by source MACH/BLOC scenes. Source
//section order, stream LOD and coefficient LOD stay together in each record.
class SourceObjectGeometry {
public:
    struct Lod {
        RenderModel model;
        std::vector<unsigned> sections,vertex_materials;
        std::vector<Vertex> vertices;
        bool ordinary_lighting{};
        SourceTransparentModelParts authored_parts;
    };
    struct Slot {unsigned material{},section{},lod{};bool prt{};unsigned shader_lod{};};
    struct ViewDraw {
        std::vector<Vertex> vertices;
        bool ordinary_lighting{},prt_lighting{};
    };
    struct Stats {
        unsigned dynamic_materials{},prt_materials{},section_banks{};
        unsigned resident_dynamic_materials{},resident_prt_materials{};
        unsigned union_dynamic_materials{},union_prt_materials{},union_section_banks{};
    };
    SourceObjectGeometry()=default;
    void load(const Cache &,const ScenarioObject &,const SourceInitialModelDefinition &,
              const SourceInitialModelSelection &,std::uint32_t render_model,bool prepare_region_variants=false);
    // Prepare every authored section without choosing a live permutation.
    // Active draws stay empty until regions() receives completed source state.
    void load_union(const Cache &,const ScenarioObject &,const SourceInitialModelDefinition &,
                    std::uint32_t render_model);
    void regions(const SourceInitialModelSelection &);
    ViewDraw active_draw(unsigned lod)const;
    Stats prepare(const Cache &,const ObjectColors &,std::span<const Transform> world_nodes,
                  const SourcePrtLighting &,Mesh &,std::optional<float> camouflage = {},
                  std::optional<Vec3> retained_incident = {});
    // Callback already skins the rooted WORLD bank; do not place it again.
    void pose_world(const std::function<std::vector<Vertex>(const RenderModel &)> &);
    void update_banks(const Cache &,std::span<const Transform>,const SourcePrtLighting &,Mesh &);
    std::vector<SourcePrtMaterialBank> owner_banks(const Cache &,std::span<const Transform>,
                                                const SourcePrtLighting &,unsigned lod,
                                                std::optional<Vec3> retained_incident = {})const;
    ViewDraw selected_section(unsigned lod,unsigned section,const SourceModelShaderCollector&,
                              const SourceModelShaderView&)const;
    ViewDraw selected_section_world(unsigned lod,unsigned section,const SourceModelShaderCollector&,
                              const SourceModelShaderView&,std::span<const Transform>)const;
    std::vector<SourcePrtMaterialBank> selected_prt_banks(unsigned lod,unsigned section,
                              std::shared_ptr<const VertexProgramBank>)const;
    SourceObjectViewSelection select(std::size_t source_object,std::uint32_t handle,
                                    Vec3 sphere_center,float radius,const SourceObjectViewLod &);
    // Same complete selector with caller-retained full-owner history. The
    // returned view is scratch storage; copy it before selecting another owner.
    SourceObjectViewSelection select(std::size_t source_object,std::uint32_t handle,
                                    Vec3 sphere_center,float radius,const SourceObjectViewLod &,
                                    std::array<std::int8_t,4> &owner_previous_lod);
    void reset_history();
    const RenderModel &model()const{return lods[5].model;}
    const Lod &at(unsigned lod)const{return lods.at(lod);}
    const ViewDraw &view(unsigned index)const{return views.at(index);}
    bool ordinary_lighting()const;
    unsigned first_material{},material_count{};
private:
    ViewDraw selected_section_vertices(unsigned,unsigned,const SourceModelShaderCollector&,
                              const SourceModelShaderView&,std::span<const Vertex>)const;
    void load_impl(const Cache &,const ScenarioObject &,const SourceInitialModelDefinition &,
        const SourceInitialModelSelection *,std::uint32_t,bool);
    std::array<Lod,6> lods;
    std::optional<SourcePrtDefinition> prt;
    SourceGeometryLodDefinition lod_definition;
    std::vector<Slot> slots;
    struct Part {
        unsigned section{},ordinal{},source_material{};
        float lod_scale{};
        std::vector<unsigned> materials;
        std::vector<bool> ordinary;
    };
    std::array<std::vector<Part>,6> parts;
    std::array<std::vector<unsigned>,6> vertex_parts;
    struct PartRun {std::size_t first{},count{};unsigned part{};};
    std::array<std::vector<PartRun>,6> part_runs;
    std::optional<SourceInitialModelDefinition> region_definition;
    std::array<std::vector<unsigned>,6> active_sections;
    std::vector<PartRun> active_runs(unsigned lod)const;
    std::vector<SourceShaderLodDefinition> shader_definitions;
    std::array<ViewDraw,4> views;
    struct ViewSelection {
        bool valid{},shader{};
        unsigned lod{};
        std::vector<unsigned> parts;
    };
    std::array<ViewSelection,4> view_selection;
    void invalidate_views(){for(auto &selection:view_selection)selection.valid=false;}
    bool union_ordinary_lighting{};
    std::array<std::int8_t,4> previous_lod{-1,-1,-1,-1};
};
}

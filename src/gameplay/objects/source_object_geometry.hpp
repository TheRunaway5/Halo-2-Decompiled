#pragma once
#include "halo2/source_object_view.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_prt.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/appearance.hpp"
#include "halo2/model.hpp"
#include "halo2/source_transparent_parts.hpp"
#include "halo2/model_skin_submission.hpp"
#include <array>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace halo2 {
//Resident geometry/material union shared by source MACH/BLOC scenes. Source
//section order, stream LOD and coefficient LOD stay together in each record.
class SourceObjectGeometry {
public:
    struct Lod {
        RenderModel model;
        std::vector<unsigned> sections,vertex_materials;
        std::vector<Vertex> vertices;
        // pose_world_lod may leave this bank's records in local space with
        // the WORLD palette the renderer deforms them with (deferred).
        std::vector<std::array<float,12>> palette;
        bool deferred{};
        bool ordinary_lighting{};
        SourceTransparentModelParts authored_parts;
    };
    struct Slot {unsigned material{},section{},lod{};bool prt{};unsigned shader_lod{};};
    struct ViewDraw {
        std::vector<Vertex> vertices;
        // Source ordinals survive CPU and deferred selection identically.
        std::vector<std::uint32_t> ordinals;
        bool ordinary_lighting{},prt_lighting{};
    };
    struct ViewSelection {
        bool valid{},shader{};
        unsigned lod{};
        std::vector<unsigned> parts;
    };
    // Exact mutable presentation outputs only. The resident RenderModel/LOD,
    // shader/material catalog, section schemas and PRT definitions stay here.
    // The enclosing scene authenticates owner/executor/revision and quiesces
    // exported spans before exchanging this value; no pose is reevaluated.
    struct PresentationState {
        std::array<std::vector<Vertex>,6> vertices;
        std::array<std::vector<std::array<float,12>>,6> palettes;
        std::array<bool,6> deferred{};
        std::array<std::vector<unsigned>,6> active_sections;
        std::array<ViewDraw,4> views;
        std::array<ViewSelection,4> selections;
        std::array<std::int8_t,4> previous_lod;
    };
    PresentationState capture_presentation()const {
        PresentationState out;
        for(unsigned n=0;n<6;++n){out.vertices[n]=lods[n].vertices;out.palettes[n]=lods[n].palette;out.deferred[n]=lods[n].deferred;}
        out.active_sections=active_sections;out.views=views;
        out.selections=view_selection;out.previous_lod=previous_lod;return out;
    }
    void exchange_presentation(PresentationState &state)noexcept {
        static_assert(std::is_nothrow_swappable_v<PresentationState>);
        for(unsigned n=0;n<6;++n){lods[n].vertices.swap(state.vertices[n]);lods[n].palette.swap(state.palettes[n]);std::swap(lods[n].deferred,state.deferred[n]);}
        active_sections.swap(state.active_sections);views.swap(state.views);
        view_selection.swap(state.selections);previous_lod.swap(state.previous_lod);
    }
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
    // pose_world with skin_model_world_nodes. The six independent LOD skins
    // deform together and publish in LOD order with identical results.
    void pose_world_nodes(std::span<const Transform> world_nodes);
    // The LOD select() chooses for this owner/view: metric, history and
    // policy alone decide it, posed records take no part.
    unsigned choose_lod(std::size_t source_object,std::uint32_t handle,Vec3 sphere_center,float radius,
                        const SourceObjectViewLod &,const std::array<std::int8_t,4> &owner_previous_lod)const;
    // pose_world for one LOD bank. A pose with gpu set keeps local records
    // and the WORLD palette; select() then records source ordinals and
    // view_skin() publishes the deferred geometry. Other LOD banks keep
    // their earlier records; select() reads only its chosen LOD.
    void pose_world_lod(unsigned lod,const std::function<ModelSkinWorldPose(const RenderModel &)> &);
    // Consume a frame-owned complete pose without allocating a temporary
    // return-vector copy. Geometry retains its own material-remapped bank;
    // no borrowed span survives the callback or enters a checkpoint.
    void pose_world_lod_view(unsigned lod,const std::function<ModelSkinWorldPoseView(const RenderModel &)> &);
    // The renderer-deformed geometry of a view whose selected LOD bank is
    // deferred; null when that bank holds WORLD records.
    std::shared_ptr<ModelSkinGeometry> view_skin(unsigned view_index,std::uint32_t owner,
                        std::uint32_t definition,std::uint32_t render_model)const;
    void update_banks(const Cache &,std::span<const Transform>,const SourcePrtLighting &,Mesh &);
    std::vector<SourcePrtMaterialBank> owner_banks(const Cache &,std::span<const Transform>,
                                                const SourcePrtLighting &,unsigned lod,
                                                std::optional<Vec3> retained_incident = {})const;
    ViewDraw selected_section(unsigned lod,unsigned section,const SourceModelShaderCollector&,
                              const SourceModelShaderView&)const;
    ViewDraw selected_section_world(unsigned lod,unsigned section,const SourceModelShaderCollector&,
                              const SourceModelShaderView&,std::span<const Transform>)const;
    // A frame-owned completed skin can serve several selected sections/views.
    // This retains the ordinary part/shader selector and leaves other LODs idle.
    ViewDraw selected_section_pose(unsigned lod,unsigned section,const SourceModelShaderCollector&,
                              const SourceModelShaderView&,std::span<const Vertex>)const;
    std::vector<SourcePrtMaterialBank> selected_prt_banks(unsigned lod,unsigned section,
                              std::shared_ptr<const VertexProgramBank>)const;
    SourceObjectViewSelection select(std::size_t source_object,std::uint32_t handle,
                                    Vec3 sphere_center,float radius,const SourceObjectViewLod &);
    // Same complete selector with caller-retained full-owner history. The
    // returned view is scratch storage; copy it before selecting another owner.
    SourceObjectViewSelection select(std::size_t source_object,std::uint32_t handle,
                                    Vec3 sphere_center,float radius,const SourceObjectViewLod &,
                                    std::array<std::int8_t,4> &owner_previous_lod);
    void reset_history() noexcept;
    const RenderModel &model()const{return lods[5].model;}
    const SourcePrtDefinition* prt_definition()const{return prt?&*prt:nullptr;}
    const Lod &at(unsigned lod)const{return lods.at(lod);}
    const ViewDraw &view(unsigned index)const{return views.at(index);}
    bool ordinary_lighting()const;
    unsigned first_material{},material_count{};
private:
    struct LodChoice {SourceObjectLodInput input;SourceGeometryLodChoice choice;unsigned lod{};};
    LodChoice lod_choice(std::size_t,std::uint32_t,Vec3,float,const SourceObjectViewLod &,
                         const std::array<std::int8_t,4> &)const;
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
    std::array<ViewSelection,4> view_selection;
    void invalidate_views() noexcept {for(auto &selection:view_selection)selection.valid=false;}
    bool union_ordinary_lighting{};
    std::array<std::int8_t,4> previous_lod{-1,-1,-1,-1};
};
}

#pragma once
#include "halo2/model.hpp"
#include "halo2/model_scale_reset.hpp"
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_object_draw_collector.hpp"
#include <array>
#include <memory>

namespace halo2 {
struct SourceLiveModelBaseDrawTu15;
// Definition-level capability, independent of any representative placement's
// baked identity. The submitting unit owns the live lighting classification.
bool source_campaign_lit_scenery(const RenderModel &,const ScenarioObject &);
// Resources for all authored permutations and LODs. Loading never chooses a
// live variant or advances the source RNG. Original4B5D0 selects every draw;
// its completed deferred WORLD pose is supplied separately by the live world.
class SourceCampaignModel {
public:
    SourceCampaignModel(const Cache &,const ScenarioObject &);
    // Dynamic actors have a definition but no scenario placement. Resource
    // residency must not manufacture a placement or choose their live variant.
    SourceCampaignModel(const Cache &,std::uint32_t definition,std::uint32_t variant=0);
    void prepare_materials(const Cache &,Mesh &destination,bool per_vertex_scenery=false);
    void rebase_materials(unsigned previous_geometry_count,unsigned next_geometry_count);
    const RenderModel &hierarchy()const{return *lods_[5];}
    std::span<const ModelScaleReset> scale_resets()const{return scale_resets_;}
    // Immutable tag names remain resident; the live source world still owns
    // the selected index, including the negative/default selection.
    std::uint32_t variant_name(std::int32_t index)const {
        return index<0?0u:variant_names_.at(unsigned(index));
    }
    // Ordered original section requests, including repeated sections. Empty
    // original admission remains empty; an unknown model/owner is an error.
    std::vector<Vertex> vertices(std::uint32_t owner,const SourceObjectDrawCollector &,
        std::span<const Transform> rendered_world_nodes)const;
    std::vector<Vertex> draw(std::uint32_t owner,const SourceObjectDrawCollector &,
        std::span<const Transform> rendered_world_nodes,
        const SourceModelShaderCollector &,const SourceModelShaderView &)const;
    // Only actual ordinary main units are admitted here. The caller retains
    // override units separately; an override cannot become an ordinary body.
    std::vector<Vertex> draw(std::uint32_t owner,
        std::span<const SourceLiveModelBaseDrawTu15>,
        std::span<const Transform> rendered_world_nodes,const SourceModelShaderView &,
        const BspLighting *current_lighting=nullptr)const;
private:
    std::vector<Vertex> vertices(unsigned lod,std::span<const unsigned> sections,
        std::span<const Transform> rendered_world_nodes,const BspLighting *lighting=nullptr,
        unsigned instance=0x7ffu)const;
    std::uint32_t definition_{},variant_{},render_model_{};
    std::vector<std::uint32_t> variant_names_;
    std::array<std::shared_ptr<const RenderModel>,6> lods_;
    std::array<std::vector<unsigned>,6> sections_;
    std::vector<unsigned> union_sections_;
    std::vector<ModelScaleReset> scale_resets_;
    std::unique_ptr<SourceDynamicModelShaders> shaders_;
    unsigned material_base_{};
    std::unique_ptr<SourceDynamicModelShaders> baked_shaders_;
    unsigned baked_material_base_{};
    std::string baked_failure_;
};
}

#pragma once
#include "halo2/model.hpp"
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_object_draw_collector.hpp"
#include <array>
#include <memory>

namespace halo2 {
// Resources for all authored permutations and LODs. Loading never chooses a
// live variant or advances the source RNG. Original4B5D0 selects every draw;
// its completed deferred WORLD pose is supplied separately by the live world.
class SourceCampaignModel {
public:
    SourceCampaignModel(const Cache &,const ScenarioObject &);
    void prepare_materials(const Cache &,Mesh &destination);
    void rebase_materials(unsigned previous_geometry_count,unsigned next_geometry_count);
    const RenderModel &hierarchy()const{return *lods_[5];}
    // Ordered original section requests, including repeated sections. Empty
    // original admission remains empty; an unknown model/owner is an error.
    std::vector<Vertex> vertices(std::uint32_t owner,const SourceObjectDrawCollector &,
        std::span<const Transform> rendered_world_nodes)const;
    std::vector<Vertex> draw(std::uint32_t owner,const SourceObjectDrawCollector &,
        std::span<const Transform> rendered_world_nodes,
        const SourceModelShaderCollector &,const SourceModelShaderView &)const;
private:
    std::uint32_t definition_{},variant_{},render_model_{};
    std::array<std::shared_ptr<const RenderModel>,6> lods_;
    std::array<std::vector<unsigned>,6> sections_;
    std::vector<unsigned> union_sections_;
    std::unique_ptr<SourceDynamicModelShaders> shaders_;
    unsigned material_base_{};
};
}

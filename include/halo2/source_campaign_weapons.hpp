#pragma once
#include "halo2/source_campaign_model.hpp"
#include "halo2/model_node_interpolation.hpp"
#include "halo2/source_netgame_weapon.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/appearance.hpp"
#include <map>

namespace halo2 {
class MultiplayerObjectRuntime;
struct SourceCampaignViewFrameTu15;
struct SourceCampaignWeaponDraw {
    std::uint32_t object{},definition{};
    unsigned ordinal{};
    std::vector<Vertex> vertices;
    ObjectLightingState lighting;
    ObjectColors colors;
    ShaderInput input;
};
struct SourceCampaignWeaponPending {
    std::uint32_t object{},shader{};
    unsigned ordinal{},lighting_class{};
};
struct SourceCampaignWeaponFrame {
    std::vector<SourceCampaignWeaponDraw> draws;
    std::vector<SourceCampaignWeaponPending> pending_overrides;
};
// Resource residency is independent of player first-person/pickup eligibility.
// Only completed original main units can submit an actual weapon owner. Source
// WORLD, material inputs, colors and lighting remain local to that owner/view.
class SourceCampaignWeapons {
public:
    SourceCampaignWeapons(const Cache &,const LevelContent &,Mesh &);
    void rebase_materials(unsigned previous_geometry_count,unsigned next_geometry_count);
    void reset(){poses_.clear();}
    bool owns_definition(std::uint32_t definition)const{return resources_.contains(definition);}
    SourceCampaignWeaponFrame draw(MultiplayerObjectRuntime &,
        const SourceCampaignViewFrameTu15 &,const SourceModelShaderView &,
        std::uint64_t tick,float fraction,float shader_seconds);
private:
    struct Resource {
        SourceWorldWeaponDefinition definition;
        std::unique_ptr<SourceCampaignModel> model;
        std::optional<float> primary_charge_seconds;
        std::string failure;
    };
    struct Pose {
        ModelNodeInterpolation history;
        std::uint32_t definition{},parent{};
        std::uint64_t tick{};
    };
    const Cache *cache_{};
    std::shared_ptr<const ShaderFunctionTables> tables_;
    std::map<std::uint32_t,Resource> resources_;
    std::map<std::uint32_t,Pose> poses_;
};
}

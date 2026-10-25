#pragma once
#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/math.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <nlohmann/json.hpp>

namespace halo2::test {
// Caller inputs only. B9960/B9B70 own admission, all object fields, named
// model selection, child births and physics. This probe proves no damage gate.
inline std::uint32_t pelican_children_constructor_tu15(MultiplayerPreparedWorld &world,
    const Cache &cache,const ScenarioObject &placement,Vec3 position,nlohmann::json &report) {
    const auto require=[](bool value,const char *message){if(!value)throw FormatError(message);};
    auto &source=*world.source_objects;auto &native=source.tu15_world();
    const auto tag=cache.query_tag({.handle=TagHandle{placement.definition}});
    require(tag && tag->name=="objects\\vehicles\\pelican\\pelican","Pelican parent identity changed");
    const auto model=cache.load_initial_model_definition(placement.definition);
    const auto variant=std::ranges::find_if(model.variants,[&](const auto &row){
        return cache.string_id(row.name)=="attack";
    });
    require(variant!=model.variants.end(),"Authored Pelican attack variant missing");
    constexpr std::uint32_t packet=0x60010000u;
    auto initialize=native.prepare_source_call(std::array{placement.definition,null_datum,0u});
    initialize.gp[6]=packet;native.run_actual(0xb9960u,initialize);
    const auto word=[&](std::uint32_t at,std::uint32_t value){
        const std::array bytes{std::byte(value),std::byte(value>>8),std::byte(value>>16),std::byte(value>>24)};
        native.write_source_call_input(at,bytes);
    };
    const auto vector=[&](std::uint32_t at,Vec3 value){
        word(at,std::bit_cast<std::uint32_t>(value.x));
        word(at+4,std::bit_cast<std::uint32_t>(value.y));
        word(at+8,std::bit_cast<std::uint32_t>(value.z));
    };
    word(packet+0xc,variant->name);
    vector(packet+0x1c,position);
    const auto basis=authored_basis(placement.rotation);
    vector(packet+0x28,basis.forward);vector(packet+0x34,basis.up);
    report["constructor_inputs"]={{"definition",placement.definition},{"variant",variant->name},
        {"variant_name","attack"},{"position",{position.x,position.y,position.z}},
        {"position_input","Native observer position plus [0,0,0.5], as in the isolated vehicle-variant constructor fixture"},
        {"authored_scenario_position",{placement.position.x,placement.position.y,placement.position.z}},
        {"forward",{basis.forward.x,basis.forward.y,basis.forward.z}},
        {"up",{basis.up.x,basis.up.y,basis.up.z}},{"original_entries",{0xb9960u,0xb9b70u}},
        {"other_request_defaults_owned_by_original",true}};
    auto birth=native.prepare_source_call(std::array{packet});native.run_actual(0xb9b70u,birth);
    const auto parent=birth.gp[0];report["actual_constructor_result"]=parent;
    require(parent!=null_datum,"Original Pelican constructor returned NONE; original admission comparison required");
    require(source.object_definition(parent)==placement.definition,"Pelican birth changed the definition");
    const auto selected=source.object_model_regions(parent).selection.variant_index;
    report["actual_variant_index"]=selected;
    require(selected==std::distance(model.variants.begin(),variant),"Original Pelican constructor selected another variant");
    auto &children=report["actual_children"];children=nlohmann::json::array();
    unsigned pods=0,guns=0;
    for(const auto &child:source.live_objects())if(child.parent==parent && child.type==1) {
        const auto definition=cache.query_tag({.handle=TagHandle{child.definition}});
        require(bool(definition),"Created Pelican child has no definition identity");
        const bool pod=definition->name=="objects\\vehicles\\pelican\\pelican_rocket_pod\\pelican_rocket_pod";
        const bool gun=definition->name=="objects\\vehicles\\pelican\\pelican_rear_gun\\pelican_rear_gun";
        require(pod || gun,"Attack Pelican created an unexpected vehicle child");
        pods+=pod;guns+=gun;
        const auto object_metadata=cache.load_source_tag_metadata(child.definition);
        const auto model_metadata=cache.load_source_tag_metadata(Bytes(object_metadata.bytes_at(
            object_metadata.source_address(),object_metadata.size())).u32(0x38));
        require(model_metadata.group()=="hlmt","Pelican child lost its authored model");
        const auto count=Bytes(model_metadata.bytes_at(model_metadata.source_address(),model_metadata.size())).u32(0x60);
        require(count==0,"Pelican child damage-information contract changed");
        children.push_back({{"object",child.object},{"parent",child.parent},{"definition",child.definition},
            {"name",definition->name},{"damage_information_count",count},
            {"world_nodes",source.world_nodes(child.object).size()}});
    }
    require(pods==2 && guns==1,"Original Pelican attack constructor did not create two pods and one rear gun");
    report["construction_only"]=true;report["damage_or_invulnerability_qualified"]=false;
    return parent;
}
} // namespace halo2::test

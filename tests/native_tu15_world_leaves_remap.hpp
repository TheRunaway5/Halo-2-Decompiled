#pragma once
// Remap modes of the group H harnesses: the services the phantom/world
// composition passes (src/physics/physics_phantom_world_tu15_generated.cpp,
// Composition::bound()). services.instruction becomes the composition's
// PhysicsInstructionRemapTu15 around a copy of the harness's own services
// (the outer callback and cursor, which hold TU1.5 PCs), and the physics
// world unit publishes canonical PCs (selected_observation_pcs off). The
// table is the composition's own (world_sites or vehicle_sites), extracted by
// the validator into remap_sites.inc. The harness keeps its plain observer,
// so both routes' stores are compared with canonical PCs.
//
// Modes: "remap", "remap-census", "remap-verify" select remap with the base
// mode after the dash.
#include "physics/runtime/physics_instruction_remap_tu15.hpp"
#include <span>
#include <string>
#include <vector>

namespace leaves_remap {
inline constexpr halo2::PhysicsInstructionRemapTu15::Binding sites[]{
#include "remap_sites.inc"
};
inline bool enabled{};
inline std::span<const halo2::PhysicsInstructionRemapTu15::Binding> table{sites};
// Returns the base mode ("", "census" or "verify") and sets enabled.
inline std::string parse(const std::string &mode) {
    if(mode.rfind("remap",0)!=0)return mode;
    enabled=true;
    return mode.size()>6?mode.substr(6):std::string();
}
// Wrap services in the remap; outer keeps the original services and must
// outlive every use of services.
inline void wrap(halo2::SourcePhysicsContactRuntimeServices &services,halo2::SourcePhysicsContactRuntimeServices &outer) {
    if(!enabled)return;
    outer=services;
    services.instruction=halo2::PhysicsInstructionRemapTu15{&outer,table};
}
// Tables a bridge must refuse: the owner's first or last instruction
// unbound, one instruction mapped with a different delta, and every
// instruction shifted by 0x10.
// first_canonical/last_canonical bound the owner's canonical instructions.
inline std::vector<std::vector<halo2::PhysicsInstructionRemapTu15::Binding>> refused_tables(std::uint32_t first_canonical,
    std::uint32_t last_canonical) {
    std::vector<std::vector<halo2::PhysicsInstructionRemapTu15::Binding>> tables;
    std::vector<halo2::PhysicsInstructionRemapTu15::Binding> all(std::begin(sites),std::end(sites));
    auto missing=all;
    std::erase_if(missing,[&](const auto &row){return row.key==first_canonical;});
    tables.push_back(missing);
    auto missing_last=all;
    std::erase_if(missing_last,[&](const auto &row){return row.key==last_canonical;});
    tables.push_back(missing_last);
    auto uneven=all;
    std::vector<std::size_t> inside;
    for(std::size_t i=0;i<uneven.size();++i)if(uneven[i].key>first_canonical && uneven[i].key<last_canonical)inside.push_back(i);
    if(!inside.empty())uneven[inside[inside.size()/2]].value+=1u;
    tables.push_back(uneven);
    auto shifted=all;
    for(auto &row:shifted)if(row.key>=first_canonical && row.key<=last_canonical)row.value+=0x10u;
    tables.push_back(shifted);
    return tables;
}
// The harness's native services carry the live cursor unless this is off.
inline bool live_cursor=true;
// ran() runs one native call and returns whether the native route ran
// (admitted, or faulted inside). Every refused table, and the valid table
// without a live cursor, must keep the translated route.
template<class Ran> bool refuses_bad_remaps(std::uint32_t first_canonical,std::uint32_t last_canonical,Ran ran) {
    const auto saved=table;
    bool refused=true;
    const auto bad=refused_tables(first_canonical,last_canonical);
    for(const auto &rows:bad){table=rows;refused=refused && !ran();}
    table=saved;
    live_cursor=false;refused=refused && !ran();live_cursor=true;
    return refused && ran();
}
} // namespace leaves_remap

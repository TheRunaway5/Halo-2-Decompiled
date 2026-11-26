#pragma once
#include "halo2/cache.hpp"
#include <string_view>

namespace halo2::path_tracing {
// Scoped to the authored shader owners used in The Heretic. Some of these
// materials are reused in later levels and retain the same finish there.
inline SourceSurfaceMaterial heretic_surface_material(std::string_view owner,std::string_view leaf) {
    using Finish=SourceSurfaceMaterial;
    if(owner=="objects\\characters\\prophet\\shaders") {
        if(leaf=="headdress")return Finish::Polished;
        const auto split=leaf.find('_');
        if(split==std::string_view::npos)return Finish::Unknown;
        const auto character=leaf.substr(0,split),part=leaf.substr(split+1);
        const bool prophet=character=="mercy" || character=="regret" || character=="truth";
        if((prophet || character=="minor") && part=="eyes")return Finish::Lens;
        if(prophet && (part=="head" || part=="skin" || part=="hands"))return Finish::Skin;
        if(prophet && part=="body")return Finish::Fabric;
        if(prophet && part=="helmet")return Finish::Polished;
    }
    if(owner=="objects\\characters\\elite\\shaders") {
        if(leaf=="head")return Finish::Skin;
        if(leaf=="council_helmet" || leaf=="helmet_ultra" || leaf=="elite_honor_armor" || leaf=="pike")
            return Finish::CoatedMetal;
    }
    if(owner=="objects\\characters\\brute\\shaders") {
        if(leaf=="brute_honor_armor")return Finish::CoatedMetal;
        if(leaf=="brute_head")return Finish::Skin;
        if(leaf=="brute_shoulder_armor" || leaf=="tartarus_shoulder_armor")return Finish::Metal;
    }
    if(owner=="objects\\characters\\masterchief\\shaders") {
        if(leaf=="masterchief")return Finish::CoatedMetal;
        if(leaf=="masterchief_visor")return Finish::Lens;
    }
    if(owner=="objects\\vehicles\\gravity_throne\\shaders") {
        if(leaf=="throne_shiny")return Finish::CoatedMetal;
        if(leaf=="throne_dull")return Finish::Metal;
    }
    if((owner=="objects\\cinematics\\human\\equipment_cart\\shaders" && leaf=="equipment_cart") ||
       (owner=="objects\\cinematics\\human\\power_core\\shaders" && leaf=="power_core") ||
       (owner=="objects\\cinematics\\human\\pillarofautumn\\shaders" &&
        (leaf=="poa_class" || leaf=="poa_class_engine")))
        return Finish::Metal;
    if(owner=="scenarios\\objects\\covenant\\military\\capital_ship\\shaders" &&
       (leaf=="capital_ship_a" || leaf=="capital_ship_b"))return Finish::CoatedMetal;
    if(owner=="scenarios\\objects\\covenant\\military\\battle_cruiser\\shaders" &&
       (leaf=="battle_cruiser_a" || leaf=="battle_cruiser_b" || leaf=="battle_cruiser_c"))
        return Finish::CoatedMetal;
    if(owner=="objects\\cinematics\\human\\inamberclad\\shaders" &&
       (leaf=="inamberclad_fore" || leaf=="inamberclad_aft" || leaf=="inamberclad_details"))
        return Finish::Metal;
    if(owner=="objects\\vehicles\\longsword\\shaders") {
        if(leaf=="longsword_hull")return Finish::Metal;
        if(leaf=="longsword_glass")return Finish::Lens;
    }
    if(owner=="scenarios\\shaders\\covenant\\military\\metals") {
        if(leaf=="cov_blue" || leaf=="cov_dark_blue" || leaf=="cov_dark_purple" ||
           leaf=="panel_f_blue" || leaf=="panel_f_purple" || leaf=="panel_g_blue" ||
           leaf=="panel_g_purple" || leaf=="panel_f_blue_ribbed" ||
           leaf=="strip_a_blue" || leaf=="strip_a_purple" || leaf=="strip_a_blue_lit" ||
           leaf=="strip_a_purple_lit" || leaf=="rounded_carapace" || leaf=="scallop")
            return Finish::CoatedMetal;
        if(leaf=="panel_f_grey" || leaf=="strip_a_grey" || leaf=="strip_b" || leaf=="grand_door")
            return Finish::Metal;
    }
    if(owner=="scenarios\\shaders\\solo\\spacestation") {
        if(leaf=="panel_floor_rubber")return Finish::Rubber;
        if(leaf=="panel_mech_strips_hazard_stripe" || leaf=="panel_mech_tile_orange")return Finish::CoatedMetal;
        if(leaf=="panel_strips_unpainted" || leaf=="exterior_gun_sheath" || leaf=="exterior_gun_bar" ||
           leaf=="panel_mech_floor" || leaf=="tile_metal_dark")return Finish::Metal;
    }
    return Finish::Unknown;
}

inline bool heretic_nearby_reflector(const Material& material) {
    using Finish=SourceSurfaceMaterial;
    const auto name=std::string_view(material.name);
    const auto finish=material.source_surface_material;
    if(finish==Finish::CoatedMetal)
        return name.starts_with("objects\\characters\\elite\\shaders\\") ||
            name=="objects\\characters\\brute\\shaders\\brute_honor_armor" ||
            name=="objects\\vehicles\\gravity_throne\\shaders\\throne_shiny";
    if(finish==Finish::Polished)return name.starts_with("objects\\characters\\prophet\\shaders\\");
    return finish==Finish::Lens && name=="objects\\characters\\masterchief\\shaders\\masterchief_visor";
}
}

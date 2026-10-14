#include "helpers/multiplayer_snapshot_checks.hpp"
#include <array>
#include <iostream>

using namespace halo2;
int main() try {
    MultiplayerState expected;
    auto &player = expected.players[0];
    player.connected = true;
    player.health = player.shield = 1;
    player.source_player = 0x80000000;
    player.source_unit = 0x80000001;
    player.source_weapon = 0x80000002;
    player.weapon = 0x80000003;
    player.first_person_clock.present = true;
    player.secondary_weapon.clock.present = true;
    expected.world_vehicles.emplace_back();
    auto& vehicle=expected.world_vehicles.back();vehicle.handle=0x80010020u;vehicle.placement=7;
    vehicle.lighting_owner=0x80010019u;vehicle.lighting_definition=0xe75a05e5u;
    vehicle.lighting_origin={1,2,3};vehicle.lighting_velocity={4,5,6};vehicle.lighting_flags=0x2000;vehicle.lighting_frozen=true;
    vehicle.geometry_flags=0x80000;vehicle.geometry_control_flags=4;
    expected.devices.groups.push_back({1,0});expected.devices.owners.emplace_back();
    auto &device=expected.devices.owners.back();device.definition=0x80010030;
    device.state.handle=0x80010040;device.state.placement=27;
    device.state.door.flags=4;device.state.door.open_ticks=-3;
    device.state.private_position={1,0};
    expected.prop_regions.emplace_back();auto &prop=expected.prop_regions.back();
    prop.handle=0x80010050;prop.placement=39;prop.definition=0xe9a00001;
    prop.regions.selection.variant_index=0;prop.regions.selection.permutations={1};
    prop.regions.selection.states={{0,4,0}};prop.regions.looping_effects={0xffffffffu};
    expected.source_scene_roots_authoritative=true;expected.source_scene_roots.emplace_back();
    auto &root=expected.source_scene_roots.back();root.object=0xe2750006;root.definition=0xea9c0100;root.placement=27;
    root.attached_effects.push_back({0,0x83,0xe6650000,0xea790904,-0.f,1,{0x01234567,0x89abcdef},
        {{0x020000ca,0,{}}}});
    struct Mutation { const char *name; void (*apply)(MultiplayerState &); };
    const Mutation mutations[] = {
        {"source owner",[](auto &s){s.source_scene_roots[0].object^=0x10000u;}},
        {"source effect count",[](auto &s){s.source_scene_roots[0].attached_effects.clear();}},
        {"source effect generation",[](auto &s){s.source_scene_roots[0].attached_effects[0].instance^=0x10000u;}},
        {"source effect slot",[](auto &s){++s.source_scene_roots[0].attached_effects[0].slot;}},
        {"source effect flags",[](auto &s){s.source_scene_roots[0].attached_effects[0].flags^=4;}},
        {"source effect signed zero",[](auto &s){s.source_scene_roots[0].attached_effects[0].scale_a=0.f;}},
        {"source effect color",[](auto &s){s.source_scene_roots[0].attached_effects[0].colors[1]^=1;}},
        {"source effect local node",[](auto &s){++s.source_scene_roots[0].attached_effects[0].locations[0].node;}},
        {"source effect local frame",[](auto &s){s.source_scene_roots[0].attached_effects[0].locations[0].local.position.x=1;}},
        {"prop bank", [](auto &s) { s.prop_regions.clear(); }},
        {"prop generation", [](auto &s) { s.prop_regions[0].handle^=0x10000; }},
        {"prop placement", [](auto &s) { ++s.prop_regions[0].placement; }},
        {"prop definition", [](auto &s) { ++s.prop_regions[0].definition; }},
        {"prop variant", [](auto &s) { ++s.prop_regions[0].regions.selection.variant_index; }},
        {"prop permutation", [](auto &s) { s.prop_regions[0].regions.selection.permutations[0]=0; }},
        {"prop damage state", [](auto &s) { ++s.prop_regions[0].regions.selection.states[0].damage; }},
        {"prop properties", [](auto &s) { ++s.prop_regions[0].regions.selection.states[0].properties; }},
        {"prop looping handle", [](auto &s) { ++s.prop_regions[0].regions.looping_effects[0]; }},
        {"device clock", [](auto &s) { ++s.devices.tick; }},
        {"shared device target", [](auto &s) { s.devices.groups[0].value=.5f; }},
        {"private device group", [](auto &s) { s.devices.owners[0].state.private_position.flags^=1; }},
        {"door flags", [](auto &s) { s.devices.owners[0].state.door.flags^=4; }},
        {"door timer", [](auto &s) { ++s.devices.owners[0].state.door.open_ticks; }},
        {"device motion", [](auto &s) { s.devices.owners[0].state.motion.position=.5f; }},
        {"device hierarchy", [](auto &s) { s.devices.owners[0].state.root.basis[0].x=.5f; }},
        {"device generation", [](auto &s) { s.devices.owners[0].state.handle^=0x10000; }},
        {"health", [](auto &s) { s.players[0].health = .5f; }},
        {"shield", [](auto &s) { s.players[0].shield = .5f; }},
        {"weapon tag", [](auto &s) { s.players[0].weapon ^= 0x10000; }},
        {"round selector", [](auto &s) { ++s.round_mode; }},
        {"match clock", [](auto &s) { ++s.clock_seconds; }},
        {"variant name", [](auto &s) { s.variant_name += "changed"; }},
        {"team statistics", [](auto &s) { ++s.team_statistics[0][2]; }},
        {"objective ownership", [](auto &s) { s.king = MultiplayerState::KingObjective{-1, 0}; }},
        {"signed zero position", [](auto &s) { s.players[0].position.x = -0.f; }},
        {"velocity", [](auto &s) { s.players[0].velocity.x = 1; }},
        {"reload timing", [](auto &s) { s.players[0].reload_seconds = 1; }},
        {"weapon shader", [](auto &s) { s.players[0].weapon_shader.weapon_flags ^= 1; }},
        {"primary charge", [](auto &s) { ++s.players[0].weapon_trigger.charge; }},
        {"primary input", [](auto &s) { ++s.players[0].weapon_trigger.input_flags; }},
        {"primary trigger mode", [](auto &s) { ++s.players[0].weapon_trigger.mode; }},
        {"primary trigger ticks", [](auto &s) { --s.players[0].weapon_trigger.ticks; }},
        {"secondary charge", [](auto &s) { ++s.players[0].secondary_weapon.trigger.charge; }},
        {"secondary input", [](auto &s) { ++s.players[0].secondary_weapon.trigger.input_flags; }},
        {"secondary trigger mode", [](auto &s) { ++s.players[0].secondary_weapon.trigger.mode; }},
        {"secondary trigger ticks", [](auto &s) { --s.players[0].secondary_weapon.trigger.ticks; }},
        {"secondary ammunition", [](auto &s) { ++s.players[0].secondary_weapon.loaded; }},
        {"grenade inventory", [](auto &s) { ++s.players[0].grenades[0]; }},
        {"grenade animation", [](auto &s) { s.players[0].grenade_action.frame = 1; }},
        {"melee timing", [](auto &s) { ++s.players[0].melee_action.progress; }},
        {"landing timing", [](auto &s) { ++s.players[0].landing.elapsed; }},
        {"profile", [](auto &s) { s.players[0].source_configuration.emplace(); }},
        {"previous unit", [](auto &s) { s.players[0].source_previous_unit ^= 1; }},
        {"respawn timer", [](auto &s) { ++s.players[0].source_respawn_countdown; }},
        {"zoom", [](auto &s) { ++s.players[0].source_zoom.motion; }},
        {"action prompt", [](auto &s) { s.players[0].action_prompt.message ^= 1; }},
        {"camouflage", [](auto &s) { s.players[0].source_camouflage.primary = .5f; }},
        {"shield feedback", [](auto &s) { s.players[0].damage_feedback.shield.visible = .5f; }},
        {"body feedback timer", [](auto &s) { ++s.players[0].damage_feedback.body.ticks; }},
        {"shield recharge", [](auto &s) { s.players[0].shield_recharging = true; }},
        {"FP origin", [](auto &s) { s.players[0].first_person_clock.origin.frame = 1; }},
        {"FP output", [](auto &s) { ++s.players[0].first_person_clock.current.fire_ticks; }},
        {"offhand FP clock", [](auto &s) { s.players[0].secondary_weapon.clock.current.rate = .5f; }},
        {"weapon effect", [](auto &s) { ++s.players[0].weapon_effect.serial; }},
        {"world weapon bank", [](auto &s) { s.world_weapons.emplace_back(); }},
        {"world vehicle bank", [](auto &s) { s.world_vehicles.emplace_back(); }},
        {"vehicle light owner", [](auto &s) { ++s.world_vehicles[0].lighting_owner; }},
        {"vehicle light definition", [](auto &s) { ++s.world_vehicles[0].lighting_definition; }},
        {"vehicle light origin", [](auto &s) { s.world_vehicles[0].lighting_origin.x=4; }},
        {"vehicle physics velocity", [](auto &s) { s.world_vehicles[0].lighting_velocity.z=-0.f; }},
        {"vehicle light query flags", [](auto &s) { s.world_vehicles[0].lighting_flags=0; }},
        {"vehicle frozen light", [](auto &s) { s.world_vehicles[0].lighting_frozen=false; }},
        {"vehicle own geometry flags", [](auto &s) { s.world_vehicles[0].geometry_flags=0; }},
        {"vehicle own geometry control", [](auto &s) { s.world_vehicles[0].geometry_control_flags=0; }},
    };
    test::verify_multiplayer_snapshot(expected, expected);
    unsigned rejected = 0;
    for (const auto &mutation : mutations) {
        auto actual = expected;
        mutation.apply(actual);
        bool detected = false;
        try { test::verify_multiplayer_snapshot(expected, actual); }
        catch (const FormatError &) { detected = true; }
        if (!detected) throw FormatError(std::string("Snapshot checker accepted changed ") + mutation.name);
        ++rejected;
    }
    std::cout << "PASS snapshot sensitivity: " << rejected << " independently corrupted field groups rejected\n";
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}

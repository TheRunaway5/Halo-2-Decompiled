#pragma once
#include "halo2/multiplayer.hpp"
#include "source_decal_frame_checks.hpp"
#include <bit>

namespace halo2::test {
// Test-only comparisons of the fields published by the native snapshot codec.
// Event banks retain their separate transport/lifetime regression tests.
inline void require_snapshot(bool value, const char *message) {
    if (!value) throw FormatError(message);
}
inline bool same_snapshot_float(float a, float b) {
    return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b);
}
inline bool same_snapshot_vector(Vec3 a, Vec3 b) {
    return same_snapshot_float(a.x, b.x) && same_snapshot_float(a.y, b.y) && same_snapshot_float(a.z, b.z);
}
inline bool same_snapshot_transform(const Transform &a,const Transform &b) {
    return same_snapshot_float(a.scale,b.scale) && same_snapshot_vector(a.position,b.position) &&
        same_snapshot_vector(a.basis[0],b.basis[0]) && same_snapshot_vector(a.basis[1],b.basis[1]) &&
        same_snapshot_vector(a.basis[2],b.basis[2]);
}
inline void verify_device_snapshot(const SourceDeviceWorldSnapshot &expected,
                                   const SourceDeviceWorldSnapshot &actual) {
    require_snapshot(expected.tick==actual.tick && expected.groups.size()==actual.groups.size() &&
        expected.owners.size()==actual.owners.size(),"UDP device clock/banks differ from authority");
    const auto same_group=[](const SourceDeviceGroup &a,const SourceDeviceGroup &b) {
        return a.flags==b.flags && same_snapshot_float(a.value,b.value);
    };
    for(std::size_t i=0;i<expected.groups.size();++i)
        require_snapshot(same_group(expected.groups[i],actual.groups[i]),"UDP shared device group differs");
    for(std::size_t i=0;i<expected.owners.size();++i) {
        const auto &a=expected.owners[i],&b=actual.owners[i];const auto &x=a.state,&y=b.state;
        require_snapshot(a.definition==b.definition && x.handle==y.handle &&
            x.placement==y.placement && x.created_tick==y.created_tick &&
            x.control_flags==y.control_flags && x.door.flags==y.door.flags &&
            x.door.open_ticks==y.door.open_ticks && x.bsp_hidden==y.bsp_hidden && x.visible==y.visible &&
            x.motion.flags==y.motion.flags && x.motion.delay_ticks==y.motion.delay_ticks &&
            same_snapshot_float(x.motion.position,y.motion.position) &&
            same_snapshot_float(x.motion.position_velocity,y.motion.position_velocity) &&
            same_snapshot_float(x.motion.power,y.motion.power) &&
            same_snapshot_float(x.motion.power_velocity,y.motion.power_velocity) &&
            same_group(x.private_position,y.private_position) && same_group(x.private_power,y.private_power) &&
            same_snapshot_transform(x.root,y.root),"UDP device identity/motion/door state differs from authority");
    }
}
inline bool same_snapshot_clock(const FirstPersonReplicatedClock &a, const FirstPersonReplicatedClock &b) {
    return a.present == b.present && (!a.present ||
        (same_first_person_channel(a.origin, b.origin) && same_first_person_channel(a.current, b.current)));
}
inline void verify_prop_region_snapshot(const MultiplayerState &expected,const MultiplayerState &actual){
    require_snapshot(expected.prop_regions.size()==actual.prop_regions.size(),"UDP prop region bank size differs from authority");
    for(unsigned i=0;i<expected.prop_regions.size();++i){
        const auto &a=expected.prop_regions[i],&b=actual.prop_regions[i];
        require_snapshot(a.handle==b.handle && a.placement==b.placement && a.definition==b.definition &&
            a.regions.selection.variant_index==b.regions.selection.variant_index &&
            a.regions.selection.permutations==b.regions.selection.permutations &&
            a.regions.selection.states==b.regions.selection.states &&
            a.regions.selection.unresolved_rng_regions==b.regions.selection.unresolved_rng_regions &&
            a.regions.looping_effects==b.regions.looping_effects,"UDP prop model-region choices differ from authority");
    }
}
inline void verify_multiplayer_snapshot(const MultiplayerState &expected, const MultiplayerState &actual) {
    require_snapshot(expected.source_object_loop_emitters.has_value()==actual.source_object_loop_emitters.has_value(),
        "UDP object-loop emitter availability differs from authority");
    if(expected.source_object_loop_emitters) {
        const auto &a=*expected.source_object_loop_emitters,&b=*actual.source_object_loop_emitters;
        require_snapshot(a.size()==b.size(),"UDP object-loop emitter bank extent differs from authority");
        for(std::size_t n=0;n<a.size();++n)
            require_snapshot(a[n].loop==b[n].loop && a[n].object==b[n].object && a[n].definition==b[n].definition &&
                a[n].source_flags==b[n].source_flags && a[n].function_active==b[n].function_active && a[n].cluster==b[n].cluster &&
                same_snapshot_float(a[n].maximum_distance,b[n].maximum_distance) &&
                same_snapshot_float(a[n].stop_duration,b[n].stop_duration) && a[n].location==b[n].location,
                "UDP completed pre-view object-loop emitter differs from authority");
    }
    require_snapshot(same_source_decal_frame(expected.source_decals,actual.source_decals),
        "UDP completed source decal cache differs from authority");
    require_snapshot(expected.source_body_poses_authoritative==actual.source_body_poses_authoritative &&
        expected.source_body_poses.size()==actual.source_body_poses.size(),"UDP source WORLD coverage bank differs");
    for(unsigned n=0;n<expected.source_body_poses.size();++n) {
        const auto &a=expected.source_body_poses[n],&b=actual.source_body_poses[n];
        require_snapshot(a.object==b.object && a.nodes.size()==b.nodes.size() &&
            same_snapshot_vector(a.sphere.center,b.sphere.center) && same_snapshot_float(a.sphere.radius,b.sphere.radius),
            "UDP completed source coverage bound differs from authority");
    }
    require_snapshot(expected.source_engine_events==actual.source_engine_events,
        "UDP original engine event payload/recipient/RNG journal differs");
    require_snapshot(expected.source_scene_roots_authoritative==actual.source_scene_roots_authoritative &&
        expected.source_scene_roots.size()==actual.source_scene_roots.size(),"UDP source scene authority/bank differs");
    for(unsigned n=0;n<expected.source_scene_roots.size();++n) {
        const auto &a=expected.source_scene_roots[n],&b=actual.source_scene_roots[n];
        require_snapshot(a.object==b.object && a.definition==b.definition && a.placement==b.placement &&
            a.created_tick==b.created_tick && a.render_clusters==b.render_clusters && a.attached_effects.size()==b.attached_effects.size(),
            "UDP attachment owner/generation differs");
        require_snapshot(a.model_regions.has_value()==b.model_regions.has_value(),"UDP source scene region presence differs");
        if(a.model_regions) {
            const auto &x=*a.model_regions,&y=*b.model_regions;
            require_snapshot(x.selection.variant_index==y.selection.variant_index &&
                x.selection.permutations==y.selection.permutations && x.selection.states==y.selection.states &&
                x.looping_effects==y.looping_effects,"UDP actual source scene regions differ");
        }
        for(unsigned i=0;i<a.attached_effects.size();++i) {
            const auto &x=a.attached_effects[i],&y=b.attached_effects[i];
            require_snapshot(x.slot==y.slot && x.instance==y.instance && x.definition==y.definition &&
                x.flags==y.flags && same_snapshot_float(x.scale_a,y.scale_a) && same_snapshot_float(x.scale_b,y.scale_b) &&
                x.colors==y.colors && x.locations.size()==y.locations.size(),"UDP source effect state differs");
            require_snapshot(x.render.has_value()==y.render.has_value(),"UDP source effect render presence differs");
            if(x.render) {
                const auto &p=*x.render,&q=*y.render;
                require_snapshot(p.identity==q.identity&&p.attached_owner==q.attached_owner&&p.local_view==q.local_view&&
                    p.flags==q.flags&&p.root_owner==q.root_owner&&p.root_type==q.root_type&&
                    same_snapshot_vector(p.color,q.color)&&same_snapshot_vector(p.velocity,q.velocity),"UDP source effect render provenance differs");
                const auto &c=x.render_clock,&d=y.render_clock;
                require_snapshot(c.flags==d.flags&&c.event==d.event&&c.previous==d.previous&&
                    same_snapshot_float(c.elapsed,d.elapsed)&&same_snapshot_float(c.target,d.target)&&
                    same_snapshot_float(c.field_68,d.field_68),"UDP source effect render clock differs");
            }
            for(unsigned j=0;j<x.locations.size();++j)
                require_snapshot(x.locations[j].name==y.locations[j].name && x.locations[j].node==y.locations[j].node &&
                    same_snapshot_transform(x.locations[j].local,y.locations[j].local),"UDP source effect local location differs");
        }
    }
    verify_prop_region_snapshot(expected,actual);
    verify_device_snapshot(expected.devices,actual.devices);
    { const auto &a = expected, &b = actual;
        require_snapshot(a.tick == b.tick &&
            a.source_game_type == b.source_game_type &&
            a.source_engine == b.source_engine &&
            a.source_engine_entity == b.source_engine_entity &&
            a.finished == b.finished &&
            a.results_finalized == b.results_finalized &&
            a.game_over_countdown == b.game_over_countdown &&
            a.game_over_fade_started == b.game_over_fade_started &&
            a.winner == b.winner &&
            a.score_limit == b.score_limit &&
            a.round_mode == b.round_mode &&
            a.source_round_lifecycle_available == b.source_round_lifecycle_available &&
            a.round_phase == b.round_phase && a.round_index == b.round_index &&
            a.round_countdown == b.round_countdown &&
            a.life_limit == b.life_limit &&
            a.time_ticks == b.time_ticks &&
            a.clock_seconds == b.clock_seconds &&
            a.variant_name == b.variant_name &&
            a.team_mode == b.team_mode &&
            a.retained_team_mask == b.retained_team_mask &&
            a.winner_team == b.winner_team &&
            a.team_scores == b.team_scores &&
            a.team_total_scores == b.team_total_scores &&
            a.team_wins == b.team_wins &&
            a.team_statistics == b.team_statistics,
            "UDP rules/clock/team/results differ from authority");
        require_snapshot(a.king.has_value() == b.king.has_value(), "UDP objective ownership differs");
        if (a.king) require_snapshot(a.king->hill == b.king->hill && a.king->occupants == b.king->occupants,
                                     "UDP objective state differs");
    }
    for (unsigned id = 0; id < multiplayer_capacity; ++id) {
        const auto &a = expected.players[id], &b = actual.players[id];
        require_snapshot(a.connected == b.connected, "UDP roster differs from authority");
        if (!a.connected) continue;
        const auto& presentation=a.source_objective_presentation;
        const auto& received_presentation=b.source_objective_presentation;
        require_snapshot(presentation.waypoints==received_presentation.waypoints &&
            presentation.goals.size()==received_presentation.goals.size(),"UDP original objective waypoint/goal bank differs");
        for(unsigned n=0;n<presentation.goals.size();++n) {
            const auto& x=presentation.goals[n];const auto& y=received_presentation.goals[n];
            require_snapshot(x.shader==y.shader && x.designator==y.designator &&
                same_snapshot_vector(x.center,y.center) && same_snapshot_vector(x.color,y.color) &&
                same_snapshot_float(x.radius,y.radius) && same_snapshot_float(x.distance_scale,y.distance_scale),
                "UDP original objective goal shader/geometry/color differs");
        }
        const auto& goals=a.source_hud_objectives;const auto& received_goals=b.source_hud_objectives;
        require_snapshot(goals.charge_visible==received_goals.charge_visible &&
            same_snapshot_float(goals.charge_fraction,received_goals.charge_fraction) &&
            goals.territory_count==received_goals.territory_count,"UDP objective HUD/arming meter differs from authority");
        for(unsigned n=0;n<goals.territory_count;++n)
            require_snapshot(goals.territories[n].color==received_goals.territories[n].color &&
                same_snapshot_float(goals.territories[n].fraction,received_goals.territories[n].fraction),
                "UDP objective HUD territory meter differs from authority");
        require_snapshot(a.dead == b.dead &&
            a.source_player == b.source_player &&
            a.source_unit == b.source_unit &&
            a.source_weapon == b.source_weapon &&
            a.source_previous_unit == b.source_previous_unit &&
            a.source_death_tick == b.source_death_tick &&
            a.name == b.name &&
            a.source_configuration == b.source_configuration &&
            a.source_flags == b.source_flags &&
            a.source_membership == b.source_membership &&
            a.source_ffa_identity == b.source_ffa_identity &&
            a.source_lives_remaining == b.source_lives_remaining &&
            a.source_king_inside_ticks == b.source_king_inside_ticks &&
            a.source_respawn_countdown == b.source_respawn_countdown &&
            a.source_respawn_seconds == b.source_respawn_seconds &&
            a.source_status == b.source_status,
            "UDP player identity/profile/lifetime differs from authority");
        require_snapshot(same_snapshot_float(a.yaw, b.yaw) &&
            same_snapshot_float(a.pitch, b.pitch) &&
            same_snapshot_float(a.crouch, b.crouch) &&
            same_snapshot_float(a.health, b.health) &&
            same_snapshot_float(a.shield, b.shield) &&
            same_snapshot_float(a.native_forward, b.native_forward) &&
            same_snapshot_float(a.native_right, b.native_right) &&
            same_snapshot_vector(a.position, b.position) &&
            same_snapshot_vector(a.velocity, b.velocity) &&
            a.native_grounded == b.native_grounded &&
            a.native_crouching == b.native_crouching &&
            a.native_support_material == b.native_support_material &&
            a.score == b.score &&
            a.kills == b.kills &&
            a.deaths == b.deaths &&
            a.assists == b.assists &&
            a.suicides == b.suicides &&
            a.total_score == b.total_score &&
            a.wins == b.wins,
            "UDP movement/vitality/statistics differ from authority");
        require_snapshot(a.weapon == b.weapon &&
            a.loaded == b.loaded &&
            a.reserve == b.reserve &&
            a.shot_tick == b.shot_tick &&
            a.reload_tick == b.reload_tick &&
            a.reload_empty == b.reload_empty &&
            a.ammunition_available == b.ammunition_available &&
            a.weapon_trigger == b.weapon_trigger &&
            a.weapon_shader.item_flags == b.weapon_shader.item_flags &&
            a.weapon_shader.weapon_flags == b.weapon_shader.weapon_flags &&
            a.grenades == b.grenades &&
            a.grenade_current == b.grenade_current &&
            same_snapshot_float(a.reload_seconds, b.reload_seconds) &&
            same_snapshot_float(a.weapon_shader.power_fraction, b.weapon_shader.power_fraction),
            "UDP weapon/inventory/reload state differs from authority");
        require_snapshot(a.grenade_action.source == b.grenade_action.source &&
            a.grenade_action.graph == b.grenade_action.graph &&
            a.grenade_action.clip == b.grenade_action.clip &&
            a.grenade_action.activity == b.grenade_action.activity &&
            a.grenade_action.animation_active == b.grenade_action.animation_active &&
            a.melee_action.activity == b.melee_action.activity &&
            a.melee_action.started == b.melee_action.started &&
            a.melee_action.modifiers == b.melee_action.modifiers &&
            a.melee_action.speed_modifier == b.melee_action.speed_modifier &&
            a.melee_action.progress == b.melee_action.progress &&
            a.melee_action.impact == b.melee_action.impact &&
            a.melee_action.recovery == b.melee_action.recovery &&
            a.melee_action.duration == b.melee_action.duration &&
            a.landing.kind == b.landing.kind &&
            a.landing.elapsed == b.landing.elapsed &&
            a.landing.duration == b.landing.duration &&
            same_snapshot_float(a.grenade_action.frame, b.grenade_action.frame),
            "UDP grenade/melee/landing actions differ from authority");
        require_snapshot(a.source_zoom.current == b.source_zoom.current &&
            a.source_zoom.requested == b.source_zoom.requested &&
            a.source_zoom.motion == b.source_zoom.motion &&
            a.zoom_reset_serial == b.zoom_reset_serial &&
            a.source_zoom_magazine_mode == b.source_zoom_magazine_mode &&
            a.source_parent == b.source_parent &&
            a.source_action_word == b.source_action_word &&
            a.source_unit_10a == b.source_unit_10a &&
            a.action_prompt.target.action == b.action_prompt.target.action &&
            a.action_prompt.target.index == b.action_prompt.target.index &&
            a.action_prompt.target.object == b.action_prompt.target.object &&
            a.action_prompt.definition == b.action_prompt.definition &&
            a.action_prompt.message == b.action_prompt.message &&
            a.source_hud_zoom_mask == b.source_hud_zoom_mask &&
            a.motion_sensor_control == b.motion_sensor_control &&
            a.shield_recharging == b.shield_recharging &&
            a.damage_feedback.shield.ticks == b.damage_feedback.shield.ticks &&
            a.damage_feedback.body.ticks == b.damage_feedback.body.ticks &&
            same_snapshot_float(a.source_camouflage.primary, b.source_camouflage.primary) &&
            same_snapshot_float(a.source_camouflage.secondary, b.source_camouflage.secondary) &&
            same_snapshot_float(a.source_camouflage.fade_speed, b.source_camouflage.fade_speed) &&
            same_snapshot_float(a.damage_feedback.shield.visible, b.damage_feedback.shield.visible) &&
            same_snapshot_float(a.damage_feedback.shield.accumulated, b.damage_feedback.shield.accumulated) &&
            same_snapshot_float(a.damage_feedback.body.visible, b.damage_feedback.body.visible) &&
            same_snapshot_float(a.damage_feedback.body.accumulated, b.damage_feedback.body.accumulated) &&
            same_snapshot_clock(a.first_person_clock, b.first_person_clock),
            "UDP zoom/prompt/camouflage/damage/FP state differs from authority");
        require_snapshot(a.secondary_weapon.trigger == b.secondary_weapon.trigger &&
            a.secondary_weapon.definition == b.secondary_weapon.definition &&
            a.secondary_weapon.datum == b.secondary_weapon.datum &&
            a.secondary_weapon.shot_tick == b.secondary_weapon.shot_tick &&
            a.secondary_weapon.reload_tick == b.secondary_weapon.reload_tick &&
            a.secondary_weapon.loaded == b.secondary_weapon.loaded &&
            a.secondary_weapon.reserve == b.secondary_weapon.reserve &&
            a.secondary_weapon.reload_empty == b.secondary_weapon.reload_empty &&
            a.secondary_weapon.shader.item_flags == b.secondary_weapon.shader.item_flags &&
            a.secondary_weapon.shader.weapon_flags == b.secondary_weapon.shader.weapon_flags &&
            a.secondary_weapon.ammunition_available == b.secondary_weapon.ammunition_available &&
            a.weapon_effect.serial == b.weapon_effect.serial &&
            a.weapon_effect.tick == b.weapon_effect.tick &&
            a.weapon_effect.unit == b.weapon_effect.unit &&
            a.weapon_effect.weapon == b.weapon_effect.weapon &&
            a.weapon_effect.effect == b.weapon_effect.effect &&
            a.weapon_effect.a_bits == b.weapon_effect.a_bits &&
            a.weapon_effect.b_bits == b.weapon_effect.b_bits &&
            same_snapshot_float(a.secondary_weapon.reload_seconds, b.secondary_weapon.reload_seconds) &&
            same_snapshot_float(a.secondary_weapon.shader.power_fraction, b.secondary_weapon.shader.power_fraction) &&
            same_snapshot_clock(a.secondary_weapon.clock, b.secondary_weapon.clock),
            "UDP offhand/effect state differs from authority");
    }
    require_snapshot(expected.world_weapons.size() == actual.world_weapons.size(), "UDP world weapon bank differs");
    for (std::size_t index = 0; index < expected.world_weapons.size(); ++index) {
        const auto &a = expected.world_weapons[index], &b = actual.world_weapons[index];
        require_snapshot(a.handle == b.handle &&
            a.definition == b.definition &&
            a.loaded == b.loaded &&
            a.reserve == b.reserve &&
            a.shader.item_flags == b.shader.item_flags &&
            a.shader.weapon_flags == b.shader.weapon_flags &&
            same_snapshot_vector(a.position, b.position) &&
            same_snapshot_vector(a.forward, b.forward) &&
            same_snapshot_vector(a.up, b.up) &&
            same_snapshot_float(a.shader.power_fraction, b.shader.power_fraction),
            "UDP world weapon identity/pose/state differs from authority");
    }
    require_snapshot(expected.world_vehicles.size() == actual.world_vehicles.size(), "UDP vehicle bank differs");
    for (std::size_t index = 0; index < expected.world_vehicles.size(); ++index) {
        const auto &a = expected.world_vehicles[index], &b = actual.world_vehicles[index];
        require_snapshot(a.handle == b.handle &&
            a.placement == b.placement &&
            a.lighting_owner == b.lighting_owner && a.lighting_definition == b.lighting_definition &&
            a.lighting_flags == b.lighting_flags && a.lighting_frozen == b.lighting_frozen &&
            a.geometry_flags == b.geometry_flags && a.geometry_control_flags == b.geometry_control_flags &&
            same_snapshot_vector(a.lighting_origin,b.lighting_origin) && same_snapshot_vector(a.lighting_velocity,b.lighting_velocity) &&
            same_snapshot_float(a.transform.scale, b.transform.scale) &&
            same_snapshot_vector(a.transform.position, b.transform.position) &&
            same_snapshot_vector(a.transform.basis[0], b.transform.basis[0]) &&
            same_snapshot_vector(a.transform.basis[1], b.transform.basis[1]) &&
            same_snapshot_vector(a.transform.basis[2], b.transform.basis[2]),
            "UDP vehicle identity/transform/lighting inputs differ from authority");
    }
}
} // namespace halo2::test

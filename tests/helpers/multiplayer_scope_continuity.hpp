#pragma once
#include "halo2/multiplayer_pose_continuity.hpp"
#include <bit>
#include <stdexcept>
#include <vector>

namespace halo2::test {
// Declared local camera storage controls using the authenticated source
// operands and the same 3x transition as the frozen original camera oracle.
// This does not assert that the received player's weapon has a 3x scope.
inline void scope_continuity_step(MultiplayerPoseContinuity::Values &values,
    const UnitScopeCameraConstants &constants,unsigned view,bool zoomed) {
    UnitScopeCameraPacket packet{1,0,0,0,0};
    source_scope_camera_target(values.scope_producers.at(view),packet,
        source_scope_camera_fov(constants.default_fov,zoomed?constants.three:constants.one,constants),constants);
    auto &observer=values.scope_observers.at(view);
    source_scope_camera_register(observer,packet);
    if(!source_scope_camera_adopt(observer,packet,constants)||
        source_scope_camera_motion(observer,source_scope_camera_dt(1.f/30.f,constants),constants))
        throw std::runtime_error("Scope continuation control lacks its source channel owner");
    source_scope_camera_render_fov(observer,constants);
}
inline void prime_scope_continuity(MultiplayerPoseContinuity::Values &values,
    const UnitScopeCameraConstants &constants,const MultiplayerSnapshotOrigin &origin) {
    for(unsigned view=0;view<origin.local_slots().size();++view){
        values.scope_producers[view]={};
        values.scope_observers[view]=source_scope_camera_initial(constants);
        scope_continuity_step(values,constants,view,false);
        for(unsigned frame=0;frame<=view;++frame)scope_continuity_step(values,constants,view,true);
        const auto &player=origin.accepted_state().players[origin.local_slots()[view]];
        values.view_zoom[view]=0;
        values.view_zoom_unit[view]=player.source_unit;
        values.view_zoom_reset[view]=player.zoom_reset_serial;
        values.view_was_dead[view]=player.dead;
        if(!(values.scope_observers[view].remaining>0))
            throw std::runtime_error("Scope continuation control did not retain an unfinished transition");
    }
}
inline std::vector<std::uint32_t> scope_continuity_words(const MultiplayerPoseContinuity::Values &values) {
    std::vector<std::uint32_t> words;
    const auto scalar=[&](float value){words.push_back(std::bit_cast<std::uint32_t>(value));};
    for(unsigned view=0;view<4;++view){
        const auto &observer=values.scope_observers[view];
        scalar(values.scope_producers[view].persistent_fov);
        words.insert(words.end(),{std::uint32_t(observer.registered),observer.registered_flags,observer.flags,observer.fov_flags});
        for(float value:{observer.current,observer.target,observer.remaining,observer.velocity,observer.acceleration,observer.delta})scalar(value);
        for(float value:observer.coefficients)scalar(value);
        scalar(observer.rendered_fov);
        words.insert(words.end(),{std::uint32_t(std::uint16_t(values.view_zoom[view])),values.view_zoom_unit[view],
            values.view_zoom_reset[view],std::uint32_t(values.view_was_dead[view])});
    }
    return words;
}
}

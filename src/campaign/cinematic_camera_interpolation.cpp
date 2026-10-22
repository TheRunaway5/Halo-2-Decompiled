#include "halo2/cinematic_camera_interpolation.hpp"
#include "halo2/cache.hpp"
#include <glm/gtc/quaternion.hpp>
#include <cmath>
#include <cstring>
namespace halo2 {
SourceCampaignCameraViewTu15 CinematicCameraInterpolation::sample(const CinematicCameraFrame &frame) {
    if(!std::isfinite(frame.fraction) || frame.fraction<0 || frame.fraction>1)
        throw FormatError("Invalid cinematic camera render fraction");
    if(reference_held_)return frame.completed;
    if(!valid_ || !frame.paired || frame.command!=command_ || frame.restores!=restores_ ||
       frame.bsp!=bsp_ || frame.tick<tick_ || frame.tick-tick_>1) {
        before_=current_=frame.completed;tick_=frame.tick;
        command_=frame.command;restores_=frame.restores;bsp_=frame.bsp;
        valid_=true;paired_=false;observed_subframe_=false;eligible_=0;stable_=31;
        fraction_=frame.fraction;return current_;
    }
    if(frame.tick!=tick_) {
        // Channels already moving within a tick keep the original per-frame
        // output. Adopt smoothing only after observing a held source interval.
        if(observed_subframe_)eligible_=stable_;
        before_=current_;tick_=frame.tick;paired_=true;
        observed_subframe_=false;stable_=31;fraction_=frame.fraction;
    } else {
        const auto same=[](const auto &a,const auto &b){return !std::memcmp(&a,&b,sizeof(a));};
        observed_subframe_=true;
        if(!same(current_.position,frame.completed.position))stable_&=~1u;
        if(!same(current_.forward,frame.completed.forward)||!same(current_.up,frame.completed.up))stable_&=~2u;
        if(!same(current_.horizontal_field_of_view,frame.completed.horizontal_field_of_view))stable_&=~4u;
        if(!same(current_.aspect,frame.completed.aspect))stable_&=~8u;
        if(!same(current_.aim_displacement,frame.completed.aim_displacement))stable_&=~16u;
        // A channel may start updating at render frequency during the shot.
        // Stop smoothing it on that first observed update, within this tick.
        eligible_&=stable_;
        // A paused/reset fractional clock must not rewind the displayed view.
        fraction_=std::max(fraction_,frame.fraction);
    }
    const auto fraction=fraction_;
    current_=frame.completed;
    if(!paired_ || !eligible_ || fraction==1 || !std::memcmp(&before_,&current_,sizeof(current_)))return current_;
    if(fraction==0 && eligible_==31)return before_;
    auto result=current_;
    const auto mix=[&](float a,float b){return std::lerp(a,b,fraction);};
    if(eligible_&1)result.position=fraction==0?before_.position:Vec3{
        mix(before_.position.x,current_.position.x),mix(before_.position.y,current_.position.y),mix(before_.position.z,current_.position.z)};
    if((eligible_&2) && (std::memcmp(&before_.forward,&current_.forward,sizeof(Vec3)) ||
       std::memcmp(&before_.up,&current_.up,sizeof(Vec3)))) {
        if(fraction==0){result.forward=before_.forward;result.up=before_.up;}
        else {
        const auto rotation=[](const auto &view) {
            const auto f=glm::normalize(glm::vec3(view.forward.x,view.forward.y,view.forward.z));
            const auto left=glm::normalize(glm::cross(glm::vec3(view.up.x,view.up.y,view.up.z),f));
            return glm::normalize(glm::quat_cast(glm::mat3(f,left,glm::cross(f,left))));
        };
        const auto basis=glm::mat3_cast(glm::slerp(rotation(before_),rotation(current_),fraction));
        result.forward={basis[0].x,basis[0].y,basis[0].z};result.up={basis[2].x,basis[2].y,basis[2].z};
        }
    }
    if(eligible_&4)result.horizontal_field_of_view=fraction==0?before_.horizontal_field_of_view:mix(before_.horizontal_field_of_view,current_.horizontal_field_of_view);
    if(eligible_&8)result.aspect=fraction==0?before_.aspect:mix(before_.aspect,current_.aspect);
    if(eligible_&16)result.aim_displacement=fraction==0?before_.aim_displacement:mix(before_.aim_displacement,current_.aim_displacement);
    return result;
}
}

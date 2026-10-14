#include "halo2/scene.hpp"
#include "world_continuation_bindings.hpp"
#include <algorithm>
#include <type_traits>

namespace halo2 {
namespace {
using namespace world_continuation;
bool finite_camera(const Camera &c) {
    return finite(c.position) && finite(c.forward) && finite(c.up) && std::isfinite(c.yaw) && std::isfinite(c.pitch) &&
        std::isfinite(c.field_of_view) && std::isfinite(c.roll) && (!c.projection_vertical_angle || std::isfinite(*c.projection_vertical_angle)) &&
        std::isfinite(c.projection_clip_scale.x) && std::isfinite(c.projection_clip_scale.y) &&
        std::isfinite(c.projection_clip_offset.x) && std::isfinite(c.projection_clip_offset.y);
}
void validate(const Presentation::AuthorityCheckpoint &s,const LevelContent &content,bool loader) {
    if(s.version!=1 || s.loader!=loader || s.content_binding!=presentation_content(content))
        throw FormatError("Presentation continuation has incompatible content or loader binding");
    for(const auto &t:s.transitions)if(!std::isfinite(t.from) || !std::isfinite(t.to) || t.duration<0 || t.elapsed<0 || t.elapsed>t.duration)
        throw FormatError("Presentation continuation has an invalid transition frontier");
    if(!finite(s.fade_color) || !finite_camera(s.camera) || !finite_camera(s.camera_from) || !finite_camera(s.camera_to) ||
        !finite(s.observer.position) || !finite(s.observer.forward) || !finite(s.observer.up) ||
        !std::ranges::all_of(s.observer.velocity,[](auto f){return std::isfinite(f);}) ||
        !std::ranges::all_of(s.observer.acceleration,[](auto f){return std::isfinite(f);}) ||
        s.camera_duration<0 || s.camera_elapsed<0 || s.camera_elapsed>s.camera_duration ||
        !finite(s.anchor.position) || !std::isfinite(s.anchor.yaw) || !std::isfinite(s.anchor.pitch))
        throw FormatError("Presentation continuation has invalid camera/observer state");
    if(s.animation) {
        const auto &a=*s.animation;
        if(a.nodes.size()!=1 || !a.frame_count || a.frame_count>std::uint32_t(INT32_MAX) ||
            a.channels.size()!=1 || a.components.size()!=1 || s.camera_duration!=int(a.frame_count-1))
            throw FormatError("Presentation continuation has an invalid camera animation shape");
        for(unsigned channel=0;channel<3;++channel) {
            const auto &c=a.channels[0][channel];
            if(c.times.size()!=c.keys.size() || !std::is_sorted(c.times.begin(),c.times.end()))
                throw FormatError("Presentation continuation has invalid animation key order");
            for(const auto &key:c.keys)for(auto value:key)if(!std::isfinite(value))
                throw FormatError("Presentation continuation has a nonfinite animation key");
        }
        (void)sample_animation(a,0,float(s.camera_elapsed));
    }
}
}
struct Presentation::PreparedAuthority::Impl {
    Presentation *owner{};bool *staging{};AuthorityCheckpoint saved;
    ~Impl(){if(staging)*staging=false;}
};
Presentation::PreparedAuthority::PreparedAuthority()=default;
Presentation::PreparedAuthority::~PreparedAuthority()=default;
Presentation::PreparedAuthority::PreparedAuthority(PreparedAuthority &&) noexcept=default;
Presentation::PreparedAuthority &Presentation::PreparedAuthority::operator=(PreparedAuthority &&) noexcept=default;
bool Presentation::PreparedAuthority::for_owner(const Presentation &owner)const noexcept{return impl_ && impl_->owner==&owner;}
Presentation::AuthorityCheckpoint Presentation::authority_checkpoint()const {
    authority_.capture();AuthorityCheckpoint s;s.content_binding=world_continuation::presentation_content(content_);s.loader=bool(loader_);
    s.cinematic=cinematic;s.scripted=scripted;s.player_camera_enabled=player_camera_enabled;s.cinematic_outro=cinematic_outro;
    const std::array<const Transition *,4> transitions{&fade_,&letterbox_,&hud_,&fov_};
    for(unsigned i=0;i<4;++i){const auto &t=*transitions[i];s.transitions[i]={t.from,t.to,t.duration,t.elapsed};}
    s.fade_color=fade_color_;s.camera=camera_;s.camera_from=camera_from_;s.camera_to=camera_to_;s.observer=observer_;
    s.camera_duration=camera_duration_;s.camera_elapsed=camera_elapsed_;s.animation=animation_;s.anchor=anchor_;
    validate(s,content_,bool(loader_));return s;
}
Presentation::PreparedAuthority Presentation::prepare_authority(const AuthorityCheckpoint &saved) {
    authority_.prepare();PreparedAuthority result;result.impl_=std::make_unique<PreparedAuthority::Impl>();
    result.impl_->owner=this;result.impl_->staging=&authority_.staging;authority_.staging=true;
    validate(saved,content_,bool(loader_));result.impl_->saved=saved;return result;
}
bool Presentation::commit_authority(PreparedAuthority &&prepared) noexcept {
    if(!prepared.for_owner(*this) || !authority_.staging || authority_.depth)return false;
    auto &s=prepared.impl_->saved;const std::array<Transition *,4> transitions{&fade_,&letterbox_,&hud_,&fov_};
    for(unsigned i=0;i<4;++i){const auto &t=s.transitions[i];*transitions[i]={t.from,t.to,t.duration,t.elapsed};}
    cinematic=s.cinematic;scripted=s.scripted;player_camera_enabled=s.player_camera_enabled;cinematic_outro=s.cinematic_outro;
    fade_color_=s.fade_color;camera_=s.camera;camera_from_=s.camera_from;camera_to_=s.camera_to;observer_=s.observer;
    camera_duration_=s.camera_duration;camera_elapsed_=s.camera_elapsed;
    static_assert(std::is_nothrow_swappable_v<std::optional<AnimationClip>> && std::is_nothrow_swappable_v<CutsceneFlag>);
    animation_.swap(s.animation);std::swap(anchor_,s.anchor);authority_.fault=false;prepared.impl_.reset();return true;
}
}

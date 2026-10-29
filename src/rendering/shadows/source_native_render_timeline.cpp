#include "halo2/source_native_render_timeline.hpp"
#include "halo2/source_shadow_presentation_owner.hpp"
#include "halo2/source_shadow_transport.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
bool source_native_render_continuous(const SourceNativeRenderContinuity &p)noexcept {
    return p.same_generation && p.same_world && p.previous_serial && p.current_serial>p.previous_serial &&
        std::int32_t(p.current_sequence-p.previous_sequence)>0 && p.current_tick>p.previous_tick &&
        p.current_tick-p.previous_tick<=120 && std::isfinite(p.previous_seconds) &&
        std::isfinite(p.current_seconds) && p.previous_seconds>=0 && p.current_seconds>=p.previous_seconds;
}
bool source_native_render_fresh(std::uint64_t native_tick,std::uint64_t ordinary_tick,
    std::uint64_t actual_front_tick)noexcept {
    return ordinary_tick>=native_tick && actual_front_tick>=native_tick &&
        ordinary_tick-native_tick<=120 && actual_front_tick-native_tick<=120;
}
const char* source_native_render_suspension_name(SourceNativeRenderSuspension why)noexcept {
    switch(why) {
    case SourceNativeRenderSuspension::none:return "none";
    case SourceNativeRenderSuspension::missing_cut:return "missing_cut";
    case SourceNativeRenderSuspension::foreground_clock:return "foreground_clock";
    case SourceNativeRenderSuspension::connection_or_ack:return "connection_or_ack";
    case SourceNativeRenderSuspension::native_owner:return "native_owner";
    case SourceNativeRenderSuspension::ordinary_body:return "ordinary_body";
    case SourceNativeRenderSuspension::native_ahead:return "native_ahead";
    case SourceNativeRenderSuspension::native_stale:return "native_stale";
    case SourceNativeRenderSuspension::local_identity:return "local_identity";
    }
    return "unknown";
}
struct SourceNativeRenderFrameContext::Data {
    AcceptedShadowCut cut;
    MultiplayerSnapshotOrigin before,now;
    std::optional<AuthorityTransferContext> authority_context;
    SourceWorldInterpolation world_nodes;
    SceneInterpolation scene_nodes;
    double captured_seconds{};
    std::uint64_t world_epoch=1;
    bool paired{};
};
namespace {
bool same_local_identity(const MultiplayerState::Player &a,const MultiplayerState::Player &b) {
    return a.connected==b.connected && a.dead==b.dead && a.source_player==b.source_player &&
        a.source_unit==b.source_unit && a.source_weapon==b.source_weapon && a.weapon==b.weapon &&
        a.secondary_weapon.datum==b.secondary_weapon.datum && a.secondary_weapon.definition==b.secondary_weapon.definition &&
        a.source_configuration==b.source_configuration && a.source_parent==b.source_parent &&
        a.source_seat==b.source_seat && a.source_teleport_serial==b.source_teleport_serial;
}
std::vector<std::uint32_t> body_cuts(const MultiplayerState &before,const MultiplayerState &now) {
    std::vector<std::uint32_t> cuts;
    for(unsigned slot=0;slot<now.players.size();++slot) {
        const auto &a=before.players[slot],&b=now.players[slot];
        const bool body=a.connected!=b.connected || a.dead!=b.dead || a.source_player!=b.source_player ||
            a.source_unit!=b.source_unit || a.source_configuration!=b.source_configuration ||
            a.source_parent!=b.source_parent || a.source_seat!=b.source_seat ||
            a.source_teleport_serial!=b.source_teleport_serial || length(a.position-b.position)>=5;
        if(body)for(auto object:{a.source_unit,b.source_unit})if(object!=null_datum)cuts.push_back(object);
        if(body || a.source_weapon!=b.source_weapon || a.weapon!=b.weapon ||
            a.secondary_weapon.datum!=b.secondary_weapon.datum || a.secondary_weapon.definition!=b.secondary_weapon.definition)
            for(auto object:{a.source_weapon,b.source_weapon,a.secondary_weapon.datum,b.secondary_weapon.datum})
                if(object!=null_datum)cuts.push_back(object);
    }
    return cuts;
}
}
SourceNativeRenderFrameContext::SourceNativeRenderFrameContext(std::shared_ptr<const Data> data,
    MultiplayerObjectRuntime *world,const MultiplayerMatch *match,float blend)
    :data_(std::move(data)),world_(world),match_(match),blend_(blend){}
const MultiplayerState& SourceNativeRenderFrameContext::previous()const{return data_->before.accepted_state();}
const MultiplayerState& SourceNativeRenderFrameContext::current()const{return data_->now.accepted_state();}
const MultiplayerSnapshotOrigin& SourceNativeRenderFrameContext::previous_origin()const{return data_->before;}
const MultiplayerSnapshotOrigin& SourceNativeRenderFrameContext::origin()const{return data_->now;}
std::uint64_t SourceNativeRenderFrameContext::serial()const{return data_->cut.serial();}
std::uint64_t SourceNativeRenderFrameContext::generation()const{return data_->cut.generation();}
std::uint64_t SourceNativeRenderFrameContext::world_epoch()const{return data_->world_epoch;}
const std::optional<AuthorityTransferContext>& SourceNativeRenderFrameContext::authority_context()const{return data_->authority_context;}
const MultiplayerMatch& SourceNativeRenderFrameContext::match()const{return *match_;}
const Simulation& SourceNativeRenderFrameContext::simulation()const{return match_->simulation();}
const SourceWorldInterpolation& SourceNativeRenderFrameContext::world_interpolation()const{return data_->world_nodes;}
const SceneInterpolation& SourceNativeRenderFrameContext::scene_interpolation()const{return data_->scene_nodes;}
std::vector<Transform> SourceNativeRenderFrameContext::sample_nodes(std::uint32_t object,
    const SourceWorldInterpolation::ModelProvider &models)const{return data_->world_nodes.sample(object,blend_,models);}
void SourceNativeRenderTimeline::capture(const LanClient &client,const SourceShadowPresentationOwner &owner,
    const AcceptedShadowCut &cut,double seconds) {
    if(!std::isfinite(seconds) || seconds<0 || (current_ && seconds<current_->captured_seconds))
        throw FormatError("Native render timeline requires a finite monotonic foreground clock");
    const auto &origin=cut.origin();
    auto *world=owner.world(client,origin);const auto *match=owner.match(client,origin);
    if(!world || !match || !client.acknowledged_shadow_cut(cut) || !origin.tick() || !origin.sequence() ||
        !cut.serial() || origin.accepted_state().tick!=*origin.tick() ||
        match->simulation().ticks()!=*origin.tick())
        throw FormatError("Native render capture lacks the exact adopted and acknowledged body/world");
    if(current_ && current_->now.same(origin) && current_->cut.serial()==cut.serial() &&
        current_->cut.same_generation(cut))return;
    auto next=std::make_shared<SourceNativeRenderFrameContext::Data>();
    next->cut=cut;next->before=next->now=origin;next->authority_context=origin.context();next->captured_seconds=seconds;
    const auto &now=origin.accepted_state();
    next->world_nodes.reset(now.tick,now.source_body_poses);
    next->scene_nodes.reset(now.tick,match->simulation().scene().objects());
    if(current_) {
        next->world_epoch=current_->world_epoch+(current_->cut.same_world(cut)?0u:1u);
        const auto &previous=current_->now.accepted_state();
        const bool continuous=source_native_render_continuous({current_->cut.same_generation(cut),
            current_->cut.same_world(cut),current_->cut.serial(),cut.serial(),previous.tick,now.tick,
            *current_->now.sequence(),*origin.sequence(),current_->captured_seconds,seconds}) &&
            previous.source_body_poses_authoritative==now.source_body_poses_authoritative;
        if(continuous) {
            next->before=current_->now;next->paired=true;
            next->world_nodes=current_->world_nodes;next->scene_nodes=current_->scene_nodes;
            const SourceNativeCutPermit permit(previous.tick,now.tick);
            const auto cuts=body_cuts(previous,now);
            next->world_nodes.capture_native_cut(permit,now.source_body_poses,cuts);
            next->scene_nodes.capture_native_cut(permit,match->simulation().scene().objects());
        }
    }
    current_=std::move(next);
}
std::optional<SourceNativeRenderFrameContext> SourceNativeRenderTimeline::frame(const LanClient &client,
    const SourceShadowPresentationOwner &owner,const MultiplayerSnapshotOrigin &latest,
    const MultiplayerState &latest_state,double seconds,SourceNativeRenderSuspension *status)const {
    const auto suspend=[&](SourceNativeRenderSuspension why)->std::optional<SourceNativeRenderFrameContext> {
        if(status)*status=why;return {};
    };
    if(status)*status=SourceNativeRenderSuspension::none;
    if(!current_)return suspend(SourceNativeRenderSuspension::missing_cut);
    if(!std::isfinite(seconds) || seconds<current_->captured_seconds)
        return suspend(SourceNativeRenderSuspension::foreground_clock);
    if(!client.owns_native_render_pair(current_->cut,latest))
        return suspend(SourceNativeRenderSuspension::connection_or_ack);
    auto *world=owner.world(client,current_->now);const auto *match=owner.match(client,current_->now);
    if(!world || !match || match->simulation().ticks()!=*current_->now.tick())
        return suspend(SourceNativeRenderSuspension::native_owner);
    const auto &now=current_->now.accepted_state(),&actual_latest=client.state();
    if(!latest.tick() || *latest.tick()!=latest_state.tick ||
        current_->now.local_slots().size()!=client.slots().size() ||
        !std::equal(current_->now.local_slots().begin(),current_->now.local_slots().end(),client.slots().begin()))
        return suspend(SourceNativeRenderSuspension::ordinary_body);
    if(now.tick>latest_state.tick || now.tick>actual_latest.tick)
        return suspend(SourceNativeRenderSuspension::native_ahead);
    // Holding a closed older world is safe only within the same explicit
    // four-second source-time bound as sparse native interpolation. A stalled
    // native lane never freezes newer ordinary gameplay or invents catch-up.
    if(!source_native_render_fresh(now.tick,latest_state.tick,actual_latest.tick))
        return suspend(SourceNativeRenderSuspension::native_stale);
    const auto &accepted_latest=latest.accepted_state();
    for(unsigned slot:current_->now.local_slots())if(slot>=now.players.size() ||
        !same_local_identity(now.players[slot],actual_latest.players[slot]) ||
        !same_local_identity(now.players[slot],accepted_latest.players[slot]) ||
        !same_local_identity(now.players[slot],latest_state.players[slot]))
        return suspend(SourceNativeRenderSuspension::local_identity);
    const double interval=current_->paired?double(now.tick-current_->before.accepted_state().tick)/30.0:0;
    const float blend=interval>0?float(std::clamp((seconds-current_->captured_seconds)/interval,0.0,1.0)):1.f;
    return SourceNativeRenderFrameContext(current_,world,match,blend);
}
void SourceNativeRenderTimeline::reset()noexcept{current_.reset();}
}

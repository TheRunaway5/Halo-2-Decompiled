#include "halo2/source_peer_host_takeover.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_authority_receiver.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>

namespace halo2 {
namespace {
void need(bool value){if(!value)throw FormatError("Invalid native peer host takeover ownership, decision or admission");}
bool current(const OnlineMatchSession &session,const PeerMigrationDecision &decision,const AcceptedSourcePlayers &admissions){
    const auto agreement=session.peer_migration();
    return session.phase()==OnlineMatchSession::Phase::Migrating&&session.self()==decision.value.host&&
        session.authority_context()==decision.previous&&session.source_players()==admissions&&
        agreement&&agreement->decision&&*agreement->decision==decision;
}
}
std::unique_ptr<SourcePeerHostTakeover> SourcePeerHostTakeover::prepare(const MultiplayerOptions &input,
    const SourceAuthorityRetainedWorld &retained,const PeerMigrationDecision &decision,
    std::shared_ptr<PhysicsLiveClockSource> candidate_clock,std::shared_ptr<PhysicsLiveClockSource> workspace_clock) {
    const auto session=input.online_match;
    need(session&&retained.host&&current(*session,decision,retained.admissions)&&
        retained.cut.peer==session->self()&&retained.cut.context==decision.previous&&retained.cut.seal==decision.value.cut&&
        retained.admissions.context==decision.previous&&retained.admissions.players.size()==session->roster().size()&&
        retained.local_views==session->local_slots()&&retained.local_views.size()==input.local_players&&
        retained.checkpoint.match.simulation.ticks==decision.value.cut.tick);
    std::vector<unsigned> producer_views;
    for(const auto &p:session->roster())if(online_admission_peer(p)==decision.previous.host)producer_views.push_back(p.slot);
    need(retained.producer_views==producer_views);
    MultiplayerOptions options;options.online_match=session;options.map=input.map;options.shared_maps=input.shared_maps;
    options.gameplay_xbe=input.gameplay_xbe;options.bsp=input.bsp;options.variant=input.variant;
    options.starting_weapon=input.starting_weapon;options.session_random_seed=input.session_random_seed;
    options.local_players=input.local_players;options.selected_teams=input.selected_teams;options.selected_profiles=input.selected_profiles;
    auto target=MultiplayerPreparedWorld::source_shadow(options,MultiplayerPreparedWorld::SourceShadowViews::LocalPeer,std::move(candidate_clock));
    auto workspace=MultiplayerPreparedWorld::source_shadow(options,MultiplayerPreparedWorld::SourceShadowViews::IncomingHost,std::move(workspace_clock));
    SourceMatchAuthority authority(target->match,*target->source_objects),staging(workspace->match,*workspace->source_objects);
    std::vector<SourcePlayerAdmission> locals;
    for(auto slot:retained.local_views){
        const auto found=std::ranges::find(retained.admissions.players,slot,&SourcePlayerAdmission::slot);
        need(found!=retained.admissions.players.end());locals.push_back(*found);
        if(!options.selected_profiles.empty())need(retained.checkpoint.match.players.at(slot).source_configuration==target->match.player(slot).source_configuration);
    }
    auto prepared=authority.prepare_with_local_views(retained.checkpoint,source_authority_binding(decision.previous),locals,
        staging,retained.admissions.players,producer_views);
    need(authority.commit(std::move(prepared))&&current(*session,decision,retained.admissions));
    return std::unique_ptr<SourcePeerHostTakeover>(new SourcePeerHostTakeover(session,decision,retained.admissions,std::move(target),*retained.host));
}
SourcePeerHostTakeover::SourcePeerHostTakeover(std::shared_ptr<OnlineMatchSession> session,PeerMigrationDecision decision,
    AcceptedSourcePlayers admissions,std::unique_ptr<MultiplayerPreparedWorld> world,const LanHost::Continuation &saved)
    :session_(std::move(session)),decision_(std::move(decision)),admissions_(std::move(admissions)),world_(std::move(world)) {
    need(session_&&current(*session_,decision_,admissions_)&&world_&&world_->source_objects&&
        world_->local_slots==session_->local_slots());
    const auto fingerprint=session_->loaded_fingerprint();need(fingerprint.has_value());
    SourceMatchAuthority authority(world_->match,*world_->source_objects);
    need(authority.execution_tick()==decision_.value.cut.tick);
    auto resumed=LanHost::resume_after_host_loss(world_->match,session_,*fingerprint,session_->roster(),saved,
        decision_.previous,decision_.next,admissions_.players);
    host_=std::move(resumed.host);local_=std::move(resumed.local);
    need(local_.slots==world_->local_slots&&local_.camera.has_value());
    camera_=std::make_unique<NativeVehicleCamera>(*world_->source_objects,*world_->cache->original_data());
    auto camera=camera_->prepare(*local_.camera,source_authority_binding(decision_.next));
    need(camera_->commit(std::move(camera)));
    // Original deferred departure keeps allocation/result identities and does
    // not manufacture numeric deaths for the old host's complete couch bank.
    for(const auto &p:session_->roster())if(online_admission_peer(p)==decision_.previous.host)
        world_->match.depart(p.slot);
    need(authority.execution_tick()==decision_.value.cut.tick&&current(*session_,decision_,admissions_));
    prepared_=authority.execution_boundary();
}
SourcePeerHostTakeover::~SourcePeerHostTakeover()=default;
void SourcePeerHostTakeover::own()const{need(std::this_thread::get_id()==executor_);}
const PeerMigrationDecision &SourcePeerHostTakeover::decision()const{own();return decision_;}
MultiplayerPreparedWorld &SourcePeerHostTakeover::world(){own();return *world_;}
LanHost &SourcePeerHostTakeover::host(){own();need(active_);return *host_;}
LanHost::LocalContinuation &SourcePeerHostTakeover::local(){own();return local_;}
NativeVehicleCamera &SourcePeerHostTakeover::camera(){own();return *camera_;}
bool SourcePeerHostTakeover::activate(){
    own();if(active_||!current(*session_,decision_,admissions_))return false;
    SourceMatchAuthority authority(world_->match,*world_->source_objects);
    if(authority.execution_boundary()!=prepared_)return false;
    active_=session_->finish_peer_migration(decision_);return active_;
}
}

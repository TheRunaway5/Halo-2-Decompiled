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
    std::shared_ptr<PhysicsLiveClockSource> candidate_clock) {
    const auto session=input.online_match;
    need(session&&retained.host&&current(*session,decision,retained.admissions)&&
        retained.cut.peer==session->self()&&retained.cut.context==decision.previous&&retained.cut.seal==decision.value.cut&&
        retained.admissions.context==decision.previous&&retained.admissions.players.size()==session->roster().size()&&
        retained.local_views==session->local_slots()&&retained.local_views.size()==input.local_players&&
        retained.checkpoint.match.simulation.ticks==decision.value.cut.tick);
    std::vector<unsigned> producer_views;
    for(const auto &p:session->roster())if(online_admission_peer(p)==decision.previous.host)producer_views.push_back(p.slot);
    need(retained.producer_views==producer_views);
    const auto options=source_authority_options(input);
    auto target=MultiplayerPreparedWorld::source_shadow(options,MultiplayerPreparedWorld::SourceShadowViews::IncomingHost,std::move(candidate_clock));
    need(target->local_slots==producer_views);
    SourceMatchAuthority authority(target->match,*target->source_objects);
    for(auto slot:retained.local_views){
        const auto found=std::ranges::find(retained.admissions.players,slot,&SourcePlayerAdmission::slot);
        const auto &player=retained.checkpoint.match.players.at(slot);
        need(found!=retained.admissions.players.end()&&player.connected&&!(player.source_flags&2u)&&
            player.source_datum!=null_datum&&player.source_datum==found->player);
        if(!options.selected_profiles.empty())need(retained.checkpoint.match.players.at(slot).source_configuration==target->match.player(slot).source_configuration);
    }
    // This whole owner is disposable until activation. Its initial producer
    // view bank validates the exact cut; subsequent local binding runs against
    // that imported native state, not a newly born recipient prototype. A
    // second transactional workspace is unnecessary for this private import.
    auto prepared=authority.prepare(retained.checkpoint,source_authority_binding(decision.previous),retained.admissions.players);
    need(authority.commit(std::move(prepared)));
    target->bind_source_local_views(retained.local_views);
    need(current(*session,decision,retained.admissions));
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
    input_cut_=saved;
    need(local_.slots==world_->local_slots&&local_.camera.has_value());
    camera_=std::make_unique<NativeVehicleCamera>(*world_->source_objects,*world_->cache->original_data());
    auto camera=camera_->prepare(*local_.camera,source_authority_binding(decision_.next));
    need(camera_->commit(std::move(camera)));
    // Original deferred departure keeps allocation/result identities and does
    // not manufacture numeric deaths for the old host's complete couch bank.
    for(const auto &p:session_->roster())if(online_admission_peer(p)==decision_.previous.host)
        world_->match.depart(p.slot);
    // Original GPU identities belong to the predecessor's renderer/PC. The
    // private incoming world uses the actual original per-request retirement
    // routine before sealing its new native adoption boundary.
    world_->source_objects->retire_lens_queries();
    need(authority.execution_tick()==decision_.value.cut.tick&&current(*session_,decision_,admissions_));
    prepared_=authority.execution_boundary();
}
SourcePeerHostTakeover::~SourcePeerHostTakeover()=default;
void SourcePeerHostTakeover::own()const{need(std::this_thread::get_id()==executor_);}
const PeerMigrationDecision &SourcePeerHostTakeover::decision()const{own();return decision_;}
MultiplayerPreparedWorld &SourcePeerHostTakeover::world(){own();return *world_;}
LanHost &SourcePeerHostTakeover::host(){own();need(active_);return *host_;}
LanHost::LocalContinuation &SourcePeerHostTakeover::local(){own();return local_;}
void SourcePeerHostTakeover::refresh_local_input(const LanClient &client){
    own();need(!active_);client.merge_local_input(input_cut_,decision_.previous,local_);
}
NativeVehicleCamera &SourcePeerHostTakeover::camera(){own();return *camera_;}
bool SourcePeerHostTakeover::activate(OnlineMatchSession::NativeCommit native_adopt,void *owner){
    own();if(active_||!current(*session_,decision_,admissions_))return false;
    SourceMatchAuthority authority(world_->match,*world_->source_objects);
    if(authority.execution_boundary()!=prepared_)return false;
    struct Adoption {SourcePeerHostTakeover *self;OnlineMatchSession::NativeCommit adopt;void *owner;};
    Adoption pending{this,native_adopt,owner};
    const auto commit=[](void *opaque)noexcept{
        auto &p=*static_cast<Adoption *>(opaque);p.self->active_=true;
        if(p.adopt&&!p.adopt(p.owner)){p.self->active_=false;return false;}return true;
    };
    const bool activated=session_->finish_peer_migration(decision_,commit,&pending);
    if(activated)input_cut_={};
    return activated;
}
}

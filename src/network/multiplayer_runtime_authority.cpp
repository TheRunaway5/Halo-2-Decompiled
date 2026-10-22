#include "halo2/multiplayer_runtime_authority.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_authority_publisher.hpp"
#include "halo2/source_authority_receiver.hpp"
#include "halo2/source_peer_host_takeover.hpp"
#include "halo2/peer_common_cut.hpp"
#include <algorithm>
#include <iostream>
#include <thread>

namespace halo2 {
namespace {
using Clock=std::chrono::steady_clock;
std::uint64_t milliseconds(){return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();}
void need(bool value){if(!value)throw FormatError("Invalid runtime authority lifetime or role transition");}
void diagnostic(std::string &previous,std::string_view prefix,const std::string &error){
    if(!error.empty()&&previous!=error){previous=error;std::cerr<<prefix<<error<<'\n';}
}
bool same_cut(const ValidatedAuthorityCut &a,const ValidatedAuthorityCut &b){
    return a.peer==b.peer&&a.context==b.context&&a.seal==b.seal;
}
bool names_cut(const PeerCommonCut &common,const ValidatedAuthorityCut &cut){
    return common.context==cut.context&&common.seal==cut.seal&&cut.peer<16&&
        (common.recipients&(1u<<cut.peer));
}
}
struct MultiplayerRuntimeAuthority::Impl {
    const std::thread::id executor=std::this_thread::get_id();
    std::shared_ptr<OnlineMatchSession> online;
    MultiplayerOptions recovery_options;
    // Keep asset references held by the renderer alive through a role change.
    std::shared_ptr<MultiplayerPreparedWorld> resident;
    // Captured from the resident constructor's private original owner, never
    // from an arbitrary replacement of its public Cache slot.
    std::shared_ptr<const Cache> resident_cache;
    std::unique_ptr<LanHost> initial_host;
    std::unique_ptr<LanClient> initial_client;
    std::unique_ptr<SourcePeerHostTakeover> active,pending;
    struct ClientRebind {
        PeerMigrationDecision decision;
        std::unique_ptr<LanClient::PreparedRebind> native;
        std::optional<MultiplayerSnapshotOrigin> origin;
        std::optional<Simulation::PreparedReplica> replica;
        MultiplayerMatch *replica_owner{};
    };
    std::optional<ClientRebind> pending_client;
    std::unique_ptr<SourceMatchAuthority> authority,pending_authority;
    std::unique_ptr<SourceAuthorityReplayWorker> receiver;
    std::unique_ptr<SourceAuthorityPublisher> publisher;
    std::shared_ptr<PeerSnapshotCandidate> snapshot_module;
    std::optional<PeerSnapshotToken> snapshot_round;
    std::optional<SourceAuthorityPublisherGroupCut> snapshot_group;
    bool snapshot_capture_started{},snapshot_common_retired{};
    PeerSnapshotClose snapshot_close=PeerSnapshotClose::Abort;
    std::optional<std::future<SourceAuthorityRetainedWorld>> recovery;
    std::shared_ptr<const SourceAuthorityRetainedWorld> retained;
    std::exception_ptr recovery_failure;
    struct CommonPair {
        std::shared_ptr<const NativeCutPin> native;
        EffectPresentationHistory::RuntimeCheckpoint foreground;
    };
    std::shared_ptr<const CommonPair> committed_pair,pending_pair;
    std::shared_ptr<const NativeCutPin> pending_native;
    std::optional<std::future<SourceAuthorityRetainedWorld>> common_export;
    std::optional<SourceAuthorityHostProof> export_proof;
    std::optional<PeerCommonCut> common_round,export_round;
    std::optional<AuthorityTransferContext> common_context;
    bool common_used{};
    std::optional<AuthorityTransferContext> published_context,client_context,shadow_context;
    std::vector<unsigned> shadow_targets;
    Clock::time_point capture_retry{};
    Clock::time_point receiver_retry{};
    std::string admission_error,receiver_error,publication_error;
    bool frame{},committing{},continuing_input{},recovery_enabled{},restart_receiver{};
    std::optional<std::uint64_t> continued_input_tick;
    std::array<Controls,4> continued_input;
    void own()const{
        need(executor==std::this_thread::get_id()&&!committing);
        need(resident&&resident_cache&&resident->cache.get()==resident_cache.get()&&
            !resident->cache.owner_before(resident_cache)&&!resident_cache.owner_before(resident->cache));
    }
    void quiescent()const{own();need(!frame);}
    void retire_obsolete_client_rebind(){
        if(!pending_client)return;
        const auto agreement=online?online->peer_migration():std::nullopt;
        if(!online||online->phase()!=OnlineMatchSession::Phase::Rebinding||
            online->authority_context()!=pending_client->decision.previous||!agreement||!agreement->decision||
            *agreement->decision!=pending_client->decision)
            pending_client.reset();
    }
    bool presentation_commit(PresentationCommit commit,void *owner) noexcept {
        // Reject runtime reentrancy before a callback can enter the session
        // mutex or observe the middle of a coupled owner publication.
        committing=true;const bool accepted=!commit||commit(owner);committing=false;
        return accepted;
    }
    MultiplayerPreparedWorld &world(){return active?active->world():*resident;}
    LanHost *host(){return active?&active->host():initial_host.get();}
    LanClient *client(){return active?nullptr:initial_client.get();}
    void clear_export(){common_export.reset();export_proof.reset();export_round.reset();}
    void retire_common(){
        // Every session getter has returned before these owning values retire.
        // No large native/foreground destruction occurs under a session lock.
        clear_export();pending_native.reset();pending_pair.reset();committed_pair.reset();common_round.reset();common_used=false;
    }
    std::shared_ptr<const CommonPair> pair(const AuthorityTransferContext &context,const AuthorityCutSeal &seal)const{
        for(const auto &value:{committed_pair,pending_pair})if(value&&value->native->cut().context==context&&
            value->native->cut().seal==seal)return value;
        return {};
    }
    void common_pins(){
        if(!online)return;
        const auto context=online->authority_context();
        if(context!=common_context){retire_common();common_context=context;}
        const auto state=online->common_cut();
        if(!state||!context||state->context!=*context||online->phase()!=OnlineMatchSession::Phase::Running||!client())return;
        if(state->pending!=common_round){
            // Protocol replacement requires this endpoint's prior retirement
            // ACK. Keep the independently committed predecessor as slot one.
            clear_export();pending_native.reset();pending_pair.reset();common_round=state->pending;
        }
        if(!state->pending)return;
        common_used=true;const auto &round=*state->pending;const auto self=online->self();
        if(self>=16||!(round.recipients&(1u<<self)))return;
        const auto bit=std::uint16_t(1u<<self);
        if(state->abort){
            if(common_export){
                // A promise future does not cancel its worker request when
                // reset. Drain/destroy the exact pending export before ACKing
                // retirement, without blocking the foreground or killing it.
                if(common_export->wait_for(std::chrono::milliseconds(0))!=std::future_status::ready)return;
                try{auto retired=common_export->get();}catch(...){}
            }
            clear_export();pending_native.reset();pending_pair.reset();
            online->acknowledge_common_cut_abort(round);return;
        }
        if(state->released&bit)return;
        if(committed_pair&&names_cut(round,committed_pair->native->cut())&&!pending_pair){
            // A local release may need retransmission/metadata retry; do not
            // reconstruct a second native value for this already rotated pin.
            if(state->release&&state->applied==round.recipients)online->release_common_cut(round);
            return;
        }
        if(state->commit&&pending_pair&&names_cut(round,pending_pair->native->cut())){
            // Applying publishes the strongly owned recovery pair only. It
            // does not restore or replace any live native/presentation owner.
            if(state->associated==round.recipients&&!(state->applied&bit))online->apply_common_cut(round);
            if(state->release&&state->applied==round.recipients){
                auto previous=std::move(committed_pair);
                committed_pair=std::move(pending_pair);pending_native.reset();clear_export();
                previous.reset(); // retire before acknowledging, outside the session lock
                online->release_common_cut(round);
            }
            return;
        }
        if(pending_pair||pending_native||!receiver)return;
        if(common_export){
            if(common_export->wait_for(std::chrono::milliseconds(0))!=std::future_status::ready)return;
            try{
                auto value=std::make_shared<const SourceAuthorityRetainedWorld>(common_export->get());
                need(export_round&&*export_round==round&&export_proof&&value->host&&names_cut(round,value->cut)&&
                    value->cut.peer==self&&same_cut(export_proof->cut(),value->cut));
                pending_native=std::shared_ptr<const NativeCutPin>(new NativeCutPin(*export_proof,std::move(value)));
                clear_export();return;
            }catch(const std::exception &error){
                clear_export();diagnostic(receiver_error,"Source common-cut export unavailable: ",error.what());
            }
        }
        // Readiness and proof are one worker-state copy. A stale asynchronous
        // export is retried at the exact offered cut, never relabeled later.
        const auto shadow=receiver->state();
        if(!state->commit&&!(state->associated&bit)&&
            (receiver->stopped()||(shadow.rejected&&names_cut(round,*shadow.rejected)&&shadow.rejected->peer==self)||
             (shadow.ready&&shadow.ready->context==round.context&&shadow.ready->peer==self&&
              (shadow.ready->seal.id>round.seal.id||shadow.ready->seal.tick>round.seal.tick)))){
            // This worker cannot produce the offered native value. Reject only
            // that exact unassociated round; keep the committed predecessor.
            online->reject_common_cut(round);return;
        }
        if(!shadow.ready||!shadow.host_ready||!shadow.host_proof||!names_cut(round,*shadow.ready)||
            shadow.ready->peer!=self||!same_cut(shadow.host_proof->cut(),*shadow.ready))return;
        export_proof=*shadow.host_proof;export_round=round;
        try{common_export=receiver->checkpoint_paired(*shadow.ready);}
        catch(...){clear_export();throw;}
    }
    void common_publication(const SourceAuthorityPublisherState &state){
        if(!state.held_group)return;
        const auto &held=*state.held_group;
        const PeerCommonCut cut{held.context,held.seal,held.recipients};
        auto common=online->common_cut();if(!common||common->context!=held.context)return;
        // Offer immediately after issuance, before native ACK completion, so
        // each foreground can reserve exact history candidates in time.
        if(common->pending!=cut){online->offer_common_cut(cut);common=online->common_cut();}
        if(!common||common->pending!=cut)return;
        common_used=true;
        bool candidate_abort{};
        if(snapshot_module&&snapshot_round&&snapshot_group==held){
            const auto candidate=snapshot_module->state();
            candidate_abort=candidate.pending==snapshot_round&&candidate.closing==PeerSnapshotClose::Abort;
        }
        if((state.validation_timed_out||candidate_abort)&&!common->commit)online->abort_common_cut(cut);
        else if(state.group_ready==held&&!common->abort)online->confirm_common_cut_native(cut);
        if(common->abort&&common->aborted==cut.recipients){
            if(publisher->abort_common_cut(held)&&snapshot_group==held){snapshot_common_retired=true;snapshot_close=PeerSnapshotClose::Abort;}
        }else if(common->release&&common->released==cut.recipients){
            if(publisher->release_common_cut(held)&&snapshot_group==held){snapshot_common_retired=true;snapshot_close=PeerSnapshotClose::Release;}
        }
    }
    void reset_snapshot_round(){
        // Context retirement is independent of normal all-recipient rotation.
        // Leave the old module's owning foreground holders to retire their
        // actual generation; no ACK or membership shrink is invented here.
        if(snapshot_module&&snapshot_round)snapshot_module->close(*snapshot_round,PeerSnapshotClose::Abort);
        snapshot_module.reset();snapshot_round.reset();snapshot_group.reset();
        snapshot_capture_started=false;snapshot_common_retired=false;
    }
    void candidate_publication(const SourceAuthorityPublisherState &state){
        if(!online->snapshot_candidates_enabled())return;
        const auto module=online->snapshot_candidates();
        if(snapshot_module&&snapshot_module!=module){
            // Temporary loss of negotiation must not replace an in-flight
            // fixed-generation owner or silently release its old pin.
            if(online->authority_context()==snapshot_module->state().context)return;
            reset_snapshot_round();
        }
        if(!module)return;
        if(!snapshot_module)snapshot_module=module;
        if(snapshot_round){
            const auto candidate=module->state();
            if(candidate.pending!=snapshot_round)return;
            if(snapshot_common_retired){
                // An already latched recipient Abort remains Abort, even if
                // the independently committed common pair finished Release.
                module->close(*snapshot_round,candidate.closing.value_or(snapshot_close));
            }else if(!snapshot_group&&snapshot_capture_started&&
                (!state.encoding&&!state.awaiting_validation&&!state.held_group)){
                publisher->abort_snapshot_candidate(*snapshot_round);
                module->close(*snapshot_round,PeerSnapshotClose::Abort);
            }else if(!snapshot_group&&candidate.closing==PeerSnapshotClose::Abort){
                publisher->abort_snapshot_candidate(*snapshot_round);
            }
            const auto retired=module->state();
            if(!retired.closing||retired.retired!=snapshot_round->recipients||
                (snapshot_group&&!snapshot_common_retired))return;
            snapshot_round.reset();snapshot_group.reset();snapshot_capture_started=false;snapshot_common_retired=false;
        }
        // Every new arm is created between actual frames, after both the
        // native/common and provisional foreground owners retired the last.
        if(!state.held_group&&!state.encoding&&!state.awaiting_validation&&!state.snapshot_candidate&&Clock::now()>=capture_retry){
            snapshot_round=module->begin(milliseconds());
            if(snapshot_round)capture_retry=Clock::now()+std::chrono::seconds(3);
        }
    }
    void publish(){
        if(!authority)return;
        try{
            const auto context=online->authority_context();
            if(!context||online->phase()!=OnlineMatchSession::Phase::Running||context->host!=online->self()){
                reset_snapshot_round();publisher.reset();shadow_targets.clear();shadow_context.reset();return;
            }
            // Every surviving physical PC needs its own independently validated
            // paired proof, including PCs ineligible for hosting. QoS still ranks
            // candidates in the agreement; it must not filter proof recipients.
            std::vector<unsigned> selected;
            for(unsigned peer=0;peer<16;++peer)if(peer!=online->self()&&(context->peers&(1u<<peer)))selected.push_back(peer);
            if(selected!=shadow_targets||shadow_context!=context||(!publisher&&!selected.empty())){
                std::unique_ptr<SourceAuthorityPublisher> next;
                if(!selected.empty())next=std::make_unique<SourceAuthorityPublisher>(*authority,*host(),online,std::span<const unsigned>(selected));
                if(next&&online->snapshot_candidates_enabled()){
                    next->set_common_cut_barrier(true);next->set_snapshot_candidate_mode(true);
                }
                // Commit the cached selection only after construction succeeds,
                // so allocation/thread failure remains retryable on service().
                reset_snapshot_round();publisher=std::move(next);shadow_targets=std::move(selected);shadow_context=context;capture_retry={};
            }
            if(!publisher)return;
            // Feature negotiation may complete after construction. Restart an
            // unheld legacy stream once before opting into the common barrier.
            if(online->common_cut()&&!publisher->state().common_cut_barrier){
                publisher->cancel();publisher->set_common_cut_barrier(true);capture_retry={};
            }
            publisher->update();auto state=publisher->state();
            if(state.snapshot_candidate&&state.snapshot_candidate_group&&snapshot_round==state.snapshot_candidate){
                // This is copied only from the actual result published by the
                // exact candidate capture, never from a newest seal guess.
                snapshot_group=state.snapshot_candidate_group;
            }
            common_publication(state);state=publisher->state();
            if(online->snapshot_candidates_enabled())candidate_publication(state);
            else if(!state.held_group&&!state.tracking_replay&&!state.encoding&&!state.awaiting_validation&&Clock::now()>=capture_retry){
                capture_retry=Clock::now()+std::chrono::seconds(3);publisher->capture();state=publisher->state();
            }
            diagnostic(publication_error,"Source shadow publication unavailable: ",state.error);
        }catch(const std::exception &e){
            if(publisher&&!publisher->state().held_group)publisher->cancel();
            capture_retry=Clock::now()+std::chrono::seconds(3);
            diagnostic(publication_error,"Source shadow publication unavailable: ",e.what());
        }
    }
};
MultiplayerRuntimeAuthority::MultiplayerRuntimeAuthority(const MultiplayerOptions &options,
    std::shared_ptr<MultiplayerPreparedWorld> world,std::unique_ptr<LanHost> host,std::unique_ptr<LanClient> client,
    std::unique_ptr<SourceAuthorityReplayWorker> receiver):impl_(std::make_unique<Impl>()){
    auto &s=*impl_;need(world&&!(host&&client));
    s.resident_cache=world->original_cache();need(world->matches(options));
    s.online=options.online_match;s.recovery_options=source_authority_options(options);s.resident=std::move(world);
    s.initial_host=std::move(host);s.initial_client=std::move(client);s.receiver=std::move(receiver);
    s.recovery_enabled=bool(s.receiver);
    need(!s.receiver||(s.online&&s.initial_client));
    if(s.online&&s.initial_host&&s.world().source_objects&&s.world().source_objects->owns_physics())
        s.authority=std::make_unique<SourceMatchAuthority>(s.world().match,*s.world().source_objects);
}
MultiplayerRuntimeAuthority::~MultiplayerRuntimeAuthority(){
    // Native objects and publisher frames must retire on their executor.
    if(impl_->executor!=std::this_thread::get_id()||impl_->frame||impl_->committing)std::terminate();
}
MultiplayerPreparedWorld &MultiplayerRuntimeAuthority::world(){impl_->own();return impl_->world();}
MultiplayerMatch &MultiplayerRuntimeAuthority::match(){return world().match;}
LanHost *MultiplayerRuntimeAuthority::host(){impl_->own();return impl_->host();}
LanClient *MultiplayerRuntimeAuthority::client(){impl_->own();return impl_->client();}
void MultiplayerRuntimeAuthority::admit_players(){
    auto &s=*impl_;s.own();if(!s.authority||s.online->phase()!=OnlineMatchSession::Phase::Running)return;
    const auto context=s.online->authority_context();
    if(!context||context->host!=s.online->self()||s.published_context==context)return;
    std::vector<unsigned> slots;
    for(const auto &p:s.online->roster()){
        if(!s.world().match.player(p.slot).connected)return;
        slots.push_back(p.slot);
    }
    try{const auto players=s.authority->player_admissions(slots);
        if(s.online->publish_source_players(*context,players))s.published_context=context;
    }catch(const std::exception &e){diagnostic(s.admission_error,"Source player admission unavailable: ",e.what());}
}
void MultiplayerRuntimeAuthority::service(){
    auto &s=*impl_;s.quiescent();s.retire_obsolete_client_rebind();admit_players();s.publish();receive();s.common_pins();
}
void MultiplayerRuntimeAuthority::receive(){
    auto &s=*impl_;s.own();
    if(s.restart_receiver&&s.online&&s.client()&&s.online->phase()==OnlineMatchSession::Phase::Running){
        if(s.receiver&&s.receiver->stopped())s.receiver.reset();
        if(!s.receiver&&Clock::now()>=s.receiver_retry)try{
            s.receiver=std::make_unique<SourceAuthorityReplayWorker>(s.recovery_options);s.restart_receiver=false;
        }catch(const std::exception &error){
            s.receiver_retry=Clock::now()+std::chrono::seconds(1);
            diagnostic(s.receiver_error,"Source shadow restart unavailable: ",error.what());
        }
    }
    if(s.online&&s.client()&&s.client()->ready()&&!s.client_context&&s.online->direct_udp()){
        if(const auto context=s.online->authority_context())try{
            s.client()->bind_continuity(*context,s.online->self(),s.online->roster());s.client_context=context;
        }catch(const std::exception &e){diagnostic(s.receiver_error,"Source client continuity unavailable: ",e.what());}
    }
    if(s.receiver)diagnostic(s.receiver_error,"Source shadow unavailable: ",s.receiver->state().error);
}
void MultiplayerRuntimeAuthority::suspend_publication(){auto &s=*impl_;s.quiescent();if(s.publisher)s.publisher->cancel();}
struct MultiplayerRuntimeAuthority::Frame::Impl {
    MultiplayerRuntimeAuthority::Impl *owner;
    std::unique_ptr<SourceAuthorityPublisher::Frame> recording;
};
MultiplayerRuntimeAuthority::Frame::Frame(MultiplayerRuntimeAuthority &owner):impl_(std::make_unique<Impl>()){
    auto &s=*owner.impl_;s.quiescent();
    impl_->owner=&s;impl_->recording=s.publisher?s.publisher->record_frame():nullptr;s.frame=true;
}
bool MultiplayerRuntimeAuthority::Frame::finish()noexcept{
    if(!impl_->owner)return true;
    auto &s=*impl_->owner;if(s.executor!=std::this_thread::get_id()||!s.frame)std::terminate();
    const bool recorded=!impl_->recording||impl_->recording->finish();impl_->recording.reset();s.frame=false;impl_->owner=nullptr;return recorded;
}
MultiplayerRuntimeAuthority::Frame::~Frame(){finish();}
std::unique_ptr<MultiplayerRuntimeAuthority::Frame> MultiplayerRuntimeAuthority::begin_frame(){
    auto &s=*impl_;s.quiescent();s.retire_obsolete_client_rebind();
    if(s.online){const auto phase=s.online->phase();
        if(phase==OnlineMatchSession::Phase::AwaitingWorld||phase==OnlineMatchSession::Phase::Migrating||
            phase==OnlineMatchSession::Phase::Rebinding||phase==OnlineMatchSession::Phase::Failed)return {};}
    return std::unique_ptr<Frame>(new Frame(*this));
}
void MultiplayerRuntimeAuthority::broadcast_candidate_frame(std::unique_ptr<Frame> &frame){
    auto &s=*impl_;s.own();need(frame&&frame->impl_->owner==&s&&s.frame&&s.host());
    if(!s.online||!s.online->snapshot_candidates_enabled()||!s.publisher||!s.snapshot_module||
        !s.snapshot_round||s.snapshot_capture_started){s.host()->broadcast();return;}
    const auto candidate=s.snapshot_module->state();const auto publication=s.publisher->state();
    if(candidate.pending!=s.snapshot_round||candidate.closing||candidate.published||
        candidate.armed!=s.snapshot_round->recipients||!publication.snapshot_candidate_mode||
        publication.held_group||publication.encoding||publication.awaiting_validation||publication.snapshot_candidate){
        s.host()->broadcast();return;
    }
    const auto token=*s.snapshot_round;
    std::optional<LanHost::CandidateEmission> emission;
    try{
        auto attempt=s.host()->broadcast_candidate(s.snapshot_module,token);
        if(!attempt.emitted)s.host()->broadcast();
        emission=std::move(attempt.candidate);
    }
    catch(...){
        // Preserve original native broadcast exceptions; a recovery candidate
        // never converts failed real work into a successful gameplay frame.
        frame->finish();frame.reset();s.snapshot_module->close(token,PeerSnapshotClose::Abort);throw;
    }
    const bool finished=frame->finish();frame.reset();s.snapshot_capture_started=true;
    bool captured{};
    try{
        if(finished&&emission&&emission->token()==token&&s.host()->candidate_current(*emission,s.snapshot_module))
            captured=s.publisher->capture_candidate(token,*emission);
        if(!captured)s.snapshot_module->close(token,PeerSnapshotClose::Abort);
    }catch(const std::exception &error){
        s.snapshot_module->close(token,PeerSnapshotClose::Abort);
        diagnostic(s.publication_error,"Source snapshot candidate unavailable: ",error.what());
    }catch(...){
        s.snapshot_module->close(token,PeerSnapshotClose::Abort);frame=begin_frame();throw;
    }
    // No native call occurs between the selected broadcast and the capture.
    // Even a rejected capture returns the caller to an ordinary live frame.
    frame=begin_frame();
}
std::optional<MultiplayerRuntimeAuthority::SnapshotCandidateCut> MultiplayerRuntimeAuthority::snapshot_candidate_cut()const{
    const auto &s=*impl_;s.own();
    if(!s.snapshot_round||!s.snapshot_group)return {};
    return SnapshotCandidateCut{*s.snapshot_round,s.snapshot_group->seal};
}
bool MultiplayerRuntimeAuthority::has_receiver()const{
    impl_->own();return bool(impl_->receiver)||bool(impl_->retained)||bool(impl_->committed_pair)||bool(impl_->pending_pair);
}
std::optional<SourceAuthorityReceiverState> MultiplayerRuntimeAuthority::shadow_state()const{
    impl_->own();return impl_->receiver?std::optional(impl_->receiver->state()):std::nullopt;
}
MultiplayerRuntimeAuthority::NativeCutPin::NativeCutPin(SourceAuthorityHostProof proof,
    std::shared_ptr<const SourceAuthorityRetainedWorld> retained):proof_(std::move(proof)),retained_(std::move(retained)){}
const ValidatedAuthorityCut &MultiplayerRuntimeAuthority::NativeCutPin::cut()const{return retained_->cut;}
const SourceAuthorityHostProof &MultiplayerRuntimeAuthority::NativeCutPin::host_proof()const{return proof_;}
const SourceAuthorityRetainedWorld &MultiplayerRuntimeAuthority::NativeCutPin::retained_world()const{return *retained_;}
std::shared_ptr<const MultiplayerRuntimeAuthority::NativeCutPin> MultiplayerRuntimeAuthority::pending_native_cut()const{
    impl_->own();return impl_->pending_native;
}
bool MultiplayerRuntimeAuthority::associate_common_cut(const std::shared_ptr<const NativeCutPin> &pin,
    const EffectPresentationHistory::RuntimeCheckpoint &foreground){
    auto &s=*impl_;s.quiescent();
    if(!pin||pin!=s.pending_native||!s.online||!s.client()||s.online->phase()!=OnlineMatchSession::Phase::Running)return false;
    const auto state=s.online->common_cut();
    if(!state||!state->pending||state->abort||!s.common_round||*s.common_round!=*state->pending||
        !names_cut(*state->pending,pin->cut())||pin->cut().peer!=s.online->self())return false;
    const auto &origin=foreground.origin();const auto &replica=foreground.frontier().replica;
    if(!same_cut(foreground.cut(),pin->cut())||!same_cut(foreground.proof().cut(),pin->cut())||
        !foreground.proof().matches(origin)||!replica.matches(origin)||!s.world().match.owns_replica_checkpoint(replica))return false;
    const auto independent=pin->host_proof().prove_snapshot(*s.client(),origin);
    if(!same_cut(independent.cut(),pin->cut())||!independent.matches(origin))return false;
    if(s.pending_pair){
        if(s.pending_pair->native!=pin||!s.pending_pair->foreground.origin().same(origin))return false;
    }else s.pending_pair=std::make_shared<const Impl::CommonPair>(Impl::CommonPair{pin,foreground});
    // Store both strong owners before entering the session's metadata lock.
    // Rejection/Abort is serviced later without losing its retirement promise.
    return s.online->associate_common_cut(*state->pending);
}
std::optional<EffectPresentationHistory::RuntimeCheckpoint> MultiplayerRuntimeAuthority::common_cut_checkpoint(
    const AuthorityTransferContext &context,const AuthorityCutSeal &seal)const{
    auto &s=*impl_;s.own();if(auto pair=s.pair(context,seal))return pair->foreground;return {};
}
std::shared_ptr<const SourceAuthorityRetainedWorld> MultiplayerRuntimeAuthority::recover(){
    auto &s=*impl_;s.quiescent();
    const auto migration=s.online?s.online->peer_migration():std::nullopt;
    const auto common=s.online?s.online->common_cut():std::nullopt;
    if(s.common_used||s.committed_pair||s.pending_pair||(common&&(common->pending||common->committed))){
        // Once a real common round exists, a later worker drain cannot stand
        // in for the independently learned cut. Retire its executor only after
        // selecting our exact owning native+foreground pair.
        if(!migration||!migration->decision)return {};
        const auto &decision=*migration->decision;const auto pair=s.pair(decision.previous,decision.value.cut);
        if(!pair)return {};
        s.retained=std::shared_ptr<const SourceAuthorityRetainedWorld>(pair->native,&pair->native->retained_world());s.recovery.reset();
        if(s.receiver){s.receiver->cancel();if(!s.receiver->stopped())return {};s.receiver.reset();}
        return s.retained;
    }
    // Compatibility for manual fixtures/legacy streams that never offered a
    // common round. It does not admit a newer cut after common pinning begins.
    if(s.recovery_failure)std::rethrow_exception(s.recovery_failure);
    if(!s.retained){
        if(!s.receiver)return {};
        if(!s.recovery)s.recovery=s.receiver->retain_validated_world();
        if(s.recovery->wait_for(std::chrono::milliseconds(0))!=std::future_status::ready)return {};
        try{s.retained=std::make_shared<const SourceAuthorityRetainedWorld>(s.recovery->get());}
        catch(...){s.recovery_failure=std::current_exception();s.recovery.reset();s.receiver->cancel();throw;}
        s.recovery.reset();s.receiver->cancel();
    }
    // A completed export does not imply completed native-world destruction.
    // Keep OS/input events moving until the executor acknowledges retirement.
    if(s.receiver){if(!s.receiver->stopped())return {};s.receiver.reset();}
    return s.retained;
}
void MultiplayerRuntimeAuthority::cancel_recovery(){auto &s=*impl_;s.quiescent();if(s.receiver)s.receiver->cancel();}
void MultiplayerRuntimeAuthority::stage_takeover(std::unique_ptr<SourcePeerHostTakeover> &&candidate){
    auto &s=*impl_;s.quiescent();need(candidate&&s.online&&!s.host()&&!s.pending&&!s.pending_client&&!s.receiver);
    const auto state=s.online->peer_migration();
    need(state&&state->decision&&*state->decision==candidate->decision()&&s.online->self()==candidate->decision().value.host);
    auto &world=candidate->world();need(world.source_objects&&world.source_objects->owns_physics()&&
        world.cache->path()==s.resident->cache->path()&&world.cache->shared_directory()==s.resident->cache->shared_directory());
    auto authority=std::make_unique<SourceMatchAuthority>(world.match,*world.source_objects);
    s.pending_authority=std::move(authority);s.pending=std::move(candidate);
}
SourcePeerHostTakeover *MultiplayerRuntimeAuthority::pending_takeover(){impl_->own();return impl_->pending.get();}
bool MultiplayerRuntimeAuthority::prepare_takeover(std::shared_ptr<PhysicsLiveClockSource> clock){
    auto &s=*impl_;s.quiescent();s.retire_obsolete_client_rebind();if(s.pending)return true;
    if(!s.online||s.host()||!s.client()||s.online->phase()!=OnlineMatchSession::Phase::Migrating)return false;
    const auto agreement=s.online->peer_migration();
    if(!agreement||!agreement->decision||agreement->decision->value.host!=s.online->self())return false;
    const auto retained=recover();if(!retained)return false;
    auto candidate=SourcePeerHostTakeover::prepare_with_resident_cache(s.recovery_options,*retained,
        *agreement->decision,*s.resident,std::move(clock));
    stage_takeover(std::move(candidate));return true;
}
bool MultiplayerRuntimeAuthority::activate_takeover(PresentationCommit presentation,void *owner){
    auto &s=*impl_;s.quiescent();if(!s.pending)return false;
    need(!s.active&&!s.authority&&!s.initial_host&&!s.publisher&&!s.pending_client&&s.pending_authority);
    receive();need(bool(s.initial_client));s.pending->refresh_local_input(*s.initial_client);
    struct Adoption {Impl &state;PresentationCommit presentation;void *owner;std::unique_ptr<LanClient> retired_client;};
    Adoption adoption{s,presentation,owner,{}};
    const auto commit=[](void *opaque)noexcept{
        auto &a=*static_cast<Adoption *>(opaque);auto &state=a.state;
        if(!state.presentation_commit(a.presentation,a.owner))return false;
        // Only move into empty owners under the session lock. Destruction of
        // the old client can send a leave and must happen outside that lock.
        a.retired_client=std::move(state.initial_client);
        state.authority=std::move(state.pending_authority);state.active=std::move(state.pending);
        state.continuing_input=true;state.continued_input_tick.reset();
        state.published_context.reset();state.client_context.reset();state.shadow_context.reset();state.shadow_targets.clear();state.capture_retry={};
        return true;
    };
    auto *candidate=s.pending.get();if(!candidate->activate(commit,&adoption))return false;
    s.retained.reset();s.recovery.reset();
    return true;
}
bool MultiplayerRuntimeAuthority::prepare_client_rebind(){
    return prepare_client_rebind_from(nullptr,nullptr,nullptr);
}
bool MultiplayerRuntimeAuthority::prepare_client_rebind(const MultiplayerSnapshotProof &proof,
    const MultiplayerSnapshotOrigin &origin,const Simulation::ReplicaCheckpoint &replica){
    return prepare_client_rebind_from(&proof,&origin,&replica);
}
bool MultiplayerRuntimeAuthority::prepare_client_rebind_from(const MultiplayerSnapshotProof *proof,
    const MultiplayerSnapshotOrigin *origin,const Simulation::ReplicaCheckpoint *replica){
    auto &s=*impl_;s.quiescent();s.retire_obsolete_client_rebind();
    need(bool(proof)==bool(origin)&&bool(proof)==bool(replica));
    if(proof)need(proof->matches(*origin)&&replica->matches(*origin)&&s.world().match.owns_replica_checkpoint(*replica));
    if(s.pending_client){
        if(proof)need(s.pending_client->origin&&s.pending_client->origin->same(*origin)&&
            proof->cut().context==s.pending_client->decision.previous&&proof->cut().seal==s.pending_client->decision.value.cut&&
            s.pending_client->replica_owner==&s.world().match&&s.pending_client->replica&&
            s.world().match.replica_ready(*s.pending_client->replica));
        return true;
    }
    if(!s.online||s.host()||!s.client()||s.pending||s.online->phase()!=OnlineMatchSession::Phase::Rebinding)return false;
    receive();const auto agreement=s.online->peer_migration();
    if(!agreement||!agreement->decision||agreement->decision->value.host==s.online->self())return false;
    const auto &decision=*agreement->decision;need(s.client_context==decision.previous);
    auto prepared=proof?s.client()->prepare_rebind_host(decision.previous,decision.next,decision.value.cut,*proof):
        s.client()->prepare_rebind_host(decision.previous,decision.next,decision.value.cut);
    std::optional<Simulation::PreparedReplica> restored;
    if(replica)restored.emplace(s.world().match.prepare_replica(*replica));
    s.pending_client=Impl::ClientRebind{decision,std::move(prepared),origin?std::optional{*origin}:std::nullopt,
        std::move(restored),replica?&s.world().match:nullptr};return true;
}
bool MultiplayerRuntimeAuthority::cancel_client_rebind(){
    auto &s=*impl_;s.quiescent();const bool pending=bool(s.pending_client);s.pending_client.reset();return pending;
}
std::optional<PeerMigrationDecision> MultiplayerRuntimeAuthority::pending_client_rebind()const{
    auto &s=*impl_;s.own();return s.pending_client?std::optional(s.pending_client->decision):std::nullopt;
}
bool MultiplayerRuntimeAuthority::activate_client_rebind(PresentationCommit presentation,void *owner){
    auto &s=*impl_;s.quiescent();s.retire_obsolete_client_rebind();if(!s.pending_client)return false;
    const auto replica_ready=[&]{return !s.pending_client->replica ||
        (s.pending_client->replica_owner==&s.world().match&&s.world().match.replica_ready(*s.pending_client->replica));};
    need(s.initial_client&&s.initial_client->can_commit_rebind_host(*s.pending_client->native)&&replica_ready());
    const auto decision=s.pending_client->decision;
    struct Adoption {Impl &state;PresentationCommit presentation;void *owner;};
    Adoption adoption{s,presentation,owner};
    const auto commit=[](void *opaque)noexcept{
        auto &a=*static_cast<Adoption *>(opaque);auto &state=a.state;
        // No executor/transport mutation can intervene after this preflight:
        // the presentation callback publishes only its prepared local owners.
        if(!state.initial_client->can_commit_rebind_host(*state.pending_client->native)||
            (state.pending_client->replica&&(!state.pending_client->replica_owner||
                !state.pending_client->replica_owner->replica_ready(*state.pending_client->replica)))||
            !state.presentation_commit(a.presentation,a.owner))return false;
        if(state.pending_client->replica&&
            !state.pending_client->replica_owner->commit_replica(*state.pending_client->replica))std::terminate();
        if(!state.initial_client->commit_rebind_host(std::move(state.pending_client->native)))std::terminate();
        return true;
    };
    if(!s.online->finish_peer_migration(decision,commit,&adoption))return false;
    s.client_context=decision.next;
    const bool retired=bool(s.recovery)||bool(s.retained)||bool(s.recovery_failure);
    if(retired&&s.receiver)s.receiver->cancel();
    s.restart_receiver=s.recovery_enabled&&retired;s.receiver_retry={};
    s.recovery.reset();s.retained.reset();s.recovery_failure={};s.pending_client.reset();
    return true;
}
NativeVehicleCamera *MultiplayerRuntimeAuthority::promoted_camera(){
    auto &s=*impl_;s.own();return s.active?&s.active->camera():nullptr;
}
bool MultiplayerRuntimeAuthority::local_input_pending()const{
    auto &s=*impl_;s.own();return s.continuing_input||(s.active&&s.continued_input_tick==s.active->world().match.simulation().ticks());
}
bool MultiplayerRuntimeAuthority::continue_local_input(std::array<Controls,multiplayer_capacity> &input){
    auto &s=*impl_;s.own();need(s.frame);
    if(s.active&&s.continued_input_tick==s.active->world().match.simulation().ticks()){
        const auto &slots=s.active->local().slots;
        for(unsigned view=0;view<slots.size();++view)input[slots[view]]=s.continued_input[view];
        return true;
    }
    if(!s.continuing_input)return false;
    need(bool(s.active));auto &saved=s.active->local();LanHost::receive_local_continuation(saved,input);
    s.continued_input_tick=s.active->world().match.simulation().ticks();
    for(unsigned view=0;view<saved.slots.size();++view)s.continued_input[view]=input[saved.slots[view]];
    s.continuing_input=false;
    for(unsigned view=0;view<saved.slots.size();++view)
        for(unsigned action=0;action<saved.received_presses[view].size();++action)
            s.continuing_input|=saved.received_presses[view][action]!=saved.delivered_presses[view][action];
    return true;
}
}

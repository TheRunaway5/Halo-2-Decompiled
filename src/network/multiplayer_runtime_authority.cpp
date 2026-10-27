#include "halo2/multiplayer_runtime_authority.hpp"
#include "halo2/system_link_state.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_authority_publisher.hpp"
#include "halo2/source_authority_receiver.hpp"
#include "halo2/source_peer_host_takeover.hpp"
#include "halo2/peer_common_cut.hpp"
#include "halo2/source_admitted_camera.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
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
bool aborted(const std::optional<PeerCommonCutState> &state,const AuthorityTransferContext &context,const AuthorityCutSeal &cut){
    return state&&state->abort&&state->pending&&state->pending->context==context&&state->pending->seal==cut;
}
bool aborted(const std::optional<PeerCommonCutState> &state,const PeerMigrationDecision &decision){
    return aborted(state,decision.previous,decision.value.cut);
}
}
struct MultiplayerRuntimeAuthority::CommonPair {
    std::shared_ptr<const NativeCutPin> native;
    EffectPresentationHistory::RuntimeCheckpoint foreground;
};
struct MultiplayerRuntimeAuthority::CommonInventoryLease::State {
    const std::thread::id executor=std::this_thread::get_id();
    const unsigned self;
    const AuthorityTransferContext context;
    const PeerSurvivorInventory inventory;
    const std::array<std::shared_ptr<const CommonPair>,3> pairs;
    bool eligible=true;
    State(unsigned self,AuthorityTransferContext context,PeerSurvivorInventory inventory,
        std::array<std::shared_ptr<const CommonPair>,3> pairs)
        :self(self),context(std::move(context)),inventory(std::move(inventory)),pairs(std::move(pairs)){}
};
struct MultiplayerRuntimeAuthority::Impl {
    using CommonPair=MultiplayerRuntimeAuthority::CommonPair;
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
        std::shared_ptr<const CommonPair> pair;
    };
    std::optional<ClientRebind> pending_client;
    struct CommonClient {
        PeerProspectiveClientRole role;
        std::shared_ptr<const CommonPair> pair;
        std::unique_ptr<LanClient::PreparedRebind> native;
        std::optional<Simulation::PreparedReplica> replica;
        MultiplayerMatch *replica_owner{};
    };
    std::optional<CommonClient> common_client;
    std::unique_ptr<SourceMatchAuthority> authority,pending_authority;
    std::unique_ptr<SourceAuthorityReplayWorker> receiver;
    std::unique_ptr<SourceAuthorityPublisher> publisher;
    std::shared_ptr<SourceAdmittedCameraBank> admitted_cameras;
    std::shared_ptr<PeerSnapshotCandidate> snapshot_module;
    std::optional<PeerSnapshotToken> snapshot_round;
    std::optional<SourceAuthorityPublisherGroupCut> snapshot_group;
    bool snapshot_capture_started{},snapshot_common_retired{};
    PeerSnapshotClose snapshot_close=PeerSnapshotClose::Abort;
    std::optional<std::future<SourceAuthorityRetainedWorld>> recovery;
    std::shared_ptr<const SourceAuthorityRetainedWorld> retained;
    std::exception_ptr recovery_failure;
    std::shared_ptr<const CommonPair> committed_pair,pending_pair;
    // One explicitly scheduled reconstruction, retaining the whole selected
    // pair beyond history/worker/common-slot advancement. No mutable target
    // or native borrower leaves this executor-private opaque owner.
    std::shared_ptr<const CommonPair> imported_pair;
    std::shared_ptr<CommonInventoryLease::State> inventory_lease;
    // An opaque Session-minted retirement reservation, not another native
    // scheduling state machine. Keep it through every deferred outer gate.
    std::optional<OnlineMatchSession::MigrationPreparationRequest> preparation_retirement;
    std::shared_ptr<SourcePeerHostTakeover::NativeImport> native_import;
    std::shared_ptr<const NativeCutPin> pending_native;
    std::shared_ptr<const SourceAuthorityEndpoint> terminal_displayed;
    std::optional<std::future<SourceAuthorityRetainedWorld>> common_export;
    std::optional<SourceAuthorityHostProof> export_proof;
    std::optional<PeerCommonCut> common_round,export_round;
    std::optional<AuthorityTransferContext> common_context;
    bool common_used{};
    std::optional<AuthorityTransferContext> published_context,client_context,shadow_context;
    std::vector<unsigned> shadow_targets;
    Clock::time_point capture_retry{};
    Clock::time_point receiver_retry{};
    std::string admission_error,receiver_error,terminal_error,publication_error,preparation_error;
    bool frame{},committing{},continuing_input{},recovery_enabled{},restart_receiver{},recovery_preparation{};
    std::optional<std::uint64_t> continued_input_tick;
    std::array<Controls,4> continued_input;
    OuterRetirementGate retirement_gate{};
    void *retirement_owner{};
    const OuterRetirementGuard *retirement_guard{};
    void own()const{
        need(executor==std::this_thread::get_id()&&!committing);
        need(resident&&resident_cache&&resident->cache.get()==resident_cache.get()&&
            !resident->cache.owner_before(resident_cache)&&!resident_cache.owner_before(resident->cache));
    }
    void quiescent()const{own();need(!frame);}
    bool preparation_retiring()const{
        // The Session owns the real Rust stage, including retirement scheduled
        // by direct Session completion before this executor adopts its request.
        return preparation_retirement||(online&&online->migration_preparation_retiring());
    }
    bool retire_outer(const AuthorityTransferContext &context,const AuthorityCutSeal &seal,OuterRetirement::Reason reason)noexcept{
        if(!retirement_gate)return true;
        const OuterRetirement retirement{context,seal,reason};
        committing=true;const bool retired=retirement_gate(retirement_owner,retirement);committing=false;return retired;
    }
    static const PeerMigrationDecision &staged_decision(const SourcePeerHostTakeover &candidate){
        return candidate.has_prospective()?candidate.prospective_decision():candidate.decision();
    }
    bool retire_obsolete_client_rebind(){
        if(!pending_client)return true;
        const auto agreement=online?online->peer_migration():std::nullopt;
        if(!online||online->phase()!=OnlineMatchSession::Phase::Rebinding||
            online->authority_context()!=pending_client->decision.previous||!agreement||!agreement->decision||
            *agreement->decision!=pending_client->decision||aborted(online->common_cut(),pending_client->decision)){
            const auto decision=pending_client->decision;
            if(!retire_outer(decision.previous,decision.value.cut,OuterRetirement::Reason::ClientRebindRetired))return false;
            pending_client.reset();
        }
        return true;
    }
    bool common_client_ready()const{
        if(!common_client||!online)return false;
        const auto phase=online->phase();
        if(phase!=OnlineMatchSession::Phase::Running&&phase!=OnlineMatchSession::Phase::AwaitingWorld&&
            phase!=OnlineMatchSession::Phase::Migrating&&phase!=OnlineMatchSession::Phase::Rebinding)return false;
        return online->authority_context()==common_client->role.previous&&
            online->self()==common_client->role.self&&initial_client&&common_client->native&&common_client->pair&&
            online->source_players()==common_client->pair->native->retained_world().admissions&&
            !aborted(online->common_cut(),common_client->role.previous,common_client->role.cut)&&
            common_client->replica_owner==&resident->match&&common_client->replica&&
            resident->match.replica_ready(*common_client->replica)&&
            initial_client->prospective_rebind_ready(*common_client->native,common_client->role);
    }
    bool retire_common_client_for_host(const PeerMigrationDecision &decision){
        if(!common_client)return true;
        const auto learned=online?online->peer_migration():std::nullopt;
        if(!learned||!learned->decision||*learned->decision!=decision||decision.value.host!=online->self()||
            online->phase()!=OnlineMatchSession::Phase::Migrating||online->authority_context()!=decision.previous||
            aborted(online->common_cut(),decision)||!common_client->pair||
            online->source_players()!=common_client->pair->native->retained_world().admissions||
            common_client->role.previous!=decision.previous||common_client->role.cut!=decision.value.cut)return false;
        if(!retire_outer(decision.previous,decision.value.cut,OuterRetirement::Reason::ProspectiveClientRetired))return false;
        common_client.reset();return true;
    }
    bool retire_common_host_for_client(const PeerMigrationDecision &decision){
        if(!native_import)return true;
        const auto learned=online?online->peer_migration():std::nullopt;
        if(!learned||!learned->decision||*learned->decision!=decision||online->self()==decision.value.host||
            online->phase()!=OnlineMatchSession::Phase::Rebinding||online->authority_context()!=decision.previous||
            aborted(online->common_cut(),decision))return false;
        const auto &cut=SourcePeerHostTakeover::imported_cut(*native_import);
        if(cut.context!=decision.previous||cut.seal!=decision.value.cut||!imported_pair||
            online->source_players()!=imported_pair->native->retained_world().admissions)return false;
        if(!retire_outer(cut.context,cut.seal,OuterRetirement::Reason::ProspectiveHostRetired))return false;
        native_import.reset();imported_pair.reset();return true;
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
    void retire_inventory(){
        if(inventory_lease)inventory_lease->eligible=false;
        inventory_lease.reset();
    }
    bool retire_unpublished_preparation(){
        need(preparation_retirement.has_value());
        const auto &context=preparation_retirement->context();
        // Actual adopted/staged certificate roles are outside this pre-election
        // cancellation scope. The Session reservation never authorizes them.
        if(active||pending||pending_client)return false;
        std::array<OuterRetirement,6> requests{};std::size_t count{};
        const auto request=[&](const AuthorityTransferContext &owner_context,const AuthorityCutSeal &seal){
            if(owner_context!=context)return;
            for(std::size_t i=0;i<count;++i)if(requests[i].seal==seal)return;
            need(count<requests.size());requests[count++]={context,seal,OuterRetirement::Reason::CommonPreparationCancelled};
        };
        if(inventory_lease&&inventory_lease->context==context)
            for(const auto &pair:inventory_lease->pairs)if(pair)request(pair->native->cut().context,pair->native->cut().seal);
        if(native_import){const auto &cut=SourcePeerHostTakeover::imported_cut(*native_import);request(cut.context,cut.seal);}
        if(imported_pair)request(imported_pair->native->cut().context,imported_pair->native->cut().seal);
        if(common_client)request(common_client->role.previous,common_client->role.cut);
        // Outer holders may retire themselves on true. Preserve ALL actual
        // native banks until every exact cut permits retirement; retries are
        // idempotent and no Session lock spans these callbacks or destruction.
        for(std::size_t i=0;i<count;++i)
            if(!retire_outer(requests[i].context,requests[i].seal,requests[i].reason))return false;
        if(common_client&&common_client->role.previous==context)common_client.reset();
        if(native_import&&SourcePeerHostTakeover::imported_cut(*native_import).context==context)native_import.reset();
        if(imported_pair&&imported_pair->native->cut().context==context)imported_pair.reset();
        if(inventory_lease&&inventory_lease->context==context)retire_inventory();
        return true;
    }
    bool inventory_current(const CommonInventoryLease &lease)const{
        if(lease.executor_!=executor||!online||!inventory_lease||!inventory_lease->eligible)return false;
        const auto retained=lease.state_.lock();
        return retained==inventory_lease&&retained->self==online->self()&&
            online->authority_context()==retained->context;
    }
    bool retire_common(const std::optional<AuthorityTransferContext> &next){
        // Every session getter has returned before these owning values retire.
        // No large native/foreground destruction occurs under a session lock.
        const auto obsolete=[&](const AuthorityTransferContext &context){return !next||context!=*next;};
        std::array<OuterRetirement,14> requests{};std::size_t count{};
        const auto request=[&](const AuthorityTransferContext &context,const AuthorityCutSeal &seal){
            if(!obsolete(context))return;
            for(std::size_t i=0;i<count;++i)if(requests[i].context==context&&requests[i].seal==seal)return;
            need(count<requests.size());requests[count++]={context,seal,OuterRetirement::Reason::ContextRetired};
        };
        for(const auto &value:{committed_pair,pending_pair,imported_pair})if(value)request(value->native->cut().context,value->native->cut().seal);
        const bool retire_lease=inventory_lease&&obsolete(inventory_lease->context);
        if(retire_lease)for(const auto &value:inventory_lease->pairs)if(value)
            request(value->native->cut().context,value->native->cut().seal);
        if(pending_native)request(pending_native->cut().context,pending_native->cut().seal);
        if(native_import){const auto &cut=SourcePeerHostTakeover::imported_cut(*native_import);request(cut.context,cut.seal);}
        const bool retire_host=pending&&obsolete(staged_decision(*pending).previous);
        if(retire_host){const auto decision=staged_decision(*pending);request(decision.previous,decision.value.cut);}
        const bool retire_client=pending_client&&obsolete(pending_client->decision.previous);
        if(retire_client)request(pending_client->decision.previous,pending_client->decision.value.cut);
        const bool retire_common_client=common_client&&obsolete(common_client->role.previous);
        if(retire_common_client)request(common_client->role.previous,common_client->role.cut);
        const bool retire_retained=retained&&obsolete(retained->cut.context);
        if(retire_retained)request(retained->cut.context,retained->cut.seal);
        const bool retire_export=export_round&&obsolete(export_round->context);
        if(retire_export)request(export_round->context,export_round->seal);
        const bool retire_round=common_round&&obsolete(common_round->context);
        if(retire_round)request(common_round->context,common_round->seal);
        for(std::size_t i=0;i<count;++i)if(!retire_outer(requests[i].context,requests[i].seal,requests[i].reason))return false;
        if(retire_lease)retire_inventory();
        if(retire_host){pending_authority.reset();pending.reset();}
        if(retire_client)pending_client.reset();
        if(retire_common_client)common_client.reset();
        if(retire_retained)retained.reset();
        if(retire_export)clear_export();
        if(native_import&&obsolete(SourcePeerHostTakeover::imported_cut(*native_import).context))native_import.reset();
        if(imported_pair&&obsolete(imported_pair->native->cut().context))imported_pair.reset();
        if(pending_native&&obsolete(pending_native->cut().context))pending_native.reset();
        if(terminal_displayed&&obsolete(terminal_displayed->world().cut.context))terminal_displayed.reset();
        if(pending_pair&&obsolete(pending_pair->native->cut().context))pending_pair.reset();
        if(committed_pair&&obsolete(committed_pair->native->cut().context))committed_pair.reset();
        if(retire_round)common_round.reset();
        common_used=bool(committed_pair||pending_pair||imported_pair||pending_native||common_round);return true;
    }
    bool retire_context(){
        const auto context=online?online->authority_context():std::nullopt;
        if(context==common_context)return true;
        if(!retire_common(context))return false;
        common_context=context;return true;
    }
    std::shared_ptr<const CommonPair> pair(const AuthorityTransferContext &context,const AuthorityCutSeal &seal)const{
        for(const auto &value:{committed_pair,pending_pair,imported_pair})if(value&&value->native->cut().context==context&&
            value->native->cut().seal==seal)return value;
        for(const auto &value:{common_client?common_client->pair:nullptr,pending_client?pending_client->pair:nullptr})
            if(value&&value->native->cut().context==context&&value->native->cut().seal==seal)return value;
        if(inventory_lease&&inventory_lease->eligible&&inventory_lease->context==context)
            for(const auto &value:inventory_lease->pairs)if(value&&value->native->cut().seal==seal)return value;
        return {};
    }
    bool retire_aborted_common(){
        if(!online)return true;
        const auto context=online->authority_context();const auto state=online->common_cut();
        if(!state||!context||state->context!=*context||!state->pending||!state->abort)return true;
        // Abort retirement is valid while the predecessor is fenced too.
        // Do not require live Running association/service before dropping the
        // private import and any exact staged native publication owner.
        {
            const auto &round=*state->pending;const auto self=online->self();
            if(self>=16||!(round.recipients&(1u<<self))||(state->aborted&(1u<<self)))return true;
            const bool retire_export=export_round&&export_round->context==round.context&&export_round->seal==round.seal;
            if(retire_export&&common_export){
                // A promise future does not cancel its worker request when
                // reset. Drain/destroy it before acknowledging retirement.
                if(common_export->wait_for(std::chrono::milliseconds(0))!=std::future_status::ready)return false;
            }
            const auto matches=[&](const PeerMigrationDecision &decision){
                return decision.previous==round.context&&decision.value.cut==round.seal;
            };
            const bool retire_host=pending&&matches(staged_decision(*pending));
            const bool retire_client=pending_client&&matches(pending_client->decision);
            const bool retire_common_client=common_client&&common_client->role.previous==round.context&&common_client->role.cut==round.seal;
            const bool retire_lease=inventory_lease&&inventory_lease->context==round.context&&
                std::any_of(inventory_lease->pairs.begin(),inventory_lease->pairs.end(),
                    [&](const auto &value){return value&&names_cut(round,value->native->cut());});
            if(!retire_outer(round.context,round.seal,OuterRetirement::Reason::Abort))return false;
            if(retire_lease)retire_inventory();
            if(retire_export&&common_export)try{auto retired=common_export->get();}catch(...){}
            if(retire_host){pending_authority.reset();pending.reset();}
            if(retire_client)pending_client.reset();
            if(retire_common_client)common_client.reset();
            if(imported_pair&&names_cut(round,imported_pair->native->cut())){native_import.reset();imported_pair.reset();}
            if(retained&&names_cut(round,retained->cut))retained.reset();
            if(pending_native&&names_cut(round,pending_native->cut()))pending_native.reset();
            if(pending_pair&&names_cut(round,pending_pair->native->cut()))pending_pair.reset();
            if(retire_export)clear_export();
            online->acknowledge_common_cut_abort(round);return true;
        }
    }
    bool retirement_boundary(){return retire_context()&&retire_aborted_common()&&retire_obsolete_client_rebind();}
    void common_pins(){
        if(!online||!retirement_boundary())return;
        const auto context=online->authority_context();const auto state=online->common_cut();
        if(!state||!context||state->context!=*context||state->abort)return;
        if(online->phase()!=OnlineMatchSession::Phase::Running||!client())return;
        if(state->pending!=common_round){
            // Protocol replacement requires this endpoint's prior retirement
            // ACK. Keep the independently committed predecessor as slot one.
            if(imported_pair&&imported_pair==pending_pair&&imported_pair!=committed_pair){native_import.reset();imported_pair.reset();}
            clear_export();pending_native.reset();pending_pair.reset();common_round=state->pending;
        }
        if(!state->pending)return;
        common_used=true;const auto &round=*state->pending;const auto self=online->self();
        if(self>=16||!(round.recipients&(1u<<self)))return;
        const auto bit=std::uint16_t(1u<<self);
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
        if(!authority||!recovery_preparation)return;
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
                capture_retry=Clock::now()+std::chrono::seconds(3);
                if(publisher->capture()&&admitted_cameras&&!admitted_cameras->active())admitted_cameras->activate();
                state=publisher->state();
            }
            diagnostic(publication_error,"Source shadow publication unavailable: ",state.error);
        }catch(const std::exception &e){
            if(publisher&&!publisher->state().held_group)publisher->cancel();
            capture_retry=Clock::now()+std::chrono::seconds(3);
            diagnostic(publication_error,"Source shadow publication unavailable: ",e.what());
        }
    }
};
bool MultiplayerRuntimeAuthority::recovery_preparation_enabled(
    const std::shared_ptr<OnlineMatchSession> &online,RecoveryPreparation preparation){
    if(!online)return false;
    switch(preparation){
    case RecoveryPreparation::Automatic:return online->continuity().has_value()||online->terminal_recovery_enabled();
    case RecoveryPreparation::PreparationOnly:return true;
    }
    return false;
}
MultiplayerRuntimeAuthority::MultiplayerRuntimeAuthority(const MultiplayerOptions &options,
    std::shared_ptr<MultiplayerPreparedWorld> world,std::unique_ptr<LanHost> host,std::unique_ptr<LanClient> client,
    std::unique_ptr<SourceAuthorityReplayWorker> receiver,RecoveryPreparation preparation):impl_(std::make_unique<Impl>()){
    auto &s=*impl_;need(world&&!(host&&client));
    s.resident_cache=world->original_cache();need(world->matches(options));
    s.online=options.online_match;s.recovery_options=source_authority_options(options);s.resident=std::move(world);
    s.recovery_preparation=recovery_preparation_enabled(s.online,preparation);
    s.initial_host=std::move(host);s.initial_client=std::move(client);s.receiver=std::move(receiver);
    s.recovery_enabled=bool(s.receiver);
    need(!s.receiver||(s.online&&s.initial_client));
    if(s.online&&s.initial_host&&s.world().source_objects&&s.world().source_objects->owns_physics())
        s.authority=std::make_unique<SourceMatchAuthority>(s.world().match,*s.world().source_objects);
}
MultiplayerRuntimeAuthority::~MultiplayerRuntimeAuthority(){
    // Native objects and publisher frames must retire on their executor.
    if(impl_->executor!=std::this_thread::get_id()||impl_->frame||impl_->committing||impl_->retirement_guard)std::terminate();
}
MultiplayerRuntimeAuthority::OuterRetirementGuard::OuterRetirementGuard(Impl &owner,OuterRetirementGate gate,void *opaque):owner_(&owner){
    owner.quiescent();need(gate&&!owner.retirement_guard);
    owner.retirement_guard=this;owner.retirement_gate=gate;owner.retirement_owner=opaque;
}
MultiplayerRuntimeAuthority::OuterRetirementGuard::~OuterRetirementGuard(){
    auto &s=*owner_;
    if(s.executor!=std::this_thread::get_id()||s.frame||s.committing||s.retirement_guard!=this)std::terminate();
    s.retirement_gate=nullptr;s.retirement_owner=nullptr;s.retirement_guard=nullptr;
}
std::unique_ptr<MultiplayerRuntimeAuthority::OuterRetirementGuard> MultiplayerRuntimeAuthority::hold_outer_retirement_gate(
    OuterRetirementGate gate,void *opaque){
    return std::unique_ptr<OuterRetirementGuard>(new OuterRetirementGuard(*impl_,gate,opaque));
}
MultiplayerPreparedWorld &MultiplayerRuntimeAuthority::world(){impl_->own();return impl_->world();}
MultiplayerMatch &MultiplayerRuntimeAuthority::match(){return world().match;}
LanHost *MultiplayerRuntimeAuthority::host(){impl_->own();return impl_->host();}
LanClient *MultiplayerRuntimeAuthority::client(){impl_->own();return impl_->client();}
std::pair<std::unique_ptr<LanHost>,std::unique_ptr<LanClient>> MultiplayerRuntimeAuthority::release_system_link(){
    auto& s=*impl_;s.quiescent();need(!s.online&&!s.active&&!s.pending&&!s.receiver);
    if(s.initial_host)need(s.initial_host->system_link_state().phase==SystemLinkPhase::postgame);
    else {need(bool(s.initial_client));const auto state=s.initial_client->system_link_state();
      need(state && (state->phase==SystemLinkPhase::postgame || (state->phase==SystemLinkPhase::lobby && !s.initial_client->ready())));}
    return {std::move(s.initial_host),std::move(s.initial_client)};
}
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
        if(s.online->terminal_recovery_enabled()&&!s.admitted_cameras){
            need(s.world().match.original_data());
            AcceptedSourcePlayers accepted;accepted.context=*context;accepted.players=players;
            auto cameras=std::make_shared<SourceAdmittedCameraBank>(s.world().match,*s.world().source_objects,
                *s.world().match.original_data(),accepted,s.online->roster());
            s.host()->bind_admitted_cameras(cameras);s.admitted_cameras=std::move(cameras);
        }
        if(s.online->publish_source_players(*context,players))s.published_context=context;
    }catch(const std::exception &e){diagnostic(s.admission_error,"Source player admission unavailable: ",e.what());}
}
bool MultiplayerRuntimeAuthority::terminal_bootstrap_ready()const{
    auto &s=*impl_;s.quiescent();
    if(!s.online||!s.online->terminal_recovery_enabled())return true;
    if(s.online->phase()!=OnlineMatchSession::Phase::Running)return false;
    const auto context=s.online->authority_context();const auto admitted=s.online->source_players();
    if(!context||!admitted||admitted->context!=*context)return false;
    const auto recipients=std::uint16_t(context->peers&~(1u<<s.online->self()));
    if(!recipients)return true;
    if(s.host()) {
        if(!s.publisher)return false;
        const auto state=s.publisher->state();
        return state.host_ready&&state.group_ready&&state.group_ready->context==*context&&
            state.group_ready->recipients==recipients&&state.group_ready->seal.tick==s.world().match.simulation().ticks();
    }
    if(!s.client()||!s.receiver)return false;
    const auto endpoint=s.receiver->state().endpoint;
    // A copied real validation boundary survives ongoing tick0 heartbeat
    // replay. Mutable ready may already be cleared for its next native phase.
    // The caller separately matches the exact received body/sequence receipt.
    return endpoint&&endpoint->world().cut.peer==s.online->self()&&endpoint->world().cut.context==*context&&
        same_cut(endpoint->proof().cut(),endpoint->world().cut)&&endpoint->world().cut.seal.tick==0&&
        endpoint->world().checkpoint.match.simulation.ticks==0&&s.world().match.simulation().ticks()==0;
}
std::shared_ptr<const SourceAuthorityEndpoint> MultiplayerRuntimeAuthority::terminal_endpoint()const{
    auto &s=*impl_;s.own();
    if(!s.online||!s.online->terminal_recovery_enabled()||!s.receiver||!s.client())return {};
    auto endpoint=s.receiver->state().endpoint;
    if(!endpoint||s.online->authority_context()!=endpoint->world().cut.context)return {};
    return endpoint;
}
void MultiplayerRuntimeAuthority::adopt_terminal_endpoint(const std::shared_ptr<const SourceAuthorityEndpoint> &endpoint,
    const MultiplayerSnapshotOrigin &origin){
    auto &s=*impl_;s.own();
    need(s.online&&s.online->terminal_recovery_enabled()&&s.client()&&endpoint&&
        endpoint->world().cut.peer==s.online->self()&&s.online->authority_context()==endpoint->world().cut.context);
    const auto proof=endpoint->proof().prove_snapshot(*s.client(),origin);need(proof.matches(origin));
    s.terminal_displayed=endpoint;
}
SourceAdmittedCameraBank *MultiplayerRuntimeAuthority::admitted_cameras(){
    auto &s=*impl_;s.own();return s.admitted_cameras&&s.admitted_cameras->active()?s.admitted_cameras.get():nullptr;
}
void MultiplayerRuntimeAuthority::service(){
    auto &s=*impl_;s.quiescent();if(s.active&&s.active->publication_held())return;
    if(!s.retirement_boundary())return;
    if(s.preparation_retiring())return;
    admit_players();s.publish();receive();s.common_pins();
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
    if(s.receiver){const auto state=s.receiver->state();
        diagnostic(s.receiver_error,"Source shadow unavailable: ",state.error);
        diagnostic(s.terminal_error,"Optional terminal pin unavailable: ",state.endpoint_error);
    }
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
    auto &s=*impl_;s.quiescent();if(s.active&&s.active->publication_held())return {};
    if(!s.retirement_boundary())return {};
    if(s.common_client||s.preparation_retiring())return {};
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
    if(s.preparation_retiring()||!pin||pin!=s.pending_native||!s.online||!s.client()||s.online->phase()!=OnlineMatchSession::Phase::Running)return false;
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
MultiplayerRuntimeAuthority::CommonInventoryLease::CommonInventoryLease(const std::shared_ptr<State> &state)
    :state_(state),executor_(state->executor){}
bool MultiplayerRuntimeAuthority::CommonInventoryLease::current()const noexcept{
    // Do not acquire even a temporary strong owner on a foreign thread: it
    // could otherwise become the last owner of the actual retained pairs.
    if(executor_!=std::this_thread::get_id())return false;
    const auto state=state_.lock();return state&&state->eligible;
}
std::optional<MultiplayerRuntimeAuthority::CommonInventoryLease>
MultiplayerRuntimeAuthority::retain_common_inventory(std::uint64_t lease){
    auto &s=*impl_;s.quiescent();if(s.preparation_retiring()||!lease||!s.retirement_boundary()||!s.online||s.host()||!s.client()||!s.client()->ready())return {};
    const auto context=s.online->authority_context();if(!context)return {};
    if(s.inventory_lease){
        if(!s.inventory_lease->eligible||s.inventory_lease->context!=*context||
            s.inventory_lease->self!=s.online->self()||s.inventory_lease->inventory.lease!=lease)return {};
        return CommonInventoryLease(s.inventory_lease);
    }
    std::array<std::shared_ptr<const CommonPair>,3> pairs{};
    std::array<std::optional<AuthorityCutSeal>,3> cuts{};std::size_t count{};
    const auto self=s.online->self();const auto admitted=s.online->source_players();
    for(const auto &pair:{s.committed_pair,s.pending_pair,s.imported_pair,
        s.common_client?s.common_client->pair:nullptr,s.pending_client?s.pending_client->pair:nullptr})if(pair){
        const auto &cut=pair->native->cut();
        if(cut.peer!=self||cut.context!=*context||admitted!=pair->native->retained_world().admissions)return {};
        if(std::find(pairs.begin(),pairs.begin()+count,pair)!=pairs.begin()+count)continue;
        if(count==pairs.size())return {};
        pairs[count]=pair;cuts[count]=cut.seal;++count;
    }
    // Only Rust derives immutable Common provenance from the actual current
    // metadata snapshot. Neither native construction nor pin retention is an
    // irreversible Common phase witness.
    const auto prepared=make_retained_survivor_inventory(*context,lease,cuts,s.online->common_cut());
    if(prepared.admission!=PeerSurvivorInventoryAdmission::Admit||
        s.online->authority_context()!=context||s.online->self()!=self)return {};
    s.inventory_lease=std::make_shared<CommonInventoryLease::State>(self,*context,prepared.inventory,std::move(pairs));
    return CommonInventoryLease(s.inventory_lease);
}
std::optional<PeerSurvivorInventory> MultiplayerRuntimeAuthority::common_inventory(const CommonInventoryLease &lease){
    auto &s=*impl_;s.quiescent();
    if(!s.retirement_boundary()||!s.inventory_current(lease))return {};
    return s.inventory_lease->inventory;
}
MultiplayerRuntimeAuthority::MigrationPreparationOutcome MultiplayerRuntimeAuthority::advance_common_preparation(){
    auto &s=*impl_;s.quiescent();
    using Kind=OnlineMatchSession::MigrationPreparationKind;
    if(!s.online)return MigrationPreparationOutcome::Inactive;
    if(s.active&&s.active->publication_held())return MigrationPreparationOutcome::Refused;
    // Genuine Abort/context retirement keeps its existing outer gates and
    // before-ACK ordering even while an unpublished reservation is retained.
    if(!s.retirement_boundary())return s.preparation_retirement?
        MigrationPreparationOutcome::Retiring:MigrationPreparationOutcome::Refused;
    if(!s.preparation_retirement){
        const auto step=s.online->migration_preparation_request();
        if(!step.request)return step.outcome;
        const auto &request=step.request;
        if(request->kind()==Kind::Retire)s.preparation_retirement=request;
        else{
            std::optional<PeerSurvivorInventory> inventory;
            try{
                if(s.online->authority_context()==std::optional{request->context()}&&s.online->self()==request->self()){
                    // Adopt an already genuinely retained immutable lease if
                    // present. Its resource id need not equal the Rust ticket.
                    const auto id=s.inventory_lease?s.inventory_lease->inventory.lease:request->id();
                    const auto lease=retain_common_inventory(id);
                    if(lease&&s.inventory_current(*lease)&&s.inventory_lease->context==request->context()&&
                        s.inventory_lease->self==request->self())inventory=s.inventory_lease->inventory;
                }
            }catch(const std::exception &error){
                // The private native banks remain owned. Rust decides whether
                // failed capture requires retirement; it receives no invented
                // inventory, witness, Ready or native success flag.
                diagnostic(s.preparation_error,"Source migration preparation unavailable: ",error.what());
            }
            const auto outcome=s.online->complete_migration_preparation(*request,inventory);
            if(outcome==MigrationPreparationOutcome::Retiring){
                // Reserve before returning to the caller: even a role-only
                // preparation without a staged live replica must not admit a
                // frame/new preparation between failed admission and cleanup.
                const auto retire=s.online->migration_preparation_request();
                if(!retire.request)return retire.outcome;
                if(retire.request->kind()==Kind::Retire)s.preparation_retirement=retire.request;
                else return retire.outcome;
            }
            return outcome;
        }
    }
    if(!s.retire_unpublished_preparation())return MigrationPreparationOutcome::Retiring;
    // Only metadata and the opaque request survive this call. The actual
    // client staging/native scope/lease destruction finished outside its lock.
    const auto outcome=s.online->complete_migration_preparation(*s.preparation_retirement);
    s.preparation_retirement.reset();return outcome;
}
bool MultiplayerRuntimeAuthority::freeze_common_inventory(const CommonInventoryLease &lease){
    auto &s=*impl_;s.quiescent();
    if(s.preparation_retiring()||!s.retirement_boundary()||!s.inventory_current(lease))return false;
    const auto context=s.inventory_lease->context;const auto inventory=s.inventory_lease->inventory;
    // No local temporary strong lease survives this Session call. The Session
    // rechecks actual Common provenance/Abort under its generation mutex; any
    // resulting native retirement then occurs outside that mutex before ACK.
    const bool frozen=s.online->freeze_survivor_selection(context,inventory);
    return s.retirement_boundary()&&s.inventory_current(lease)&&frozen;
}
bool MultiplayerRuntimeAuthority::prepare_common_native(const ValidatedAuthorityCut &expected,
    std::shared_ptr<PhysicsLiveClockSource> clock){
    auto &s=*impl_;s.quiescent();if(s.preparation_retiring()||!s.retirement_boundary())return false;
    const auto current=[&]{
        if(!s.online||s.host()||!s.client()||!s.client()->ready()||s.pending||s.pending_client||
            s.online->self()!=expected.peer||s.online->authority_context()!=expected.context)return false;
        const auto phase=s.online->phase();
        return phase==OnlineMatchSession::Phase::Running||phase==OnlineMatchSession::Phase::AwaitingWorld||phase==OnlineMatchSession::Phase::Migrating;
    };
    if(!current())return false;
    if(s.common_client&&(s.common_client->role.previous!=expected.context||s.common_client->role.cut!=expected.seal))return false;
    const auto common=s.online->common_cut();
    if(common&&common->abort&&common->pending&&names_cut(*common->pending,expected))return false;
    if(s.native_import){const auto &cut=SourcePeerHostTakeover::imported_cut(*s.native_import);
        return same_cut(cut,expected)&&s.imported_pair&&
            s.online->source_players()==s.imported_pair->native->retained_world().admissions;
    }
    // Both owners must already be independently associated. A pending import
    // stays private and is retired before any Abort acknowledgement.
    const auto pair=s.pair(expected.context,expected.seal);
    if(!pair||!same_cut(pair->native->cut(),expected)||
        s.online->source_players()!=pair->native->retained_world().admissions)return false;
    auto imported=SourcePeerHostTakeover::import_native(s.recovery_options,pair->native->retained_world(),std::move(clock),s.resident.get());
    // Session progress runs independently while reconstruction is synchronous.
    // Recheck before retaining eligibility; no peer/Ready metadata is changed.
    if(!current()||s.online->source_players()!=pair->native->retained_world().admissions)return false;
    const auto after=s.online->common_cut();
    if(after&&after->abort&&after->pending&&names_cut(*after->pending,expected))return false;
    s.imported_pair=pair;s.native_import=std::move(imported);return true;
}
std::optional<ValidatedAuthorityCut> MultiplayerRuntimeAuthority::prepared_common_native()const{
    auto &s=*impl_;s.own();if(!s.native_import)return {};return SourcePeerHostTakeover::imported_cut(*s.native_import);
}
bool MultiplayerRuntimeAuthority::prepare_common_prospective(const ValidatedAuthorityCut &expected,
    std::shared_ptr<PhysicsLiveClockSource> clock){
    if(!prepare_common_native(expected,std::move(clock)))return false;
    auto &s=*impl_;
    const auto role=derive_peer_prospective_host_role(expected.context,expected.seal,expected.peer);
    return SourcePeerHostTakeover::prepare_prospective(s.native_import,role);
}
std::optional<PeerProspectiveHostRole> MultiplayerRuntimeAuthority::prepared_common_prospective()const{
    auto &s=*impl_;s.own();
    if(!s.native_import||!SourcePeerHostTakeover::has_prospective(*s.native_import))return {};
    return SourcePeerHostTakeover::prospective_role(*s.native_import);
}
bool MultiplayerRuntimeAuthority::prepare_common_client(const ValidatedAuthorityCut &expected){
    auto &s=*impl_;s.quiescent();if(s.preparation_retiring()||!s.retirement_boundary())return false;
    if(!s.online||s.host()||!s.initial_client||!s.initial_client->ready()||s.pending||s.pending_client||
        s.online->self()!=expected.peer||s.online->authority_context()!=expected.context||s.client_context!=expected.context)return false;
    if(s.common_client)return s.common_client->role.previous==expected.context&&s.common_client->role.cut==expected.seal&&s.common_client_ready();
    const auto phase=s.online->phase();const auto agreement=s.online->peer_migration();
    if((phase!=OnlineMatchSession::Phase::Running&&phase!=OnlineMatchSession::Phase::AwaitingWorld)||!agreement||agreement->decision)return false;
    const auto common=s.online->common_cut();
    if(common&&common->abort&&common->pending&&names_cut(*common->pending,expected))return false;
    if(s.native_import&&!same_cut(SourcePeerHostTakeover::imported_cut(*s.native_import),expected))return false;
    auto pair=s.pair(expected.context,expected.seal);
    if(!pair||!same_cut(pair->native->cut(),expected)||s.online->source_players()!=pair->native->retained_world().admissions||
        s.world().source_objects||!pair->foreground.proof().matches(pair->foreground.origin())||
        !pair->foreground.frontier().replica.matches(pair->foreground.origin())||
        !s.world().match.owns_replica_checkpoint(pair->foreground.frontier().replica))return false;
    PeerProspectiveClientRole role;
    try{role=derive_peer_prospective_client_role(expected.context,expected.seal,expected.peer);}
    catch(const std::invalid_argument &){return false;}
    auto native=s.initial_client->prepare_prospective_rebind(role,pair->foreground.proof());
    auto replica=s.world().match.prepare_replica(pair->foreground.frontier().replica);
    // All fallible receipt copying and live-replica staging precedes retention.
    // A rejected recheck destroys these unpublished local owners before any
    // later retirement ACK; no strong local pin spans retirement_boundary().
    const auto after=s.online->common_cut();const auto learned=s.online->peer_migration();
    if(s.online->authority_context()!=expected.context||!learned||learned->decision||
        s.online->source_players()!=pair->native->retained_world().admissions||
        (after&&after->abort&&after->pending&&names_cut(*after->pending,expected)))return false;
    s.common_client=Impl::CommonClient{role,std::move(pair),std::move(native),std::move(replica),&s.world().match};
    return s.retirement_boundary()&&s.common_client_ready();
}
std::optional<PeerProspectiveClientRole> MultiplayerRuntimeAuthority::prepared_common_client()const{
    auto &s=*impl_;s.own();return s.common_client_ready()?std::optional{s.common_client->role}:std::nullopt;
}
SourceForegroundWorldHandoff::Prepared MultiplayerRuntimeAuthority::prepare_common_foreground(
    const ValidatedAuthorityCut &expected,const SourceForegroundWorldHandoff &foreground){
    auto &s=*impl_;s.quiescent();need(!s.preparation_retiring()&&s.retirement_boundary()&&s.native_import&&s.imported_pair);
    need(same_cut(SourcePeerHostTakeover::imported_cut(*s.native_import),expected)&&
        SourcePeerHostTakeover::has_prospective(*s.native_import));
    return foreground.prepare(SourcePeerHostTakeover::prospective_target(*s.native_import),s.imported_pair->foreground,true);
}
bool MultiplayerRuntimeAuthority::seal_common_prospective(const ValidatedAuthorityCut &expected){
    auto &s=*impl_;s.quiescent();if(s.preparation_retiring()||!s.retirement_boundary()||!s.native_import||
        !SourcePeerHostTakeover::has_prospective(*s.native_import)||
        !same_cut(SourcePeerHostTakeover::imported_cut(*s.native_import),expected))return false;
    SourcePeerHostTakeover::seal_prospective(*s.native_import);
    return SourcePeerHostTakeover::prospective_ready(*s.native_import);
}
std::optional<MccSoundVariationProvider::NativeBinding> MultiplayerRuntimeAuthority::common_prospective_audio_binding(
    const ValidatedAuthorityCut &expected)const{
    auto &s=*impl_;s.own();if(s.preparation_retiring()||!s.native_import||!same_cut(SourcePeerHostTakeover::imported_cut(*s.native_import),expected)||
        !SourcePeerHostTakeover::prospective_ready(*s.native_import))return {};
    return SourcePeerHostTakeover::prospective_audio_binding(*s.native_import);
}
std::optional<SourceNativePublication::Boundary> MultiplayerRuntimeAuthority::common_prospective_boundary(
    const ValidatedAuthorityCut &expected)const{
    auto &s=*impl_;s.own();if(s.preparation_retiring()||!s.native_import||!same_cut(SourcePeerHostTakeover::imported_cut(*s.native_import),expected)||
        !SourcePeerHostTakeover::prospective_ready(*s.native_import))return {};
    return SourcePeerHostTakeover::prospective_boundary_binding(*s.native_import);
}
MultiplayerPreparedWorld &MultiplayerRuntimeAuthority::common_prospective_target_for_lens(const ValidatedAuthorityCut &expected){
    auto &s=*impl_;s.quiescent();need(!s.preparation_retiring()&&s.retirement_boundary()&&s.native_import&&s.imported_pair);
    need(same_cut(SourcePeerHostTakeover::imported_cut(*s.native_import),expected)&&
        SourcePeerHostTakeover::has_prospective(*s.native_import));
    return SourcePeerHostTakeover::prospective_target(*s.native_import);
}
MultiplayerRuntimeAuthority::RecoveryEligibility MultiplayerRuntimeAuthority::recovery_eligibility()const{
    auto &s=*impl_;s.quiescent();
    if(s.preparation_retiring()||!s.online||!s.client())return RecoveryEligibility::Unavailable;
    if(s.online->terminal_recovery_enabled()) {
        const auto terminal=s.online->terminal_observation();
        if(!terminal||s.online->authority_context()!=terminal->context()||s.common_used||s.committed_pair||s.pending_pair)
            return RecoveryEligibility::Unavailable;
        return s.terminal_displayed?RecoveryEligibility::LegacyDrain:RecoveryEligibility::Unavailable;
    }
    const auto migration=s.online->peer_migration();
    if(!migration||!migration->decision)return RecoveryEligibility::Unavailable;
    const auto common=s.online->common_cut();
    if(s.common_used||s.committed_pair||s.pending_pair||(common&&(common->pending||common->committed))){
        if(aborted(common,*migration->decision)||
            !s.pair(migration->decision->previous,migration->decision->value.cut))return RecoveryEligibility::Unavailable;
        return RecoveryEligibility::LearnedCommonCut;
    }
    return s.receiver||s.retained?RecoveryEligibility::LegacyDrain:RecoveryEligibility::Unavailable;
}
std::shared_ptr<const SourceAuthorityRetainedWorld> MultiplayerRuntimeAuthority::recover(){
    auto &s=*impl_;s.quiescent();if(s.preparation_retiring()||!s.retirement_boundary())return {};
    if(s.online&&s.online->terminal_recovery_enabled()){
        const auto terminal=s.online->terminal_observation();
        if(!terminal||!s.terminal_displayed||s.online->authority_context()!=terminal->context()||
            s.terminal_displayed->world().cut.context!=terminal->context())return {};
        // Preserve the actual displayed pair. Draining to a newer mutable
        // replay endpoint would also violate exact displayed-state equality.
        s.retained=std::shared_ptr<const SourceAuthorityRetainedWorld>(s.terminal_displayed,&s.terminal_displayed->world());
        if(s.receiver){s.receiver->cancel();if(!s.receiver->stopped())return {};s.receiver.reset();}
        return s.retained;
    }
    const auto migration=s.online?s.online->peer_migration():std::nullopt;
    const auto common=s.online?s.online->common_cut():std::nullopt;
    if(s.common_used||s.committed_pair||s.pending_pair||(common&&(common->pending||common->committed))){
        // Once a real common round exists, a later worker drain cannot stand
        // in for the independently learned cut. Retire its executor only after
        // selecting our exact owning native+foreground pair.
        if(!migration||!migration->decision)return {};
        const auto &decision=*migration->decision;if(aborted(common,decision))return {};
        const auto pair=s.pair(decision.previous,decision.value.cut);
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
    auto &s=*impl_;s.quiescent();need(!s.preparation_retiring()&&s.retirement_boundary());
    need(candidate&&s.online&&!s.host()&&!s.pending&&!s.pending_client&&!s.receiver);
    const auto state=s.online->peer_migration();
    const auto &decision=Impl::staged_decision(*candidate);
    need(state&&state->decision&&*state->decision==decision&&s.online->self()==decision.value.host);
    need(!aborted(s.online->common_cut(),decision));
    if(candidate->has_prospective()){
        auto authority=candidate->take_prospective_authority();need(bool(authority));
        s.pending_authority=std::move(authority);s.pending=std::move(candidate);return;
    }
    auto &world=candidate->world();need(world.source_objects&&world.source_objects->owns_physics()&&
        world.cache->path()==s.resident->cache->path()&&world.cache->shared_directory()==s.resident->cache->shared_directory());
    auto authority=std::make_unique<SourceMatchAuthority>(world.match,*world.source_objects);
    s.pending_authority=std::move(authority);s.pending=std::move(candidate);
}
SourcePeerHostTakeover *MultiplayerRuntimeAuthority::pending_takeover(){impl_->own();return impl_->pending.get();}
bool MultiplayerRuntimeAuthority::prepare_takeover(std::shared_ptr<PhysicsLiveClockSource> clock){
    auto &s=*impl_;s.quiescent();
    if(s.preparation_retiring())return false;
    const auto agreement=s.online?s.online->peer_migration():std::nullopt;
    if(!agreement||!agreement->decision)return false;
    return prepare_takeover(*agreement->decision,std::move(clock));
}
bool MultiplayerRuntimeAuthority::prepare_takeover(const PeerMigrationDecision &decision,std::shared_ptr<PhysicsLiveClockSource> clock){
    auto &s=*impl_;s.quiescent();
    if(s.preparation_retiring())return false;
    const auto agreement=s.online?s.online->peer_migration():std::nullopt;
    if(!agreement||!agreement->decision||*agreement->decision!=decision||decision.value.host!=s.online->self())return false;
    const auto common=s.online->common_cut();
    if(aborted(common,decision))return false;
    if(!s.retirement_boundary())return false;
    if(s.pending)return Impl::staged_decision(*s.pending)==decision&&s.retire_common_client_for_host(decision);
    if(s.host()||!s.client()||s.online->phase()!=OnlineMatchSession::Phase::Migrating)return false;
    if(s.native_import){const auto &cut=SourcePeerHostTakeover::imported_cut(*s.native_import);
        if(!s.imported_pair||cut.peer!=s.online->self()||cut.context!=decision.previous||cut.seal!=decision.value.cut||
            s.online->authority_context()!=cut.context||s.online->source_players()!=s.imported_pair->native->retained_world().admissions)return false;
    }
    const auto retained=recover();if(!retained)return false;
    auto candidate=s.native_import?SourcePeerHostTakeover::prepare_imported(s.native_import,decision):
        SourcePeerHostTakeover::prepare_with_resident_cache(s.recovery_options,*retained,decision,*s.resident,std::move(clock));
    s.native_import.reset();
    stage_takeover(std::move(candidate));return s.retire_common_client_for_host(decision);
}
bool MultiplayerRuntimeAuthority::activate_takeover(PresentationCommit presentation,void *owner){
    auto &s=*impl_;s.quiescent();if(s.preparation_retiring()||!s.retirement_boundary()||!s.pending)return false;
    if(!s.retire_common_client_for_host(Impl::staged_decision(*s.pending)))return false;
    need(!s.active&&!s.authority&&!s.initial_host&&!s.publisher&&!s.pending_client&&s.pending_authority);
    receive();need(bool(s.initial_client));
    if(s.pending->has_prospective())s.pending->refresh_prospective_local_input(*s.initial_client);
    else s.pending->refresh_local_input(*s.initial_client);
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
    auto *candidate=s.pending.get();
    if(!(candidate->has_prospective()?candidate->activate_prospective(commit,&adoption):candidate->activate(commit,&adoption)))return false;
    s.retained.reset();s.recovery.reset();
    return true;
}
void MultiplayerRuntimeAuthority::release_prospective_publication(){
    auto &s=*impl_;s.quiescent();need(s.active&&s.active->publication_held());
    s.active->release_prospective_publication();
}
bool MultiplayerRuntimeAuthority::prepare_client_rebind(){
    return prepare_client_rebind_from(nullptr,nullptr,nullptr);
}
bool MultiplayerRuntimeAuthority::prepare_client_rebind(const PeerMigrationDecision &decision){
    return prepare_client_rebind_from(nullptr,nullptr,nullptr,&decision);
}
bool MultiplayerRuntimeAuthority::prepare_client_rebind(const MultiplayerSnapshotProof &proof,
    const MultiplayerSnapshotOrigin &origin,const Simulation::ReplicaCheckpoint &replica){
    return prepare_client_rebind_from(&proof,&origin,&replica);
}
bool MultiplayerRuntimeAuthority::prepare_client_rebind_from(const MultiplayerSnapshotProof *proof,
    const MultiplayerSnapshotOrigin *origin,const Simulation::ReplicaCheckpoint *replica,const PeerMigrationDecision *expected){
    auto &s=*impl_;s.quiescent();
    if(s.preparation_retiring())return false;
    if(expected){const auto learned=s.online?s.online->peer_migration():std::nullopt;
        if(!learned||!learned->decision||*learned->decision!=*expected)return false;}
    if(!s.retirement_boundary())return false;
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
    const auto &decision=*agreement->decision;if(aborted(s.online->common_cut(),decision))return false;
    need(s.client_context==decision.previous);
    if(s.common_client){
        auto &prepared=*s.common_client;
        if(!s.common_client_ready()||prepared.role.previous!=decision.previous||prepared.role.cut!=decision.value.cut||
            s.online->source_players()!=prepared.pair->native->retained_world().admissions)return false;
        if(proof)need(prepared.pair->foreground.origin().same(*origin)&&proof->cut().context==decision.previous&&
            proof->cut().seal==decision.value.cut&&proof->cut().peer==s.online->self());
        try{(void)bind_peer_prospective_client_role(prepared.role,decision);}
        catch(const std::invalid_argument &){return false;}
        // The complete learned certificate and both actual prepared owners
        // were checked before retiring a losing private host target. A veto
        // preserves both alternatives, their scopes and the live replica.
        if(!s.retire_common_host_for_client(decision))return false;
        if(!s.initial_client->bind_prospective_rebind(*prepared.native,prepared.role,decision))return false;
        s.pending_client=Impl::ClientRebind{decision,std::move(prepared.native),prepared.pair->foreground.origin(),
            std::move(prepared.replica),prepared.replica_owner,std::move(prepared.pair)};
        s.common_client.reset();
        return s.retirement_boundary()&&s.pending_client&&s.pending_client->decision==decision;
    }
    auto prepared=proof?s.client()->prepare_rebind_host(decision.previous,decision.next,decision.value.cut,*proof):
        s.client()->prepare_rebind_host(decision.previous,decision.next,decision.value.cut);
    std::optional<Simulation::PreparedReplica> restored;
    if(replica)restored.emplace(s.world().match.prepare_replica(*replica));
    s.pending_client=Impl::ClientRebind{decision,std::move(prepared),origin?std::optional{*origin}:std::nullopt,
        std::move(restored),replica?&s.world().match:nullptr};return true;
}
bool MultiplayerRuntimeAuthority::cancel_client_rebind(){
    auto &s=*impl_;s.quiescent();if(!s.pending_client)return false;
    const auto decision=s.pending_client->decision;
    if(!s.retire_outer(decision.previous,decision.value.cut,OuterRetirement::Reason::ClientRebindRetired))return false;
    s.pending_client.reset();return true;
}
std::optional<PeerMigrationDecision> MultiplayerRuntimeAuthority::pending_client_rebind()const{
    auto &s=*impl_;s.own();return s.pending_client?std::optional(s.pending_client->decision):std::nullopt;
}
bool MultiplayerRuntimeAuthority::activate_client_rebind(PresentationCommit presentation,void *owner){
    auto &s=*impl_;s.quiescent();if(s.preparation_retiring()||!s.retirement_boundary()||!s.pending_client)return false;
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

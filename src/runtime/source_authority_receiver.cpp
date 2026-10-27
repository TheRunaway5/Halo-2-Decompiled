#include "halo2/source_authority_receiver.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_authority_delta.hpp"
#include "halo2/source_host_continuation_codec.hpp"
#include "halo2/source_peer_host_takeover.hpp"
#include "halo2/source_terminal_recovery.hpp"
#include "halo2/source_admitted_camera.hpp"
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <atomic>

namespace halo2 {
namespace {
using Clock=std::chrono::steady_clock;
// Only a complete-world cut takes this reservation. The permit survives the
// worker/result boundary and native staging; cancellation destroys it too.
// Atomic ownership permits worker acquisition and executor-thread release.
struct NetworkCutReservation {
    static inline std::atomic<bool> occupied=false;
    bool held{};
    ~NetworkCutReservation(){if(held)occupied.store(false,std::memory_order_release);}
    static std::unique_ptr<NetworkCutReservation> acquire(std::stop_token stop){
        auto reservation=std::make_unique<NetworkCutReservation>();
        while(!stop.stop_requested()){
            bool expected=false;
            if(occupied.compare_exchange_strong(expected,true,std::memory_order_acquire)){
                reservation->held=true;return reservation;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return {};
    }
};
static_assert(8*sizeof(SourceMatchAuthority::Checkpoint)<authority_network_fixed_bytes);
void need(bool ok){if(!ok)throw std::invalid_argument("Invalid source authority receiver ownership, admission or cut");}
std::uint64_t micros(Clock::time_point started){return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-started).count();}
void clear_ready(SourceAuthorityReceiverState &state){state.ready.reset();state.host_ready=false;state.host_proof.reset();}
class HostValidationDatagrams final:public MultiplayerDatagrams {
public:
    void send(std::span<const std::byte>,unsigned)override{throw FormatError("Private host validation cannot send gameplay");}
    bool receive(std::vector<std::byte> &,unsigned &)override{return false;}
};
bool delta(std::span<const std::byte> p){constexpr std::array<std::byte,8> magic{std::byte{'H'},std::byte{'2'},std::byte{'D'},std::byte{'L'},std::byte{'T'},std::byte{'0'},std::byte{'0'},std::byte{'1'}};
    return p.size()>=magic.size()&&std::ranges::equal(p.first(magic.size()),magic);}
}
struct SourceAuthorityHostProof::Data {
    ValidatedAuthorityCut cut;
    LanHost::Continuation host;
};
SourceAuthorityHostProof::SourceAuthorityHostProof(ValidatedAuthorityCut cut,LanHost::Continuation host)
    :data_(std::make_shared<const Data>(Data{std::move(cut),std::move(host)})){}
const ValidatedAuthorityCut &SourceAuthorityHostProof::cut()const {
    if(!data_)throw FormatError("Missing validated host cut proof");return data_->cut;
}
MultiplayerSnapshotProof SourceAuthorityHostProof::prove_snapshot(const LanClient &client,const MultiplayerSnapshotOrigin &origin)const {
    if(!data_)throw FormatError("Missing validated host cut proof");
    return client.prove_snapshot(data_->host,data_->cut,origin);
}
MultiplayerOptions source_authority_options(const MultiplayerOptions &input){
    MultiplayerOptions out;out.online_match=input.online_match;out.map=input.map;out.shared_maps=input.shared_maps;out.gameplay_xbe=input.gameplay_xbe;
    out.bsp=input.bsp;out.variant=input.variant;out.starting_weapon=input.starting_weapon;out.session_random_seed=input.session_random_seed;
    out.local_players=input.local_players;out.selected_teams=input.selected_teams;out.selected_profiles=input.selected_profiles;return out;
}
std::shared_ptr<MultiplayerPreparedWorld> SourceTerminalRecovery::prepare(const MultiplayerOptions &request,
    const SourceAuthorityRetainedWorld &saved,const OnlineMatchSession::TerminalObservation &observed) {
    const auto &session=request.online_match;
    need(session&&session->terminal_recovery_enabled()&&session->phase()==OnlineMatchSession::Phase::Failed&&
        session->terminal_observation().has_value()&&session->authority_context()==observed.context()&&
        session->self()==observed.self()&&observed.context()==saved.cut.context&&saved.cut.peer==observed.self()&&
        observed.context().host!=observed.self()&&saved.host.has_value()&&session->source_players()==saved.admissions&&
        saved.local_views==session->local_slots()&&saved.admissions.context==saved.cut.context&&
        saved.admissions.players.size()==session->roster().size()&&saved.local_views.size()==request.local_players&&
        saved.checkpoint.match.simulation.ticks==saved.cut.seal.tick&&
        observed.context().peers==std::uint16_t(observed.retired_peers()|(1u<<observed.self()))&&
        !(observed.retired_peers()&(1u<<observed.self()))&&observed.retired_peers());
    auto options=source_authority_options(request);options.source_object_world=request.source_object_world;
    options.source_object_savegame_directory=request.source_object_savegame_directory;
    std::vector<unsigned> producer_views;for(const auto &p:session->roster())
        if(online_admission_peer(p)==saved.cut.context.host)producer_views.push_back(p.slot);
    need(saved.producer_views==producer_views);
    // The target is wholly private: import under the producer's exact view
    // mapping, then run original local-view binding there. This is the same
    // cold native import owner used by SourcePeerHostTakeover; no second
    // complete executor or network host activation is necessary.
    auto target=MultiplayerPreparedWorld::source_shadow(options,MultiplayerPreparedWorld::SourceShadowViews::IncomingHost);
    need(target->local_slots==producer_views);
    for(auto slot:saved.local_views) {
        const auto &p=saved.checkpoint.match.players.at(slot);
        need(p.connected&&!(p.source_flags&2u)&&p.source_datum==saved.admissions.players.at(slot).player);
        if(!options.selected_profiles.empty())need(p.source_configuration==target->match.player(slot).source_configuration);
    }
    SourceMatchAuthority recipient(target->match,*target->source_objects);
    auto prepared=recipient.prepare(saved.checkpoint,source_authority_binding(saved.cut.context),saved.admissions.players);
    need(recipient.commit(std::move(prepared)));
    target->bind_source_local_views(saved.local_views);
    need(target->local_slots==session->local_slots()&&recipient.execution_tick()==saved.cut.seal.tick);
    // Membership comes only from the live authenticated direct observation.
    // Original depart keeps the source result row and does not synthesize a kill.
    for(const auto &p:session->roster())if(observed.retired_peers()&(1u<<online_admission_peer(p)))
        target->match.depart(p.slot);
    // The original target was configured with this PC's real local views.
    // Network authority stays retired; permit only a fresh local renderer to
    // consume this exact native owner after the old runtime has been destroyed.
    target->shadow_=false;return target;
}
struct SourceAuthorityReceiver::Impl {
    const MultiplayerOptions options;const std::shared_ptr<OnlineMatchSession> session;
    const SourceAuthorityStreamMode mode;
    const std::vector<OnlineAdmission> roster;const std::vector<unsigned> locals;
    const std::shared_ptr<PhysicsLiveClockSource> candidate_clock,workspace_clock;
    const std::thread::id executor=std::this_thread::get_id();bool enabled=true,receiving=true;
    std::optional<AuthorityTransferContext> context;std::optional<AuthorityArchiveImage> base;
    std::optional<SourceReplayBinding> replay;
    bool paired{};std::optional<LanHost::Continuation> host;
    std::unique_ptr<MultiplayerPreparedWorld> target,workspace;
    std::unique_ptr<SourceMatchAuthority> authority;
    std::unique_ptr<SourceAdmittedCameraBank> admitted_cameras;
    std::optional<SourceMatchAuthority::ExecutionBoundary> validated_boundary;
    SourceAuthorityReceiverState stats;
    std::vector<std::weak_ptr<const SourceAuthorityEndpoint>> endpoints;
    struct Job {std::uint64_t generation;ReceivedAuthorityCut cut;AcceptedSourcePlayers admitted;std::optional<AuthorityArchiveImage> base;
        std::optional<SourceReplayBinding> replay;bool after_replay{};};
    struct Result {std::uint64_t generation{},decode_us{};ReceivedAuthorityCut cut;AcceptedSourcePlayers admitted;
        std::unique_ptr<NetworkCutReservation> memory;
        std::optional<SourceMatchAuthority::Checkpoint> checkpoint;std::optional<AuthorityArchiveImage> image;
        std::optional<SourceReplayBatch> replay;std::optional<LanHost::Continuation> host;bool delta{};std::string error;};
    struct ActiveReplay {Result output;std::size_t at{};Clock::time_point started=Clock::now();};
    std::optional<ActiveReplay> active_replay;
    std::atomic<bool> replay_busy=false;
    mutable std::mutex mutex;std::condition_variable_any condition;std::uint64_t generation{};bool working{};
    std::optional<Job> job;std::optional<Result> result;
    std::jthread worker;
    Impl(const MultiplayerOptions &o,SourceAuthorityStreamMode m,std::shared_ptr<PhysicsLiveClockSource> c,std::shared_ptr<PhysicsLiveClockSource> w)
        :options(source_authority_options(o)),session(options.online_match),mode(m),roster(session?session->roster():std::vector<OnlineAdmission>{}),
         locals(session?session->local_slots():std::vector<unsigned>{}),candidate_clock(std::move(c)),workspace_clock(std::move(w)) {
        need(session&&session->direct_udp()&&!locals.empty()&&locals.size()<=4&&locals.size()==options.local_players);
        need(mode==SourceAuthorityStreamMode::Snapshots||mode==SourceAuthorityStreamMode::Replay);
        worker=std::jthread([this](std::stop_token stop){decode(stop);});
    }
    ~Impl(){worker.request_stop();condition.notify_all();if(worker.joinable())worker.join();}
    void own()const{need(std::this_thread::get_id()==executor);}
    void discard(){std::lock_guard lock(mutex);++generation;job.reset();result.reset();base.reset();replay.reset();active_replay.reset();replay_busy=false;host.reset();paired=false;clear_ready(stats);stats.rejected.reset();stats.endpoint.reset();stats.endpoint_error.clear();endpoints.clear();admitted_cameras.reset();authority.reset();target.reset();workspace.reset();stats.warmed=stats.workspace_warmed=false;condition.notify_all();}
    void reject_failed(const ReceivedAuthorityCut &cut){
        stats.rejected=ValidatedAuthorityCut{session->self(),cut.context,cut.seal};
        session->reject_authority(cut.context,cut.seal);
    }
    void validate_host(const LanHost::Continuation &saved,const AcceptedSourcePlayers &accepted){
        need(mode==SourceAuthorityStreamMode::Replay&&authority&&target&&context);
        const auto fingerprint=session->loaded_fingerprint();need(fingerprint.has_value());
        const auto before=authority->execution_boundary();
        auto checked=LanHost::resume(target->match,std::make_shared<HostValidationDatagrams>(),*fingerprint,roster,
            saved,source_authority_binding(*context),accepted.players);
        if(admitted_cameras)LanHost::verify_admitted_cameras(saved,*admitted_cameras);
        need(authority->execution_boundary()==before);
    }
    void reject_prefetched(){
        std::optional<ReceivedAuthorityCut> queued,decoded;
        {std::lock_guard lock(mutex);++generation;
            if(job)queued=job->cut;if(result)decoded=result->cut;
            job.reset();result.reset();}
        // A successor depends on the failed native predecessor. Release its
        // delivered slot and tell the sender to repair the complete chain.
        // The decoder rejects its own in-flight value on generation mismatch.
        if(queued)session->reject_authority(queued->context,queued->seal);
        if(decoded)session->reject_authority(decoded->context,decoded->seal);
    }
    void sync(){const auto current=session->authority_context();if(current!=context){discard();context=current;}}
    bool eligible()const{return enabled&&context&&context->host!=session->self()&&(context->peers&(1u<<session->self()));}
    bool available()const{std::lock_guard lock(mutex);return !working&&!job&&!result&&!active_replay;}
    bool decoder_available()const{std::lock_guard lock(mutex);return !working&&!job&&!result;}
    std::optional<AcceptedSourcePlayers> admissions()const {
        auto accepted=session->source_players();if(!accepted||!context||accepted->context!=*context||accepted->players.size()!=roster.size())return std::nullopt;
        return accepted;
    }
    bool warm(){
        if(!eligible()||!admissions())return false;
        if(target){
            if(!workspace){const auto started=Clock::now();
                workspace=MultiplayerPreparedWorld::source_shadow(options,mode==SourceAuthorityStreamMode::Replay?
                    MultiplayerPreparedWorld::SourceShadowViews::LocalPeer:MultiplayerPreparedWorld::SourceShadowViews::IncomingHost,workspace_clock);
                ++stats.workspace_constructions;stats.workspace_microseconds=micros(started);stats.workspace_warmed=true;}
            return true;
        }
        const auto started=Clock::now();
        auto world=MultiplayerPreparedWorld::source_shadow(options,mode==SourceAuthorityStreamMode::Replay?
            MultiplayerPreparedWorld::SourceShadowViews::IncomingHost:MultiplayerPreparedWorld::SourceShadowViews::LocalPeer,candidate_clock);
        auto owner=std::make_unique<SourceMatchAuthority>(world->match,*world->source_objects);
        target=std::move(world);authority=std::move(owner);stats.warm_microseconds=micros(started);stats.warmed=true;return true;
    }
    void decode(std::stop_token stop){
        while(!stop.stop_requested()){
            std::optional<Job> item;
            {std::unique_lock lock(mutex);condition.wait(lock,stop,[&]{return job&&(!job->after_replay||!replay_busy.load());});if(stop.stop_requested())return;item=std::move(job);job.reset();working=true;}
            Result out;out.generation=item->generation;out.cut=item->cut;out.admitted=std::move(item->admitted);const auto started=Clock::now();
            try {
                need(out.cut.bytes&&out.cut.bytes->size()<=authority_network_encoded_bytes&&
                    out.cut.peer==out.cut.context.host&&out.cut.context==out.admitted.context);
                if(source_replay_batch_payload(*out.cut.bytes)||source_host_replay_payload(*out.cut.bytes)){
                    need(mode==SourceAuthorityStreamMode::Replay&&item->replay.has_value()&&out.cut.seal.id>item->replay->previous.id);
                    if(source_host_replay_payload(*out.cut.bytes)){
                        const auto fingerprint=session->loaded_fingerprint();need(fingerprint.has_value());
                        auto cut=decode_source_host_replay(*out.cut.bytes,*item->replay,*fingerprint,roster,out.admitted.players);
                        out.replay=std::move(cut.replay);out.host=std::move(cut.host);
                    }else out.replay=decode_source_replay_batch(*out.cut.bytes,*item->replay);
                    need(out.cut.seal.tick==out.replay->steps.back().after_tick);
                }else {
                out.memory=NetworkCutReservation::acquire(stop);need(bool(out.memory));
                const auto limits=source_authority_network_codec_limits();
                if(source_host_continuation_payload(*out.cut.bytes)){
                    need(mode==SourceAuthorityStreamMode::Replay);const auto fingerprint=session->loaded_fingerprint();need(fingerprint.has_value());
                    auto cut=decode_source_host_continuation(*out.cut.bytes,out.cut.context,*fingerprint,roster,out.admitted.players,limits);
                    out.checkpoint=std::move(cut.world);out.host=std::move(cut.host);
                }else{
                    out.delta=delta(*out.cut.bytes);need(!out.delta||item->base.has_value());
                    const auto delta_limits=source_authority_network_delta_limits();
                    out.image=out.delta?apply_authority_delta(*item->base,*out.cut.bytes,delta_limits):AuthorityArchiveImage::read(*out.cut.bytes,delta_limits);
                    out.checkpoint=decode_source_authority_raw(out.image->raw(),out.image->digest(),limits);
                }
                const auto &saved=out.checkpoint->match;
                const auto binding=source_authority_binding(out.cut.context);
                need(saved.simulation.ticks==out.cut.seal.tick&&saved.admitted_binding==binding&&saved.simulation.admitted_binding==binding);
                for(unsigned slot=0;slot<multiplayer_capacity;++slot){const auto &player=saved.players[slot];
                    if(slot<roster.size())need(out.admitted.players[slot].slot==slot&&player.source_datum==out.admitted.players[slot].player&&player.name==roster[slot].name);
                    else need(!player.connected&&player.source_datum==null_datum);
                }
                }
            }catch(const std::exception &e){out.error=e.what();out.checkpoint.reset();out.image.reset();out.host.reset();out.memory.reset();}
            out.decode_us=micros(started);
            bool rejected=false;
            {std::lock_guard lock(mutex);working=false;
                if(out.generation==generation&&!stop.stop_requested())result=std::move(out);
                else rejected=true;}
            if(rejected)session->reject_authority(out.cut.context,out.cut.seal);
        }
    }
};
SourceAuthorityReceiver::SourceAuthorityReceiver(const MultiplayerOptions &o,std::shared_ptr<PhysicsLiveClockSource> c,std::shared_ptr<PhysicsLiveClockSource> w)
    :SourceAuthorityReceiver(o,SourceAuthorityStreamMode::Snapshots,std::move(c),std::move(w)){}
SourceAuthorityReceiver::SourceAuthorityReceiver(const MultiplayerOptions &o,SourceAuthorityStreamMode mode,
    std::shared_ptr<PhysicsLiveClockSource> c,std::shared_ptr<PhysicsLiveClockSource> w)
    :impl_(std::make_unique<Impl>(o,mode,std::move(c),std::move(w))){}
SourceAuthorityReceiver::~SourceAuthorityReceiver()=default;
bool SourceAuthorityReceiver::warm(){auto &s=*impl_;s.own();if(!s.enabled||!s.receiving)return false;s.sync();try{return s.warm();}catch(const std::exception &e){s.stats.error=e.what();return false;}}
void SourceAuthorityReceiver::update(){update(true);}
void SourceAuthorityReceiver::prefetch(){update(false);}
void SourceAuthorityReceiver::update(bool advance_replay){
    auto &s=*impl_;s.own();s.stats.replay_paused=!advance_replay;
    if(!s.enabled)return;s.sync();if(!s.eligible())return;
    // Metadata alone does not start a cold load in ordinary gameplay. An
    // explicit loading warm or a successfully decoded cut can construct one
    // executor per call, leaving a network/input pump boundary between them.
    bool pending_world=false;{std::lock_guard lock(s.mutex);pending_world=s.result&&s.result->checkpoint&&s.result->error.empty();}
    if(pending_world&&(!s.target||!s.workspace)){try{s.warm();}catch(const std::exception &e){s.stats.error=e.what();}return;}
    std::optional<Impl::Result> output;{std::lock_guard lock(s.mutex);if(!s.active_replay){output=std::move(s.result);s.result.reset();}}
    if(output){
        s.stats.decode_microseconds=output->decode_us;
        if(output->generation==s.generation&&output->cut.context==s.context){
            if(!output->error.empty()){s.stats.error=output->error;s.reject_failed(output->cut);}
            else if(output->replay)try{
                need(s.target&&s.authority&&s.replay&&output->replay->binding==*s.replay&&s.admissions()==output->admitted&&!s.active_replay&&
                    s.paired==output->host.has_value());
                need(s.validated_boundary&&s.authority->execution_boundary()==*s.validated_boundary&&
                    s.validated_boundary->tick==s.replay->previous.tick);
                clear_ready(s.stats);s.host.reset();s.active_replay.emplace(Impl::ActiveReplay{std::move(*output),0,Clock::now()});s.replay_busy=true;
            }catch(const std::exception &e){s.stats.error=e.what();s.reject_failed(output->cut);}
            else {bool commit_attempted=false;try {
                const auto accepted=s.admissions();need(accepted&&*accepted==output->admitted&&output->checkpoint&&(output->image||output->host)&&s.target&&s.workspace&&
                    (!s.paired||output->host));
                const auto started=Clock::now();SourceMatchAuthority staging(s.workspace->match,*s.workspace->source_objects);
                std::vector<SourcePlayerAdmission> locals;for(auto slot:s.locals){locals.push_back(accepted->players.at(slot));
                    const auto &expected=s.target->match.player(slot).source_configuration;
                    if(!s.options.selected_profiles.empty())need(expected&&output->checkpoint->match.players[slot].source_configuration==expected);}
                auto prepared=s.mode==SourceAuthorityStreamMode::Replay?
                    s.authority->prepare(*output->checkpoint,source_authority_binding(output->cut.context),accepted->players):
                    s.authority->prepare_with_local_views(*output->checkpoint,source_authority_binding(output->cut.context),locals,staging,accepted->players,s.workspace->local_slots);
                need(s.session->authority_context()==s.context&&s.admissions()==accepted);
                clear_ready(s.stats);s.host.reset();commit_attempted=true;
                need(s.authority->commit(std::move(prepared)));s.stats.prepare_microseconds=micros(started);
                if(s.session->terminal_recovery_enabled()){
                    need(output->host&&s.session->authority_admitted_camera_supported(output->cut.context.host));
                    // Received raw controller/observer words never initialize
                    // this owner. A fresh stream must begin at actual tick0;
                    // later state is obtained solely by authenticated numeric
                    // replay and checked against the sealed host sidecar.
                    s.admitted_cameras=std::make_unique<SourceAdmittedCameraBank>(s.target->match,*s.target->source_objects,
                        *s.target->match.original_data(),*accepted,s.roster);
                    s.admitted_cameras->activate();
                }
                if(output->host)s.validate_host(*output->host,*accepted);
                const ValidatedAuthorityCut validated{s.session->self(),output->cut.context,output->cut.seal};
                std::optional<SourceAuthorityHostProof> proof;
                if(output->host)proof=SourceAuthorityHostProof(validated,*output->host);
                const auto boundary=s.authority->execution_boundary();
                need(s.session->authority_context()==s.context&&s.session->acknowledge_authority(output->cut.context,output->cut.seal));
                s.base=std::move(output->image);s.host=std::move(output->host);s.paired=s.host.has_value();s.stats.host_ready=s.paired;
                s.stats.ready=validated;s.stats.host_proof=std::move(proof);s.stats.rejected.reset();
                s.validated_boundary=boundary;
                if(s.mode==SourceAuthorityStreamMode::Replay){s.replay=SourceReplayBinding{output->cut.context,output->cut.seal,output->cut.seal,1};s.stats.replay_sequence=0;}
                ++s.stats.prepared;s.stats.delta=output->delta;s.stats.error.clear();retain_endpoint();
            }catch(const std::exception &e){s.stats.error=e.what();s.reject_failed(output->cut);
                if(commit_attempted){clear_ready(s.stats);s.host.reset();s.admitted_cameras.reset();s.authority.reset();s.target.reset();s.stats.warmed=false;}}
            }
        }
    }
    if(advance_replay&&s.active_replay){try{
        auto &active=*s.active_replay;const auto &batch=*active.output.replay;
        need(s.replay&&batch.binding==*s.replay&&s.admissions()==active.output.admitted&&s.session->authority_context()==s.context);
        const auto &step=batch.steps.at(active.at);need(s.authority->execution_boundary().tick==step.before_tick);
        std::unique_ptr<SourceAdmittedCameraBank::Binding> cameras;
        if(s.admitted_cameras)cameras=std::make_unique<SourceAdmittedCameraBank::Binding>(*s.admitted_cameras);
        s.authority->replay_execution_phase(step.phase);cameras.reset();need(s.authority->execution_boundary().tick==step.after_tick);
        ++active.at;++s.stats.replayed_phases;
        if(active.at==batch.steps.size()){
            std::vector<unsigned> slots;for(const auto &player:active.output.admitted.players)slots.push_back(player.slot);
            if(s.authority->player_admissions(slots)!=active.output.admitted.players)
                throw FormatError("Replayed source roster differs from independent admission");
            if(active.output.host)s.validate_host(*active.output.host,active.output.admitted);
            const ValidatedAuthorityCut validated{s.session->self(),active.output.cut.context,active.output.cut.seal};
            std::optional<SourceAuthorityHostProof> proof;
            if(active.output.host)proof=SourceAuthorityHostProof(validated,*active.output.host);
            const auto boundary=s.authority->execution_boundary();
            need(s.session->acknowledge_authority(active.output.cut.context,active.output.cut.seal));
            s.replay->previous=active.output.cut.seal;s.replay->first_sequence+=active.at;
            s.stats.replay_sequence=s.replay->first_sequence-1;s.stats.replay_microseconds=micros(active.started);
            s.host=std::move(active.output.host);s.stats.host_ready=s.host.has_value();
            s.stats.ready=validated;s.stats.host_proof=std::move(proof);s.stats.rejected.reset();
            s.validated_boundary=boundary;
            ++s.stats.replay_batches;s.stats.error.clear();s.active_replay.reset();s.replay_busy=false;s.condition.notify_all();retain_endpoint();
        }
    }catch(const std::exception &e){s.stats.error=e.what();
        if(s.active_replay)s.reject_failed(s.active_replay->output.cut);
        s.active_replay.reset();s.replay_busy=false;s.condition.notify_all();s.replay.reset();clear_ready(s.stats);s.host.reset();
        s.reject_prefetched();
        s.admitted_cameras.reset();s.authority.reset();s.target.reset();s.stats.warmed=false;s.stats.replay_sequence=0;}}
    if(s.receiving&&s.decoder_available())if(auto accepted=s.admissions())if(auto cut=s.session->received_authority()){
        if(cut->context!=s.context||cut->peer!=s.context->host)return;
        auto expected=s.replay;
        // Preflight/decode one successor while native execution owns the
        // current batch. It cannot mutate or acknowledge the world until the
        // preceding batch has completed all its original execution checks.
        if(s.active_replay){need(expected.has_value());expected->previous=s.active_replay->output.cut.seal;
            expected->first_sequence+=s.active_replay->output.replay->steps.size();}
        // Preserve replay prefetch, but defer complete-world expansion until
        // the current replay's dynamic values have retired. Only its bounded
        // encoded transfer slot can overlap the full-cut reservation.
        const bool defer=s.active_replay&&!source_replay_batch_payload(*cut->bytes)&&!source_host_replay_payload(*cut->bytes);
        {std::lock_guard lock(s.mutex);s.job.emplace(Impl::Job{s.generation,std::move(*cut),std::move(*accepted),s.base,expected,defer});}
        ++s.stats.received;s.condition.notify_one();
    }
}
void SourceAuthorityReceiver::retain_endpoint(){
    auto &s=*impl_;
    if(!s.session->terminal_recovery_enabled()||!s.stats.ready||!s.stats.host_proof)return;
    // At most two older owning images plus the copy under construction.
    // The foreground retains its actual displayed image; the worker may
    // replace its unadopted pending image, without stopping authenticated replay.
    const std::weak_ptr<const SourceAuthorityEndpoint> previous=s.stats.endpoint;
    s.stats.endpoint.reset();
    std::erase_if(s.endpoints,[](const auto &p){return p.expired();});
    if(s.endpoints.size()>=3){++s.stats.endpoint_budget_skips;s.stats.endpoint=previous.lock();return;}
    const auto begin=Clock::now();
    try{
    auto world=std::make_shared<const SourceAuthorityRetainedWorld>(checkpoint_paired(*s.stats.ready));
    std::uint64_t bytes{};for(const auto &region:world->checkpoint.world.world.bank.regions)bytes+=region.bytes.size();
    s.stats.endpoint_bank_bytes=bytes;
    if(bytes>authority_network_native_bytes)throw FormatError("Terminal endpoint native bank bytes "+std::to_string(bytes)+
        " exceed local retention image limit "+std::to_string(authority_network_native_bytes));
    auto endpoint=std::shared_ptr<const SourceAuthorityEndpoint>(new SourceAuthorityEndpoint(*s.stats.host_proof,std::move(world)));
    s.endpoints.push_back(endpoint);s.stats.endpoint=std::move(endpoint);
    ++s.stats.endpoint_copies;s.stats.endpoint_copy_microseconds+=micros(begin);s.stats.endpoint_bank_bytes=bytes;s.stats.endpoint_error.clear();
    }catch(const std::exception &error){
        // A failed optional copy must not reject an already acknowledged
        // valid replay or relabel the previous display endpoint.
        ++s.stats.endpoint_copy_failures;
        try{s.stats.endpoint_error=std::string("Terminal endpoint retention unavailable: ")+error.what();}catch(...){}
        s.stats.endpoint=previous.lock();
    }
}
SourceAuthorityReceiverState SourceAuthorityReceiver::state()const{auto &s=*impl_;s.own();auto state=s.stats;state.replaying=s.active_replay.has_value();std::lock_guard lock(s.mutex);state.decoding=s.working||bool(s.job);return state;}
const MultiplayerPreparedWorld *SourceAuthorityReceiver::world()const{impl_->own();return impl_->target.get();}
SourceMatchAuthority::Checkpoint SourceAuthorityReceiver::checkpoint(const ValidatedAuthorityCut &expected)const {
    auto &s=*impl_;s.own();need(s.stats.ready&&s.stats.ready->peer==expected.peer&&s.stats.ready->context==expected.context&&
        s.stats.ready->seal==expected.seal&&s.authority&&!s.active_replay&&s.validated_boundary&&
        s.authority->execution_boundary()==*s.validated_boundary);
    return s.authority->checkpoint(source_authority_binding(expected.context));
}
SourceAuthorityRetainedWorld SourceAuthorityReceiver::checkpoint_paired(const ValidatedAuthorityCut &expected)const {
    auto &s=*impl_;s.own();
    const auto accepted=s.admissions();
    need(s.enabled&&s.receiving&&s.mode==SourceAuthorityStreamMode::Replay&&s.context==expected.context&&
        s.session->authority_context()==s.context&&expected.peer==s.session->self()&&accepted&&s.target&&s.authority&&
        s.paired&&s.host&&s.stats.host_ready&&s.stats.host_proof&&s.stats.ready&&
        s.stats.ready->peer==expected.peer&&s.stats.ready->context==expected.context&&s.stats.ready->seal==expected.seal&&
        s.stats.host_proof->cut().peer==expected.peer&&s.stats.host_proof->cut().context==expected.context&&
        s.stats.host_proof->cut().seal==expected.seal&&s.validated_boundary&&!s.active_replay&&
        s.authority->execution_boundary()==*s.validated_boundary);
    const auto boundary=*s.validated_boundary;
    SourceAuthorityRetainedWorld result{expected,*accepted,s.target->local_slots,s.locals,checkpoint(expected),s.host};
    // Session admission can change on its own transport thread while the owning
    // world copy is made. A rejected export changes neither local owner.
    need(s.session->authority_context()==s.context&&s.admissions()==accepted&&
        s.authority->execution_boundary()==boundary);
    return result;
}
std::optional<SourceAuthorityRetainedWorld> SourceAuthorityReceiver::retain_validated_world(){
    auto &s=*impl_;s.own();s.receiving=false;s.sync();
    need(s.enabled);
    if(!s.available())return {};
    const auto accepted=s.admissions();
    need(s.context&&s.stats.ready&&s.stats.ready->context==*s.context&&
        s.stats.ready->peer==s.session->self()&&accepted&&s.target&&s.authority);
    SourceAuthorityRetainedWorld out{*s.stats.ready,*accepted,s.target->local_slots,s.locals,
        checkpoint(*s.stats.ready),s.host};
    need(s.session->authority_context()==s.context&&s.admissions()==accepted);
    return out;
}
std::unique_ptr<MultiplayerPreparedWorld> SourceAuthorityReceiver::take_world(const ValidatedAuthorityCut &expected){
    auto &s=*impl_;s.own();if(!s.stats.ready||s.stats.ready->peer!=expected.peer||s.stats.ready->context!=expected.context||s.stats.ready->seal!=expected.seal)return {};
    need(s.authority&&s.validated_boundary&&s.authority->execution_boundary()==*s.validated_boundary&&!s.active_replay);
    s.enabled=false;s.authority.reset();auto world=std::move(s.target);s.discard();s.context.reset();return world;
}
void SourceAuthorityReceiver::cancel(){auto &s=*impl_;s.own();s.enabled=false;s.discard();s.context.reset();}
std::unique_ptr<MultiplayerPreparedWorld> SourceAuthorityReceiver::take_world_for_local_views(const ValidatedAuthorityCut &expected){
    auto &s=*impl_;s.own();if(s.mode==SourceAuthorityStreamMode::Snapshots)return take_world(expected);
    if(!s.stats.ready||s.stats.ready->peer!=expected.peer||s.stats.ready->context!=expected.context||s.stats.ready->seal!=expected.seal)return {};
    const auto accepted=s.admissions();need(accepted&&s.target&&s.workspace&&s.authority&&!s.active_replay);
    const auto binding=source_authority_binding(expected.context);const auto cut=s.authority->checkpoint(binding);
    std::vector<SourcePlayerAdmission> locals;for(auto slot:s.locals)locals.push_back(accepted->players.at(slot));
    clear_ready(s.stats);
    try{
        SourceMatchAuthority recipient(s.workspace->match,*s.workspace->source_objects);
        auto prepared=recipient.prepare_with_local_views(cut,binding,locals,*s.authority,accepted->players,s.target->local_slots);
        need(recipient.commit(std::move(prepared)));s.enabled=false;auto world=std::move(s.workspace);s.discard();s.context.reset();return world;
    }catch(...){s.discard();throw;}
}
std::unique_ptr<SourcePeerHostTakeover> SourceAuthorityReceiver::prepare_peer_host_takeover(const PeerMigrationDecision &decision){
    auto &s=*impl_;s.own();const auto learned=s.session->peer_migration();const auto accepted=s.admissions();
    need(s.mode==SourceAuthorityStreamMode::Replay&&s.enabled&&s.available()&&s.host&&s.stats.host_ready&&
        s.stats.ready&&s.stats.ready->peer==s.session->self()&&s.stats.ready->context==decision.previous&&
        s.stats.ready->seal==decision.value.cut&&decision.value.host==s.session->self()&&
        s.session->phase()==OnlineMatchSession::Phase::Migrating&&learned&&learned->decision&&
        *learned->decision==decision&&accepted&&s.session->authority_context()==decision.previous);
    const auto saved=*s.host;const auto ready=*s.stats.ready;
    auto world=take_world_for_local_views(ready);need(bool(world));
    // Failure discards only this private candidate. It cannot reopen media or
    // mutate the old rendered world; a different cut needs a new receiver.
    return std::unique_ptr<SourcePeerHostTakeover>(new SourcePeerHostTakeover(s.session,decision,*accepted,std::move(world),saved,ready));
}
struct SourceAuthorityReplayWorker::Impl {
    struct Request {
        ValidatedAuthorityCut expected;
        std::promise<SourceMatchAuthority::Checkpoint> result;
    };
    struct PairedRequest {
        ValidatedAuthorityCut expected;
        std::promise<SourceAuthorityRetainedWorld> result;
    };
    struct Retention {std::promise<SourceAuthorityRetainedWorld> result;};
    const MultiplayerOptions options;
    const std::shared_ptr<PhysicsLiveClockSource> candidate_clock,workspace_clock;
    mutable std::mutex mutex;
    std::condition_variable_any condition;
    SourceAuthorityReceiverState stats;
    std::optional<Request> request;
    std::optional<PairedRequest> paired_request;
    std::optional<Retention> retention_request;
    bool cancelled{},stopped{},exporting{},retention_requested{},replay_paused{};
    std::jthread worker;
    Impl(const MultiplayerOptions &o,std::shared_ptr<PhysicsLiveClockSource> c,std::shared_ptr<PhysicsLiveClockSource> w)
        :options(source_authority_options(o)),candidate_clock(std::move(c)),workspace_clock(std::move(w)),
         worker([this](std::stop_token stop){run(stop);}){}
    ~Impl(){cancel();if(worker.joinable())worker.join();}
    static std::exception_ptr unavailable(){return std::make_exception_ptr(std::runtime_error("Source replay export is unavailable, stale or cancelled"));}
    void cancel(){
        {std::lock_guard lock(mutex);cancelled=true;clear_ready(stats);stats.endpoint.reset();stats.rejected.reset();stats.warmed=stats.workspace_warmed=false;}
        worker.request_stop();condition.notify_all();
    }
    void run(std::stop_token stop)noexcept {
        std::optional<Retention> retention;
        bool holding=false;
        try {
            SourceAuthorityReceiver receiver(options,SourceAuthorityStreamMode::Replay,candidate_clock,workspace_clock);
            while(!stop.stop_requested()){
                std::optional<Request> pending;std::optional<PairedRequest> paired;bool paused;
                {std::lock_guard lock(mutex);pending=std::move(request);request.reset();
                    paired=std::move(paired_request);paired_request.reset();paused=replay_paused;
                    if(retention_request){retention=std::move(retention_request);retention_request.reset();holding=true;}}
                if(pending){
                    try {
                        need(options.online_match->authority_context()==pending->expected.context);
                        auto cut=receiver.checkpoint(pending->expected);
                        std::lock_guard lock(mutex);
                        if(cancelled)pending->result.set_exception(unavailable());
                        else pending->result.set_value(std::move(cut));
                        exporting=false;
                    }catch(...){std::lock_guard lock(mutex);pending->result.set_exception(std::current_exception());exporting=false;}
                }
                if(paired){
                    try {
                        need(options.online_match->authority_context()==paired->expected.context);
                        auto cut=receiver.checkpoint_paired(paired->expected);
                        std::lock_guard lock(mutex);
                        if(cancelled)paired->result.set_exception(unavailable());
                        else paired->result.set_value(std::move(cut));
                        exporting=false;
                    }catch(...){std::lock_guard lock(mutex);paired->result.set_exception(std::current_exception());exporting=false;}
                }
                if(retention){
                    try {
                        if(auto world=receiver.retain_validated_world()){
                            std::lock_guard lock(mutex);
                            if(cancelled)retention->result.set_exception(unavailable());
                            else retention->result.set_value(std::move(*world));
                            retention.reset();exporting=false;
                        }
                    }catch(...){std::lock_guard lock(mutex);retention->result.set_exception(std::current_exception());
                        retention.reset();exporting=false;}
                }
                if(stop.stop_requested())break;
                // These are private worlds. Warm only from the independently
                // admitted peer roster, while the render/input thread proceeds.
                const auto before=receiver.state();
                if(!holding&&!before.received&&(!before.warmed||!before.workspace_warmed))receiver.warm();
                if(paused)receiver.prefetch();else receiver.update();
                auto state=receiver.state();
                {std::unique_lock lock(mutex);if(cancelled)break;stats=std::move(state);
                    condition.wait_for(lock,stop,std::chrono::milliseconds(2),[&]{return bool(request)||bool(paired_request)||bool(retention_request)||cancelled||paused!=replay_paused;});}
            }
        }catch(const std::exception &e){std::lock_guard lock(mutex);stats.error=e.what();clear_ready(stats);stats.endpoint.reset();stats.warmed=stats.workspace_warmed=false;}
        catch(...){std::lock_guard lock(mutex);stats.error="Source replay executor failed";clear_ready(stats);stats.endpoint.reset();stats.warmed=stats.workspace_warmed=false;}
        {std::lock_guard lock(mutex);stats.endpoint.reset();stopped=true;exporting=false;
            if(request){request->result.set_exception(unavailable());request.reset();}
            if(paired_request){paired_request->result.set_exception(unavailable());paired_request.reset();}
            if(retention){retention->result.set_exception(unavailable());retention.reset();}
            if(retention_request){retention_request->result.set_exception(unavailable());retention_request.reset();}}
    }
};
SourceAuthorityReplayWorker::SourceAuthorityReplayWorker(const MultiplayerOptions &o,
    std::shared_ptr<PhysicsLiveClockSource> c,std::shared_ptr<PhysicsLiveClockSource> w)
    :impl_(std::make_unique<Impl>(o,std::move(c),std::move(w))){}
SourceAuthorityReplayWorker::~SourceAuthorityReplayWorker()=default;
SourceAuthorityReceiverState SourceAuthorityReplayWorker::state()const{std::lock_guard lock(impl_->mutex);return impl_->stats;}
bool SourceAuthorityReplayWorker::pause_replay(bool paused){
    {std::lock_guard lock(impl_->mutex);if(impl_->cancelled||impl_->stopped)return false;impl_->replay_paused=paused;}
    impl_->condition.notify_one();return true;
}
std::future<SourceMatchAuthority::Checkpoint> SourceAuthorityReplayWorker::checkpoint(ValidatedAuthorityCut expected){
    Impl::Request request{std::move(expected),{}};auto result=request.result.get_future();
    {std::lock_guard lock(impl_->mutex);
        if(impl_->cancelled||impl_->stopped||impl_->exporting||impl_->retention_requested)request.result.set_exception(Impl::unavailable());
        else {impl_->exporting=true;impl_->request=std::move(request);}}
    impl_->condition.notify_one();return result;
}
std::future<SourceAuthorityRetainedWorld> SourceAuthorityReplayWorker::checkpoint_paired(ValidatedAuthorityCut expected){
    Impl::PairedRequest request{std::move(expected),{}};auto result=request.result.get_future();
    {std::lock_guard lock(impl_->mutex);
        if(impl_->cancelled||impl_->stopped||impl_->exporting||impl_->retention_requested)request.result.set_exception(Impl::unavailable());
        else {impl_->exporting=true;impl_->paired_request=std::move(request);}}
    impl_->condition.notify_one();return result;
}
std::future<SourceAuthorityRetainedWorld> SourceAuthorityReplayWorker::retain_validated_world(){
    Impl::Retention request;auto result=request.result.get_future();
    {std::lock_guard lock(impl_->mutex);
        if(impl_->cancelled||impl_->stopped||impl_->exporting||impl_->retention_requested)
            request.result.set_exception(Impl::unavailable());
        else {impl_->exporting=true;impl_->retention_requested=true;impl_->retention_request=std::move(request);}}
    impl_->condition.notify_one();return result;
}
void SourceAuthorityReplayWorker::cancel(){impl_->cancel();}
bool SourceAuthorityReplayWorker::stopped()const{std::lock_guard lock(impl_->mutex);return impl_->stopped;}
}

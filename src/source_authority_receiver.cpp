#include "halo2/source_authority_receiver.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_authority_delta.hpp"
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace halo2 {
namespace {
using Clock=std::chrono::steady_clock;
void need(bool ok){if(!ok)throw std::invalid_argument("Invalid source authority receiver ownership, admission or cut");}
std::uint64_t micros(Clock::time_point started){return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-started).count();}
bool delta(std::span<const std::byte> p){constexpr std::array<std::byte,8> magic{std::byte{'H'},std::byte{'2'},std::byte{'D'},std::byte{'L'},std::byte{'T'},std::byte{'0'},std::byte{'0'},std::byte{'1'}};
    return p.size()>=magic.size()&&std::ranges::equal(p.first(magic.size()),magic);}
MultiplayerOptions gameplay_options(const MultiplayerOptions &input){
    MultiplayerOptions out;out.online_match=input.online_match;out.map=input.map;out.shared_maps=input.shared_maps;out.gameplay_xbe=input.gameplay_xbe;
    out.bsp=input.bsp;out.variant=input.variant;out.starting_weapon=input.starting_weapon;out.session_random_seed=input.session_random_seed;
    out.local_players=input.local_players;out.selected_teams=input.selected_teams;out.selected_profiles=input.selected_profiles;return out;
}
}
struct SourceAuthorityReceiver::Impl {
    const MultiplayerOptions options;const std::shared_ptr<OnlineMatchSession> session;
    const SourceAuthorityStreamMode mode;
    const std::vector<OnlineAdmission> roster;const std::vector<unsigned> locals;
    const std::shared_ptr<PhysicsLiveClockSource> candidate_clock,workspace_clock;
    const std::thread::id executor=std::this_thread::get_id();bool enabled=true;
    std::optional<AuthorityTransferContext> context;std::optional<AuthorityArchiveImage> base;
    std::optional<SourceReplayBinding> replay;
    std::unique_ptr<MultiplayerPreparedWorld> target,workspace;
    std::unique_ptr<SourceMatchAuthority> authority;
    SourceAuthorityReceiverState stats;
    struct Job {std::uint64_t generation;ReceivedAuthorityCut cut;AcceptedSourcePlayers admitted;std::optional<AuthorityArchiveImage> base;
        std::optional<SourceReplayBinding> replay;};
    struct Result {std::uint64_t generation{},decode_us{};ReceivedAuthorityCut cut;AcceptedSourcePlayers admitted;
        std::optional<SourceMatchAuthority::Checkpoint> checkpoint;std::optional<AuthorityArchiveImage> image;
        std::optional<SourceReplayBatch> replay;bool delta{};std::string error;};
    struct ActiveReplay {Result output;std::size_t at{};Clock::time_point started=Clock::now();};
    std::optional<ActiveReplay> active_replay;
    mutable std::mutex mutex;std::condition_variable_any condition;std::uint64_t generation{};bool working{};
    std::optional<Job> job;std::optional<Result> result;
    std::jthread worker;
    Impl(const MultiplayerOptions &o,SourceAuthorityStreamMode m,std::shared_ptr<PhysicsLiveClockSource> c,std::shared_ptr<PhysicsLiveClockSource> w)
        :options(gameplay_options(o)),session(options.online_match),mode(m),roster(session?session->roster():std::vector<OnlineAdmission>{}),
         locals(session?session->local_slots():std::vector<unsigned>{}),candidate_clock(std::move(c)),workspace_clock(std::move(w)) {
        need(session&&session->direct_udp()&&!locals.empty()&&locals.size()<=4&&locals.size()==options.local_players);
        need(mode==SourceAuthorityStreamMode::Snapshots||mode==SourceAuthorityStreamMode::Replay);
        worker=std::jthread([this](std::stop_token stop){decode(stop);});
    }
    ~Impl(){worker.request_stop();condition.notify_all();if(worker.joinable())worker.join();}
    void own()const{need(std::this_thread::get_id()==executor);}
    void discard(){std::lock_guard lock(mutex);++generation;job.reset();result.reset();base.reset();replay.reset();active_replay.reset();stats.ready.reset();authority.reset();target.reset();workspace.reset();stats.warmed=stats.workspace_warmed=false;}
    void sync(){const auto current=session->authority_context();if(current!=context){discard();context=current;}}
    bool eligible()const{return enabled&&context&&context->host!=session->self()&&(context->peers&(1u<<session->self()));}
    bool available()const{std::lock_guard lock(mutex);return !working&&!job&&!result&&!active_replay;}
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
            {std::unique_lock lock(mutex);condition.wait(lock,stop,[&]{return bool(job);});if(stop.stop_requested())return;item=std::move(job);job.reset();working=true;}
            Result out;out.generation=item->generation;out.cut=item->cut;out.admitted=std::move(item->admitted);const auto started=Clock::now();
            try {
                need(out.cut.bytes&&out.cut.peer==out.cut.context.host&&out.cut.context==out.admitted.context);
                if(source_replay_batch_payload(*out.cut.bytes)){
                    need(mode==SourceAuthorityStreamMode::Replay&&item->replay.has_value()&&out.cut.seal.id>item->replay->previous.id);
                    out.replay=decode_source_replay_batch(*out.cut.bytes,*item->replay);
                    need(out.cut.seal.tick==out.replay->steps.back().after_tick);
                }else {
                out.delta=delta(*out.cut.bytes);need(!out.delta||item->base.has_value());
                out.image=out.delta?apply_authority_delta(*item->base,*out.cut.bytes):AuthorityArchiveImage::read(*out.cut.bytes);
                SourceAuthorityCodecLimits limits;limits.encoded_bytes=64ull*1024*1024;
                out.checkpoint=decode_source_authority(out.image->encode(),limits);const auto &saved=out.checkpoint->match;
                const auto binding=source_authority_binding(out.cut.context);
                need(saved.simulation.ticks==out.cut.seal.tick&&saved.admitted_binding==binding&&saved.simulation.admitted_binding==binding);
                for(unsigned slot=0;slot<multiplayer_capacity;++slot){const auto &player=saved.players[slot];
                    if(slot<roster.size())need(out.admitted.players[slot].slot==slot&&player.source_datum==out.admitted.players[slot].player&&player.name==roster[slot].name);
                    else need(!player.connected&&player.source_datum==null_datum);
                }
                }
            }catch(const std::exception &e){out.error=e.what();out.checkpoint.reset();out.image.reset();}
            out.decode_us=micros(started);
            {std::lock_guard lock(mutex);working=false;if(out.generation==generation&&!stop.stop_requested())result=std::move(out);}
        }
    }
};
SourceAuthorityReceiver::SourceAuthorityReceiver(const MultiplayerOptions &o,std::shared_ptr<PhysicsLiveClockSource> c,std::shared_ptr<PhysicsLiveClockSource> w)
    :SourceAuthorityReceiver(o,SourceAuthorityStreamMode::Snapshots,std::move(c),std::move(w)){}
SourceAuthorityReceiver::SourceAuthorityReceiver(const MultiplayerOptions &o,SourceAuthorityStreamMode mode,
    std::shared_ptr<PhysicsLiveClockSource> c,std::shared_ptr<PhysicsLiveClockSource> w)
    :impl_(std::make_unique<Impl>(o,mode,std::move(c),std::move(w))){}
SourceAuthorityReceiver::~SourceAuthorityReceiver()=default;
bool SourceAuthorityReceiver::warm(){auto &s=*impl_;s.own();if(!s.enabled)return false;s.sync();try{return s.warm();}catch(const std::exception &e){s.stats.error=e.what();return false;}}
void SourceAuthorityReceiver::update(){
    auto &s=*impl_;s.own();if(!s.enabled)return;s.sync();if(!s.eligible())return;
    // Metadata alone does not start a cold load in ordinary gameplay. An
    // explicit loading warm or a successfully decoded cut can construct one
    // executor per call, leaving a network/input pump boundary between them.
    bool pending_world=false;{std::lock_guard lock(s.mutex);pending_world=s.result&&s.result->checkpoint&&s.result->error.empty();}
    if(pending_world&&(!s.target||!s.workspace)){try{s.warm();}catch(const std::exception &e){s.stats.error=e.what();}return;}
    std::optional<Impl::Result> output;{std::lock_guard lock(s.mutex);output=std::move(s.result);s.result.reset();}
    if(output){
        s.stats.decode_microseconds=output->decode_us;
        if(output->generation==s.generation&&output->cut.context==s.context){
            if(!output->error.empty())s.stats.error=output->error;
            else if(output->replay)try{
                need(s.target&&s.authority&&s.replay&&output->replay->binding==*s.replay&&s.admissions()==output->admitted&&!s.active_replay);
                need(s.authority->execution_boundary().tick==s.replay->previous.tick);
                s.stats.ready.reset();s.active_replay.emplace(Impl::ActiveReplay{std::move(*output),0,Clock::now()});
            }catch(const std::exception &e){s.stats.error=e.what();}
            else {bool commit_attempted=false;try {
                const auto accepted=s.admissions();need(accepted&&*accepted==output->admitted&&output->checkpoint&&output->image&&s.target&&s.workspace);
                const auto started=Clock::now();SourceMatchAuthority staging(s.workspace->match,*s.workspace->source_objects);
                std::vector<SourcePlayerAdmission> locals;for(auto slot:s.locals){locals.push_back(accepted->players.at(slot));
                    const auto &expected=s.target->match.player(slot).source_configuration;
                    if(!s.options.selected_profiles.empty())need(expected&&output->checkpoint->match.players[slot].source_configuration==expected);}
                auto prepared=s.mode==SourceAuthorityStreamMode::Replay?
                    s.authority->prepare(*output->checkpoint,source_authority_binding(output->cut.context),accepted->players):
                    s.authority->prepare_with_local_views(*output->checkpoint,source_authority_binding(output->cut.context),locals,staging,accepted->players,s.workspace->local_slots);
                need(s.session->authority_context()==s.context&&s.admissions()==accepted);
                s.stats.ready.reset();commit_attempted=true;
                need(s.authority->commit(std::move(prepared)));s.stats.prepare_microseconds=micros(started);
                need(s.session->authority_context()==s.context&&s.session->acknowledge_authority(output->cut.context,output->cut.seal));
                s.base=std::move(output->image);s.stats.ready=ValidatedAuthorityCut{s.session->self(),output->cut.context,output->cut.seal};
                if(s.mode==SourceAuthorityStreamMode::Replay){s.replay=SourceReplayBinding{output->cut.context,output->cut.seal,output->cut.seal,1};s.stats.replay_sequence=0;}
                ++s.stats.prepared;s.stats.delta=output->delta;s.stats.error.clear();
            }catch(const std::exception &e){s.stats.error=e.what();if(commit_attempted){s.authority.reset();s.target.reset();s.stats.warmed=false;}}
            }
        }
    }
    if(s.active_replay){try{
        auto &active=*s.active_replay;const auto &batch=*active.output.replay;
        need(s.replay&&batch.binding==*s.replay&&s.admissions()==active.output.admitted&&s.session->authority_context()==s.context);
        const auto &step=batch.steps.at(active.at);need(s.authority->execution_boundary().tick==step.before_tick);
        s.authority->replay_execution_phase(step.phase);need(s.authority->execution_boundary().tick==step.after_tick);
        ++active.at;++s.stats.replayed_phases;
        if(active.at==batch.steps.size()){
            std::vector<unsigned> slots;for(const auto &player:active.output.admitted.players)slots.push_back(player.slot);
            if(s.authority->player_admissions(slots)!=active.output.admitted.players)
                throw FormatError("Replayed source roster differs from independent admission");
            need(s.session->acknowledge_authority(active.output.cut.context,active.output.cut.seal));
            s.replay->previous=active.output.cut.seal;s.replay->first_sequence+=active.at;
            s.stats.replay_sequence=s.replay->first_sequence-1;s.stats.replay_microseconds=micros(active.started);
            s.stats.ready=ValidatedAuthorityCut{s.session->self(),active.output.cut.context,active.output.cut.seal};
            ++s.stats.replay_batches;s.stats.error.clear();s.active_replay.reset();
        }
    }catch(const std::exception &e){s.stats.error=e.what();s.active_replay.reset();s.replay.reset();s.stats.ready.reset();
        s.authority.reset();s.target.reset();s.stats.warmed=false;s.stats.replay_sequence=0;}}
    if(s.available())if(auto accepted=s.admissions())if(auto cut=s.session->received_authority()){
        if(cut->context!=s.context||cut->peer!=s.context->host)return;
        {std::lock_guard lock(s.mutex);s.job.emplace(Impl::Job{s.generation,std::move(*cut),std::move(*accepted),s.base,s.replay});}
        ++s.stats.received;s.condition.notify_one();
    }
}
SourceAuthorityReceiverState SourceAuthorityReceiver::state()const{auto &s=*impl_;s.own();auto state=s.stats;state.replaying=s.active_replay.has_value();std::lock_guard lock(s.mutex);state.decoding=s.working||bool(s.job);return state;}
const MultiplayerPreparedWorld *SourceAuthorityReceiver::world()const{impl_->own();return impl_->target.get();}
SourceMatchAuthority::Checkpoint SourceAuthorityReceiver::checkpoint(const ValidatedAuthorityCut &expected)const {
    auto &s=*impl_;s.own();need(s.stats.ready&&s.stats.ready->peer==expected.peer&&s.stats.ready->context==expected.context&&
        s.stats.ready->seal==expected.seal&&s.authority&&!s.active_replay);
    return s.authority->checkpoint(source_authority_binding(expected.context));
}
std::unique_ptr<MultiplayerPreparedWorld> SourceAuthorityReceiver::take_world(const ValidatedAuthorityCut &expected){
    auto &s=*impl_;s.own();if(!s.stats.ready||s.stats.ready->peer!=expected.peer||s.stats.ready->context!=expected.context||s.stats.ready->seal!=expected.seal)return {};
    s.enabled=false;s.authority.reset();auto world=std::move(s.target);s.discard();s.context.reset();return world;
}
void SourceAuthorityReceiver::cancel(){auto &s=*impl_;s.own();s.enabled=false;s.discard();s.context.reset();}
std::unique_ptr<MultiplayerPreparedWorld> SourceAuthorityReceiver::take_world_for_local_views(const ValidatedAuthorityCut &expected){
    auto &s=*impl_;s.own();if(s.mode==SourceAuthorityStreamMode::Snapshots)return take_world(expected);
    if(!s.stats.ready||s.stats.ready->peer!=expected.peer||s.stats.ready->context!=expected.context||s.stats.ready->seal!=expected.seal)return {};
    const auto accepted=s.admissions();need(accepted&&s.target&&s.workspace&&s.authority&&!s.active_replay);
    const auto binding=source_authority_binding(expected.context);const auto cut=s.authority->checkpoint(binding);
    std::vector<SourcePlayerAdmission> locals;for(auto slot:s.locals)locals.push_back(accepted->players.at(slot));
    s.stats.ready.reset();
    try{
        SourceMatchAuthority recipient(s.workspace->match,*s.workspace->source_objects);
        auto prepared=recipient.prepare_with_local_views(cut,binding,locals,*s.authority,accepted->players,s.target->local_slots);
        need(recipient.commit(std::move(prepared)));s.enabled=false;auto world=std::move(s.workspace);s.discard();s.context.reset();return world;
    }catch(...){s.discard();throw;}
}
struct SourceAuthorityReplayWorker::Impl {
    struct Request {
        ValidatedAuthorityCut expected;
        std::promise<SourceMatchAuthority::Checkpoint> result;
    };
    const MultiplayerOptions options;
    const std::shared_ptr<PhysicsLiveClockSource> candidate_clock,workspace_clock;
    mutable std::mutex mutex;
    std::condition_variable_any condition;
    SourceAuthorityReceiverState stats;
    std::optional<Request> request;
    bool cancelled{},stopped{},exporting{};
    std::jthread worker;
    Impl(const MultiplayerOptions &o,std::shared_ptr<PhysicsLiveClockSource> c,std::shared_ptr<PhysicsLiveClockSource> w)
        :options(gameplay_options(o)),candidate_clock(std::move(c)),workspace_clock(std::move(w)),
         worker([this](std::stop_token stop){run(stop);}){}
    ~Impl(){cancel();if(worker.joinable())worker.join();}
    static std::exception_ptr unavailable(){return std::make_exception_ptr(std::runtime_error("Source replay export is unavailable, stale or cancelled"));}
    void cancel(){
        {std::lock_guard lock(mutex);cancelled=true;stats.ready.reset();stats.warmed=stats.workspace_warmed=false;}
        worker.request_stop();condition.notify_all();
    }
    void run(std::stop_token stop)noexcept {
        try {
            SourceAuthorityReceiver receiver(options,SourceAuthorityStreamMode::Replay,candidate_clock,workspace_clock);
            while(!stop.stop_requested()){
                std::optional<Request> pending;
                {std::lock_guard lock(mutex);pending=std::move(request);request.reset();}
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
                if(stop.stop_requested())break;
                // These are private worlds. Warm only from the independently
                // admitted peer roster, while the render/input thread proceeds.
                const auto before=receiver.state();
                if(!before.received&&(!before.warmed||!before.workspace_warmed))receiver.warm();
                receiver.update();
                auto state=receiver.state();
                {std::unique_lock lock(mutex);if(cancelled)break;stats=std::move(state);
                    condition.wait_for(lock,stop,std::chrono::milliseconds(2),[&]{return bool(request)||cancelled;});}
            }
        }catch(const std::exception &e){std::lock_guard lock(mutex);stats.error=e.what();stats.ready.reset();stats.warmed=stats.workspace_warmed=false;}
        catch(...){std::lock_guard lock(mutex);stats.error="Source replay executor failed";stats.ready.reset();stats.warmed=stats.workspace_warmed=false;}
        {std::lock_guard lock(mutex);stopped=true;exporting=false;
            if(request){request->result.set_exception(unavailable());request.reset();}}
    }
};
SourceAuthorityReplayWorker::SourceAuthorityReplayWorker(const MultiplayerOptions &o,
    std::shared_ptr<PhysicsLiveClockSource> c,std::shared_ptr<PhysicsLiveClockSource> w)
    :impl_(std::make_unique<Impl>(o,std::move(c),std::move(w))){}
SourceAuthorityReplayWorker::~SourceAuthorityReplayWorker()=default;
SourceAuthorityReceiverState SourceAuthorityReplayWorker::state()const{std::lock_guard lock(impl_->mutex);return impl_->stats;}
std::future<SourceMatchAuthority::Checkpoint> SourceAuthorityReplayWorker::checkpoint(ValidatedAuthorityCut expected){
    Impl::Request request{std::move(expected),{}};auto result=request.result.get_future();
    {std::lock_guard lock(impl_->mutex);
        if(impl_->cancelled||impl_->stopped||impl_->exporting)request.result.set_exception(Impl::unavailable());
        else {impl_->exporting=true;impl_->request=std::move(request);}}
    impl_->condition.notify_one();return result;
}
void SourceAuthorityReplayWorker::cancel(){impl_->cancel();}
}

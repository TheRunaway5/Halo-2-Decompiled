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
    const std::vector<OnlineAdmission> roster;const std::vector<unsigned> locals;
    const std::shared_ptr<PhysicsLiveClockSource> candidate_clock,workspace_clock;
    const std::thread::id executor=std::this_thread::get_id();bool enabled=true;
    std::optional<AuthorityTransferContext> context;std::optional<AuthorityArchiveImage> base;
    std::unique_ptr<MultiplayerPreparedWorld> target;
    std::unique_ptr<SourceMatchAuthority> authority;
    SourceAuthorityReceiverState stats;
    struct Job {std::uint64_t generation;ReceivedAuthorityCut cut;AcceptedSourcePlayers admitted;std::optional<AuthorityArchiveImage> base;};
    struct Result {std::uint64_t generation{},decode_us{};ReceivedAuthorityCut cut;AcceptedSourcePlayers admitted;
        std::optional<SourceMatchAuthority::Checkpoint> checkpoint;std::optional<AuthorityArchiveImage> image;bool delta{};std::string error;};
    mutable std::mutex mutex;std::condition_variable_any condition;std::uint64_t generation{};bool working{};
    std::optional<Job> job;std::optional<Result> result;
    std::jthread worker;
    Impl(const MultiplayerOptions &o,std::shared_ptr<PhysicsLiveClockSource> c,std::shared_ptr<PhysicsLiveClockSource> w)
        :options(gameplay_options(o)),session(options.online_match),roster(session?session->roster():std::vector<OnlineAdmission>{}),
         locals(session?session->local_slots():std::vector<unsigned>{}),candidate_clock(std::move(c)),workspace_clock(std::move(w)) {
        need(session&&session->direct_udp()&&!locals.empty()&&locals.size()<=4&&locals.size()==options.local_players);
        worker=std::jthread([this](std::stop_token stop){decode(stop);});
    }
    ~Impl(){worker.request_stop();condition.notify_all();if(worker.joinable())worker.join();}
    void own()const{need(std::this_thread::get_id()==executor);}
    void discard(){std::lock_guard lock(mutex);++generation;job.reset();result.reset();base.reset();stats.ready.reset();authority.reset();target.reset();stats.warmed=false;}
    void sync(){const auto current=session->authority_context();if(current!=context){discard();context=current;}}
    bool eligible()const{return enabled&&context&&context->host!=session->self()&&(context->peers&(1u<<session->self()));}
    bool available()const{std::lock_guard lock(mutex);return !working&&!job&&!result;}
    std::optional<AcceptedSourcePlayers> admissions()const {
        auto accepted=session->source_players();if(!accepted||!context||accepted->context!=*context||accepted->players.size()!=roster.size())return std::nullopt;
        return accepted;
    }
    bool warm(){
        if(!eligible()||!admissions())return false;if(target)return true;const auto started=Clock::now();
        auto world=MultiplayerPreparedWorld::source_shadow(options,MultiplayerPreparedWorld::SourceShadowViews::LocalPeer,candidate_clock);
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
            }catch(const std::exception &e){out.error=e.what();out.checkpoint.reset();out.image.reset();}
            out.decode_us=micros(started);
            {std::lock_guard lock(mutex);working=false;if(out.generation==generation&&!stop.stop_requested())result=std::move(out);}
        }
    }
};
SourceAuthorityReceiver::SourceAuthorityReceiver(const MultiplayerOptions &o,std::shared_ptr<PhysicsLiveClockSource> c,std::shared_ptr<PhysicsLiveClockSource> w)
    :impl_(std::make_unique<Impl>(o,std::move(c),std::move(w))){}
SourceAuthorityReceiver::~SourceAuthorityReceiver()=default;
bool SourceAuthorityReceiver::warm(){auto &s=*impl_;s.own();if(!s.enabled)return false;s.sync();try{return s.warm();}catch(const std::exception &e){s.stats.error=e.what();return false;}}
void SourceAuthorityReceiver::update(){
    auto &s=*impl_;s.own();if(!s.enabled)return;s.sync();if(!s.eligible())return;
    std::optional<Impl::Result> output;{std::lock_guard lock(s.mutex);output=std::move(s.result);s.result.reset();}
    if(output){
        s.stats.decode_microseconds=output->decode_us;
        if(output->generation==s.generation&&output->cut.context==s.context){
            if(!output->error.empty())s.stats.error=output->error;
            else {bool commit_attempted=false;try {
                const auto accepted=s.admissions();need(accepted&&*accepted==output->admitted&&output->checkpoint&&output->image&&s.warm());
                const auto started=Clock::now();auto workspace=MultiplayerPreparedWorld::source_shadow(s.options,MultiplayerPreparedWorld::SourceShadowViews::IncomingHost,s.workspace_clock);
                SourceMatchAuthority staging(workspace->match,*workspace->source_objects);
                std::vector<SourcePlayerAdmission> locals;for(auto slot:s.locals){locals.push_back(accepted->players.at(slot));
                    const auto &expected=s.target->match.player(slot).source_configuration;
                    if(!s.options.selected_profiles.empty())need(expected&&output->checkpoint->match.players[slot].source_configuration==expected);}
                auto prepared=s.authority->prepare_with_local_views(*output->checkpoint,source_authority_binding(output->cut.context),locals,staging,accepted->players);
                need(s.session->authority_context()==s.context&&s.admissions()==accepted);
                s.stats.ready.reset();commit_attempted=true;
                need(s.authority->commit(std::move(prepared)));s.stats.prepare_microseconds=micros(started);
                need(s.session->authority_context()==s.context&&s.session->acknowledge_authority(output->cut.context,output->cut.seal));
                s.base=std::move(output->image);s.stats.ready=ValidatedAuthorityCut{s.session->self(),output->cut.context,output->cut.seal};
                ++s.stats.prepared;s.stats.delta=output->delta;s.stats.error.clear();
            }catch(const std::exception &e){s.stats.error=e.what();if(commit_attempted){s.authority.reset();s.target.reset();s.stats.warmed=false;}}
            }
        }
    }
    if(s.available())if(auto accepted=s.admissions())if(auto cut=s.session->received_authority()){
        if(cut->context!=s.context||cut->peer!=s.context->host)return;
        {std::lock_guard lock(s.mutex);s.job.emplace(Impl::Job{s.generation,std::move(*cut),std::move(*accepted),s.base});}
        ++s.stats.received;s.condition.notify_one();
    }
}
SourceAuthorityReceiverState SourceAuthorityReceiver::state()const{auto &s=*impl_;s.own();auto state=s.stats;std::lock_guard lock(s.mutex);state.decoding=s.working||bool(s.job);return state;}
const MultiplayerPreparedWorld *SourceAuthorityReceiver::world()const{impl_->own();return impl_->target.get();}
std::unique_ptr<MultiplayerPreparedWorld> SourceAuthorityReceiver::take_world(const ValidatedAuthorityCut &expected){
    auto &s=*impl_;s.own();if(!s.stats.ready||s.stats.ready->peer!=expected.peer||s.stats.ready->context!=expected.context||s.stats.ready->seal!=expected.seal)return {};
    s.enabled=false;s.authority.reset();auto world=std::move(s.target);s.discard();s.context.reset();return world;
}
void SourceAuthorityReceiver::cancel(){auto &s=*impl_;s.own();s.enabled=false;s.discard();s.context.reset();}
}

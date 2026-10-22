#include "halo2/source_authority_publisher.hpp"
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <deque>
#include <algorithm>

namespace halo2 {
namespace {
using Clock=std::chrono::steady_clock;
using ArchiveBytes=std::vector<std::byte>;
void need(bool ok){if(!ok)throw std::invalid_argument("Invalid source authority publisher ownership or endpoint");}
std::uint64_t micros(Clock::time_point started){return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-started).count();}
}
struct SourceAuthorityPublisher::Impl {
    SourceMatchAuthority &authority;const std::shared_ptr<OnlineMatchSession> session;const std::vector<unsigned> targets;
    const std::uint16_t recipients;
    const SourceAuthorityStreamMode mode;LanHost *host{};
    const std::chrono::milliseconds timeout;const std::thread::id executor=std::this_thread::get_id();
    std::optional<AuthorityTransferContext> context;std::optional<AuthorityArchiveImage> base;
    SourceAuthorityPublisherState stats;
    std::optional<LanHost::Continuation> validated_host;
    struct Pending {AuthorityTransferContext context;AuthorityCutSeal seal;std::optional<AuthorityArchiveImage> image;
        std::optional<SourceReplayBinding> replay;std::uint64_t phases{};Clock::time_point started;bool paired{};
        std::optional<LanHost::Continuation> host;std::uint16_t expected{},validated{};};
    std::deque<Pending> pending;
    std::optional<SourceReplayBinding> replay;
    std::optional<SourceMatchAuthority::ExecutionBoundary> recorded_boundary;
    bool frame_active{};
    struct QueuedPhase {SourceReplayStep step;std::optional<LanHost::Continuation> host;std::uint64_t bytes{};};
    std::deque<QueuedPhase> phases;std::uint64_t queued_bytes{};
    static constexpr std::uint64_t queue_limit=64ull*1024*1024;
    struct Job {std::uint64_t generation{};AuthorityTransferContext context;std::optional<SourceMatchAuthority::Checkpoint> cut;
        std::optional<SourceReplayBatch> replay;std::optional<AuthorityArchiveImage> base;std::optional<LanHost::Continuation> host;std::uint64_t serial{};
        std::optional<PeerSnapshotToken> candidate;};
    struct Result {std::uint64_t generation{},tick{},encoding_us{},full_bytes{};AuthorityTransferContext context;
        std::optional<AuthorityArchiveImage> image;std::optional<SourceReplayBinding> replay;std::uint64_t phases{};
        ArchiveBytes payload;std::optional<LanHost::Continuation> host;bool delta{},paired{};std::string error;
        std::optional<PeerSnapshotToken> candidate;};
    mutable std::mutex mutex;std::condition_variable_any condition;
    std::uint64_t generation{};bool working{};std::optional<Job> job;std::optional<Result> result;
    // Join before destroying the captured value queue or any session borrower.
    std::jthread worker;
    static std::uint16_t recipient_mask(std::span<const unsigned> peers){
        need(!peers.empty()&&peers.size()<16);std::uint16_t mask{};
        for(auto peer:peers){need(peer<16&&!(mask&(1u<<peer)));mask|=std::uint16_t(1u<<peer);}return mask;
    }
    Impl(SourceMatchAuthority &a,std::shared_ptr<OnlineMatchSession> s,std::span<const unsigned> t,SourceAuthorityStreamMode m,std::chrono::milliseconds wait,LanHost *h=nullptr)
        :authority(a),session(std::move(s)),targets(t.begin(),t.end()),recipients(recipient_mask(t)),mode(m),host(h),timeout(wait){
        need(session&&session->direct_udp()&&!(recipients&(1u<<session->self()))&&wait>=std::chrono::seconds(1)&&wait<=std::chrono::minutes(5));
        need(mode==SourceAuthorityStreamMode::Snapshots||mode==SourceAuthorityStreamMode::Replay);
        need(!host||source_host_continuation_owner(*host,authority));
        worker=std::jthread([this](std::stop_token stop){encode(stop);});
    }
    ~Impl(){worker.request_stop();condition.notify_all();if(worker.joinable())worker.join();}
    void own()const{need(std::this_thread::get_id()==executor);}
    void append(std::uint64_t before,SourceExecutionPhase phase){
        const auto after=authority.execution_boundary();
        std::optional<LanHost::Continuation> saved;
        if(host)saved=host->checkpoint(source_authority_binding(*context));
        const auto size=source_replay_phase_allocation(phase)+sizeof(QueuedPhase)+(saved?LanHost::continuation_allocation(*saved):0);
        need(phases.size()<1024&&size<=queue_limit-queued_bytes);
        need(authority.execution_boundary()==after);
        phases.push_back({{before,after.tick,std::move(phase)},std::move(saved),size});queued_bytes+=size;recorded_boundary=after;
        ++stats.recorded_phases;
    }
    void discard(bool preserve_completed=false){std::lock_guard lock(mutex);++generation;job.reset();result.reset();pending.clear();base.reset();replay.reset();
        phases.clear();queued_bytes=0;recorded_boundary.reset();stats.ready.reset();
        if(!preserve_completed||targets.size()==1){stats.group_ready.reset();validated_host.reset();stats.host_ready=false;}
        stats.pending_group.reset();stats.pending_native_validated_peers=0;stats.awaiting_validation=false;stats.held_group.reset();stats.validation_timed_out=false;
        stats.snapshot_candidate.reset();stats.snapshot_candidate_group.reset();}
    void discard_stream(){
        if(!stats.common_cut_barrier||!stats.held_group){discard(targets.size()>1);return;}
        // A held historical cut is independent of the successor journal. A
        // bounded recording/encoding failure may lose that journal, but must
        // keep its exact pending ACKs/host value until explicit release/abort.
        std::lock_guard lock(mutex);++generation;job.reset();result.reset();base.reset();replay.reset();
        phases.clear();queued_bytes=0;recorded_boundary.reset();stats.awaiting_validation=!pending.empty();
    }
    bool available()const{std::lock_guard lock(mutex);return !working&&!job&&!result;}
    bool active(const std::optional<AuthorityTransferContext> &c)const{
        return c&&c->host==session->self()&&(c->peers&recipients)==recipients&&session->phase()==OnlineMatchSession::Phase::Running&&
            std::ranges::all_of(targets,[&](unsigned peer){return session->authority_supported(peer)&&session->source_players_confirmed(peer)&&
                (mode!=SourceAuthorityStreamMode::Replay||session->authority_replay_supported(peer))&&
                (!host||session->authority_host_continuation_supported(peer));});
    }
    bool candidate_active(const PeerSnapshotToken &token)const{
        if(!stats.snapshot_candidate_mode||!stats.common_cut_barrier||!host||context!=token.context||
            token.recipients!=recipients)return false;
        const auto candidates=session->snapshot_candidates();if(!candidates)return false;
        const auto state=candidates->state();
        return state.context==token.context&&state.recipients==recipients&&state.pending==token&&
            state.armed==recipients&&state.published&&!state.closing;
    }
    void encode(std::stop_token stop){
        while(!stop.stop_requested()){
            std::optional<Job> item;
            {std::unique_lock lock(mutex);condition.wait(lock,stop,[&]{return bool(job);});if(stop.stop_requested())return;
                item=std::move(job);job.reset();working=true;}
            Result out;out.generation=item->generation;out.context=item->context;out.candidate=item->candidate;const auto started=Clock::now();
            try {
                out.host=item->host;
                if(item->replay){
                    need(!item->cut&&item->replay->binding.context==item->context&&!item->replay->steps.empty());
                    out.replay=item->replay->binding;out.phases=item->replay->steps.size();out.tick=item->replay->steps.back().after_tick;
                    out.paired=item->host.has_value();
                    out.payload=item->host?encode_source_host_replay({std::move(*item->replay),std::move(*item->host)}):encode_source_replay_batch(*item->replay);
                }else {
                need(item->cut.has_value());out.tick=item->cut->match.simulation.ticks;
                const auto binding=source_authority_binding(item->context);
                need(item->cut->match.admitted_binding==binding&&item->cut->match.simulation.admitted_binding==binding);
                SourceAuthorityCodecLimits limits;limits.encoded_bytes=64ull*1024*1024;
                out.paired=item->host.has_value();
                out.payload=item->host?encode_source_host_continuation({item->context,item->serial,std::move(*item->cut),std::move(*item->host)},limits):
                    encode_source_authority(*item->cut,limits);out.full_bytes=out.payload.size();
                if(!out.paired)out.image=AuthorityArchiveImage::read(out.payload);
                if(item->base&&!out.paired){auto delta=encode_authority_delta(*item->base,*out.image);
                    if(delta.size()<out.payload.size()){out.payload=std::move(delta);out.delta=true;}}
                }
            }catch(const std::exception &e){out.error=e.what();out.image.reset();out.payload.clear();}
            out.encoding_us=micros(started);
            {std::lock_guard lock(mutex);working=false;if(out.generation==generation&&!stop.stop_requested())result=std::move(out);}
        }
    }
};
SourceAuthorityPublisher::SourceAuthorityPublisher(SourceMatchAuthority &a,std::shared_ptr<OnlineMatchSession> s,unsigned target,std::chrono::milliseconds timeout)
    :SourceAuthorityPublisher(a,std::move(s),target,SourceAuthorityStreamMode::Snapshots,timeout){}
SourceAuthorityPublisher::SourceAuthorityPublisher(SourceMatchAuthority &a,std::shared_ptr<OnlineMatchSession> s,unsigned target,
    SourceAuthorityStreamMode mode,std::chrono::milliseconds timeout):impl_(std::make_unique<Impl>(a,std::move(s),std::array<unsigned,1>{target},mode,timeout)){}
SourceAuthorityPublisher::~SourceAuthorityPublisher()=default;
SourceAuthorityPublisher::SourceAuthorityPublisher(SourceMatchAuthority &a,LanHost &h,std::shared_ptr<OnlineMatchSession> s,
    unsigned target,std::chrono::milliseconds timeout):impl_(std::make_unique<Impl>(a,std::move(s),std::array<unsigned,1>{target},SourceAuthorityStreamMode::Replay,timeout,&h)){}
SourceAuthorityPublisher::SourceAuthorityPublisher(SourceMatchAuthority &a,LanHost &h,std::shared_ptr<OnlineMatchSession> s,
    std::span<const unsigned> targets,std::chrono::milliseconds timeout):impl_(std::make_unique<Impl>(a,std::move(s),targets,SourceAuthorityStreamMode::Replay,timeout,&h)){}
void SourceAuthorityPublisher::cancel(){impl_->own();need(!impl_->frame_active);impl_->discard();impl_->context.reset();}
void SourceAuthorityPublisher::set_common_cut_barrier(bool enabled){
    auto &s=*impl_;s.own();need(!s.frame_active);
    if(s.stats.common_cut_barrier==enabled)return;
    need(!s.recorded_boundary&&s.pending.empty()&&!s.stats.held_group&&s.available()&&(!s.stats.snapshot_candidate_mode||enabled));
    s.stats.common_cut_barrier=enabled;
}
void SourceAuthorityPublisher::set_snapshot_candidate_mode(bool enabled){
    auto &s=*impl_;s.own();need(!s.frame_active);
    if(s.stats.snapshot_candidate_mode==enabled)return;
    need(!s.recorded_boundary&&s.pending.empty()&&!s.stats.held_group&&!s.stats.snapshot_candidate&&s.available());
    need(!enabled||(s.host&&s.stats.common_cut_barrier));
    s.stats.snapshot_candidate_mode=enabled;
}
bool SourceAuthorityPublisher::abort_snapshot_candidate(const PeerSnapshotToken &token){
    auto &s=*impl_;s.own();need(!s.frame_active);
    if(!s.stats.snapshot_candidate_mode||s.stats.snapshot_candidate!=token||s.stats.held_group)return false;
    s.discard(s.targets.size()>1);return true;
}
bool SourceAuthorityPublisher::release_common_cut(const SourceAuthorityPublisherGroupCut &expected){
    auto &s=*impl_;s.own();need(!s.frame_active);
    if(!s.stats.common_cut_barrier||s.stats.held_group!=expected||s.stats.group_ready!=expected||!s.pending.empty())return false;
    s.stats.held_group.reset();s.stats.snapshot_candidate.reset();s.stats.snapshot_candidate_group.reset();return true;
}
bool SourceAuthorityPublisher::abort_common_cut(const SourceAuthorityPublisherGroupCut &expected){
    auto &s=*impl_;s.own();need(!s.frame_active);
    if(!s.stats.common_cut_barrier||s.stats.held_group!=expected)return false;
    s.discard();return true;
}
SourceAuthorityPublisherState SourceAuthorityPublisher::state()const{
    impl_->own();auto stats=impl_->stats;stats.queued_phases=impl_->phases.size();stats.queued_bytes=impl_->queued_bytes;
    if(!impl_->pending.empty()){const auto &pending=impl_->pending.front();
        stats.pending_group=SourceAuthorityPublisherGroupCut{pending.context,pending.seal,pending.expected};
        stats.pending_native_validated_peers=pending.validated;}
    stats.tracking_replay=impl_->recorded_boundary.has_value();std::lock_guard lock(impl_->mutex);stats.encoding=impl_->working||bool(impl_->job);return stats;
}
unsigned SourceAuthorityPublisher::in_flight_cuts()const{impl_->own();return unsigned(impl_->pending.size());}
LanHost::Continuation SourceAuthorityPublisher::host_checkpoint(const ValidatedAuthorityCut &expected)const{
    const auto &s=*impl_;s.own();need(s.stats.ready&&s.stats.host_ready&&s.validated_host&&
        s.stats.ready->peer==expected.peer&&s.stats.ready->context==expected.context&&s.stats.ready->seal==expected.seal);
    return *s.validated_host;
}
LanHost::Continuation SourceAuthorityPublisher::host_checkpoint(const SourceAuthorityPublisherGroupCut &expected)const{
    const auto &s=*impl_;s.own();need(s.stats.group_ready&&*s.stats.group_ready==expected&&s.stats.host_ready&&s.validated_host);
    return *s.validated_host;
}
void SourceAuthorityPublisher::update(){
    auto &s=*impl_;s.own();need(!s.frame_active);const auto current=s.session->authority_context();
    if(current!=s.context){s.discard();s.context=current;}
    if(!s.active(current)){
        // A readiness loss invalidates this candidate, never room membership.
        s.discard_stream();return;
    }
    if(s.recorded_boundary&&s.authority.execution_boundary()!=*s.recorded_boundary){
        s.discard_stream();s.stats.error="Unrecorded native work between replay phases";return;
    }
    for(const auto &receipt:s.session->validated_authority())if(!s.pending.empty()&&receipt.peer<16&&
        (s.pending.front().expected&(1u<<receipt.peer))&&receipt.context==s.pending.front().context&&receipt.seal==s.pending.front().seal){
        auto &pending=s.pending.front();const auto bit=std::uint16_t(1u<<receipt.peer);
        if(pending.validated&bit)continue;
        pending.validated|=bit;++s.stats.native_receipts;
        if(pending.validated!=pending.expected)continue;
        if(s.pending.front().replay){s.stats.acknowledged_phases+=s.pending.front().phases;
        }else {need(s.pending.front().image.has_value()||s.pending.front().paired);s.base=s.pending.front().image;
            if(s.mode==SourceAuthorityStreamMode::Replay&&!s.stats.snapshot_candidate_mode)s.replay=SourceReplayBinding{receipt.context,receipt.seal,receipt.seal,1};}
        if(s.targets.size()==1)s.stats.ready=receipt;
        s.stats.group_ready=SourceAuthorityPublisherGroupCut{pending.context,pending.seal,pending.expected};
        s.stats.host_ready=pending.paired;s.validated_host=std::move(pending.host);
        ++s.stats.validations;s.pending.pop_front();s.stats.awaiting_validation=!s.pending.empty();
    }
    if(!s.pending.empty()&&Clock::now()-s.pending.front().started>s.timeout){s.discard_stream();s.stats.validation_timed_out=true;s.stats.error="Shadow executor validation timed out";return;}
    const bool pipeline=!s.stats.snapshot_candidate_mode&&!s.stats.common_cut_barrier&&s.targets.size()==1&&s.session->authority_pipeline_supported(s.targets.front());
    const auto appendable=[&]{return pipeline&&s.pending.size()==1&&s.pending.back().replay&&
        s.session->authority_delivered(s.targets.front(),s.pending.back().context,s.pending.back().seal);};
    std::optional<Impl::Result> output;
    {std::lock_guard lock(s.mutex);if(!s.stats.held_group&&s.result&&(s.pending.empty()||(s.result->replay&&appendable()))){output=std::move(s.result);s.result.reset();}}
    if(output&&output->generation==s.generation&&output->context==current){
    s.stats.encoding_microseconds=output->encoding_us;
    if(!output->error.empty()){const auto error=output->error;s.discard_stream();s.stats.error=error;return;}
    try {
        if(s.stats.snapshot_candidate_mode)
            need(output->candidate&&s.stats.snapshot_candidate==output->candidate&&s.candidate_active(*output->candidate)&&!output->replay);
        else need(!output->candidate);
        need((s.pending.empty()||(output->replay&&appendable()))&&(output->image||output->replay||output->paired));
        if(output->replay)need(s.replay&&*s.replay==*output->replay);
        const auto bytes=output->payload.size();const auto seal=s.pending.empty()?
            s.session->publish_authority(*current,output->tick,std::move(output->payload),s.targets):
            s.session->publish_authority_ahead(*current,s.pending.back().seal,output->tick,std::move(output->payload),s.targets.front());
        s.pending.emplace_back(Impl::Pending{*current,seal,std::move(output->image),output->replay,output->phases,Clock::now(),output->paired,std::move(output->host),s.recipients,0});
        s.stats.validation_timed_out=false;
        if(s.stats.common_cut_barrier)s.stats.held_group=SourceAuthorityPublisherGroupCut{*current,seal,s.recipients};
        if(output->candidate)s.stats.snapshot_candidate_group=SourceAuthorityPublisherGroupCut{*current,seal,s.recipients};
        // Advance only the sent chain here. Promotion readiness and the count
        // of validated phases advance solely on the ordered native receipt.
        if(output->replay){s.replay->previous=seal;s.replay->first_sequence+=output->phases;}
        ++s.stats.publications;if(output->replay)++s.stats.replay_batches;
        s.stats.awaiting_validation=true;s.stats.payload_bytes=bytes;if(!output->replay)s.stats.full_bytes=output->full_bytes;
        s.stats.delta=output->delta;s.stats.error.clear();
    }catch(const std::exception &e){const std::string error=e.what();s.discard_stream();s.stats.error=error;return;}
    }
    if(!s.stats.snapshot_candidate_mode&&!s.stats.held_group&&s.replay&&s.pending.size()<(pipeline?2u:1u)&&s.available()&&!s.phases.empty())try{
        SourceReplayBatch batch{*s.replay,{}};std::uint64_t bytes{};
        std::optional<LanHost::Continuation> end_host;
        while(!s.phases.empty()&&batch.steps.size()<32){const auto size=s.phases.front().bytes;
            if(!batch.steps.empty()&&size>16ull*1024*1024-bytes)break;
            bytes+=size;s.queued_bytes-=size;end_host=std::move(s.phases.front().host);batch.steps.push_back(std::move(s.phases.front().step));s.phases.pop_front();}
        Impl::Job next;next.generation=s.generation;next.context=*current;next.replay=std::move(batch);next.host=std::move(end_host);
        {std::lock_guard lock(s.mutex);s.job=std::move(next);}s.condition.notify_one();
    }catch(const std::exception &e){const std::string error=e.what();s.discard_stream();s.stats.error=error;}
}
bool SourceAuthorityPublisher::capture(){
    auto &s=*impl_;s.own();update();if(s.stats.snapshot_candidate_mode||!s.active(s.context)||s.stats.held_group||!s.pending.empty()||!s.available())return false;
    try {
        const auto started=Clock::now();Impl::Job next;next.generation=s.generation;next.context=*s.context;
        if(s.host){need(s.stats.captures<UINT64_MAX);auto cut=capture_source_host_continuation(*s.host,*s.context,s.stats.captures+1);
            next.cut=std::move(cut.world);next.host=std::move(cut.host);next.serial=cut.serial;}
        else next.cut=s.authority.checkpoint(source_authority_binding(*s.context));
        s.stats.capture_microseconds=micros(started);next.base=s.base;
        s.replay.reset();s.phases.clear();s.queued_bytes=0;s.recorded_boundary.reset();
        if(s.mode==SourceAuthorityStreamMode::Replay)s.recorded_boundary=s.authority.execution_boundary();
        {std::lock_guard lock(s.mutex);s.job=std::move(next);}
        ++s.stats.captures;s.stats.error.clear();s.condition.notify_one();return true;
    }catch(const std::exception &e){s.stats.error=e.what();return false;}
}
bool SourceAuthorityPublisher::capture_candidate(const PeerSnapshotToken &token,const LanHost::CandidateEmission &emission){
    auto &s=*impl_;s.own();need(!s.frame_active);update();
    if(!s.active(s.context)||!s.candidate_active(token)||s.stats.snapshot_candidate||s.stats.held_group||
        !s.pending.empty()||!s.available()||!s.host->candidate_current(emission,s.session->snapshot_candidates())||emission.token()!=token)return false;
    try{
        need(s.stats.captures<UINT64_MAX);const auto started=Clock::now();
        auto cut=capture_source_host_continuation(*s.host,token.context,s.stats.captures+1);
        // A bare native step may not touch the host peer table. The captured
        // world itself, as well as the opaque peer stamps, must be exact.
        need(cut.world.match.simulation.ticks==emission.tick()&&s.host->candidate_current(emission,s.session->snapshot_candidates())&&s.candidate_active(token));
        Impl::Job next;next.generation=s.generation;next.context=token.context;next.cut=std::move(cut.world);
        next.host=std::move(cut.host);next.serial=cut.serial;next.candidate=token;
        s.stats.capture_microseconds=micros(started);
        s.replay.reset();s.phases.clear();s.queued_bytes=0;s.recorded_boundary.reset();
        s.stats.snapshot_candidate=token;s.stats.snapshot_candidate_group.reset();
        {std::lock_guard lock(s.mutex);s.job=std::move(next);}
        ++s.stats.captures;s.stats.error.clear();s.condition.notify_one();return true;
    }catch(const std::exception &e){s.stats.error=e.what();return false;}
}
bool SourceAuthorityPublisher::execute(const std::function<void()> &execute){
    impl_->own();need(bool(execute));auto frame=record_frame();execute();return frame&&frame->finish();
}
struct SourceAuthorityPublisher::Frame::Impl {
    SourceAuthorityPublisher::Impl &publisher;
    const std::uint64_t before;
    const int exceptions=std::uncaught_exceptions();
    SourceMatchAuthority::ExecutionRecording recording;
    explicit Impl(SourceAuthorityPublisher &p):publisher(*p.impl_),before(publisher.recorded_boundary->tick),
        recording(publisher.authority.start_execution_phase()){publisher.frame_active=true;}
};
SourceAuthorityPublisher::Frame::Frame(SourceAuthorityPublisher &publisher):impl_(std::make_unique<Impl>(publisher)){}
SourceAuthorityPublisher::Frame::~Frame(){finish();}
bool SourceAuthorityPublisher::Frame::finish()noexcept{
    if(!impl_)return false;
    auto &s=impl_->publisher;
    try{
        s.own();
        if(std::uncaught_exceptions()>impl_->exceptions){
            impl_.reset();s.frame_active=false;s.discard_stream();s.stats.error="Host frame interrupted";return false;
        }
        auto phase=impl_->recording.finish();s.append(impl_->before,std::move(phase));
        impl_.reset();s.frame_active=false;return true;
    }catch(const std::exception &e){
        impl_.reset();s.frame_active=false;
        try{s.discard_stream();s.stats.error=e.what();}catch(...){}
    }catch(...){
        impl_.reset();s.frame_active=false;
        try{s.discard_stream();s.stats.error="Native frame recording failed";}catch(...){}
    }
    return false;
}
std::unique_ptr<SourceAuthorityPublisher::Frame> SourceAuthorityPublisher::record_frame(){
    auto &s=*impl_;s.own();need(!s.frame_active);
    try{update();if(!s.recorded_boundary)return {};return std::unique_ptr<Frame>(new Frame(*this));}
    catch(const std::exception &e){try{s.discard_stream();s.stats.error=e.what();}catch(...){}return {};}
    catch(...){try{s.discard_stream();s.stats.error="Native frame recording could not start";}catch(...){}return {};}
}
}

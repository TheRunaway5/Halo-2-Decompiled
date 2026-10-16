#include "halo2/source_authority_publisher.hpp"
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <deque>

namespace halo2 {
namespace {
using Clock=std::chrono::steady_clock;
using ArchiveBytes=std::vector<std::byte>;
void need(bool ok){if(!ok)throw std::invalid_argument("Invalid source authority publisher ownership or endpoint");}
bool member(const AuthorityTransferContext &c,unsigned peer){return peer<16&&(c.peers&(1u<<peer));}
std::uint64_t micros(Clock::time_point started){return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-started).count();}
}
struct SourceAuthorityPublisher::Impl {
    SourceMatchAuthority &authority;const std::shared_ptr<OnlineMatchSession> session;const unsigned target;
    const SourceAuthorityStreamMode mode;
    const std::chrono::milliseconds timeout;const std::thread::id executor=std::this_thread::get_id();
    std::optional<AuthorityTransferContext> context;std::optional<AuthorityArchiveImage> base;
    SourceAuthorityPublisherState stats;
    struct Pending {AuthorityTransferContext context;AuthorityCutSeal seal;std::optional<AuthorityArchiveImage> image;
        std::optional<SourceReplayBinding> replay;std::uint64_t phases{};Clock::time_point started;};
    std::optional<Pending> pending;
    std::optional<SourceReplayBinding> replay;
    std::optional<SourceMatchAuthority::ExecutionBoundary> recorded_boundary;
    bool frame_active{};
    std::deque<SourceReplayStep> phases;std::uint64_t queued_bytes{};
    static constexpr std::uint64_t queue_limit=64ull*1024*1024;
    struct Job {std::uint64_t generation{};AuthorityTransferContext context;std::optional<SourceMatchAuthority::Checkpoint> cut;
        std::optional<SourceReplayBatch> replay;std::optional<AuthorityArchiveImage> base;};
    struct Result {std::uint64_t generation{},tick{},encoding_us{},full_bytes{};AuthorityTransferContext context;
        std::optional<AuthorityArchiveImage> image;std::optional<SourceReplayBinding> replay;std::uint64_t phases{};
        ArchiveBytes payload;bool delta{};std::string error;};
    mutable std::mutex mutex;std::condition_variable_any condition;
    std::uint64_t generation{};bool working{};std::optional<Job> job;std::optional<Result> result;
    // Join before destroying the captured value queue or any session borrower.
    std::jthread worker;
    Impl(SourceMatchAuthority &a,std::shared_ptr<OnlineMatchSession> s,unsigned t,SourceAuthorityStreamMode m,std::chrono::milliseconds wait)
        :authority(a),session(std::move(s)),target(t),mode(m),timeout(wait){
        need(session&&session->direct_udp()&&target<16&&target!=session->self()&&wait>=std::chrono::seconds(1)&&wait<=std::chrono::minutes(5));
        need(mode==SourceAuthorityStreamMode::Snapshots||mode==SourceAuthorityStreamMode::Replay);
        worker=std::jthread([this](std::stop_token stop){encode(stop);});
    }
    ~Impl(){worker.request_stop();condition.notify_all();if(worker.joinable())worker.join();}
    void own()const{need(std::this_thread::get_id()==executor);}
    void append(std::uint64_t before,SourceExecutionPhase phase){
        const auto after=authority.execution_boundary();
        const auto size=source_replay_phase_allocation(phase)+sizeof(SourceReplayStep);
        need(phases.size()<1024&&size<=queue_limit-queued_bytes);
        phases.push_back({before,after.tick,std::move(phase)});queued_bytes+=size;recorded_boundary=after;
        ++stats.recorded_phases;
    }
    void discard(){std::lock_guard lock(mutex);++generation;job.reset();result.reset();pending.reset();base.reset();replay.reset();
        phases.clear();queued_bytes=0;recorded_boundary.reset();stats.ready.reset();stats.awaiting_validation=false;}
    bool available()const{std::lock_guard lock(mutex);return !working&&!job&&!result;}
    bool active(const std::optional<AuthorityTransferContext> &c)const{
        return c&&c->host==session->self()&&member(*c,target)&&session->phase()==OnlineMatchSession::Phase::Running&&
            session->authority_supported(target)&&session->source_players_confirmed(target)&&
            (mode!=SourceAuthorityStreamMode::Replay||session->authority_replay_supported(target));
    }
    void encode(std::stop_token stop){
        while(!stop.stop_requested()){
            std::optional<Job> item;
            {std::unique_lock lock(mutex);condition.wait(lock,stop,[&]{return bool(job);});if(stop.stop_requested())return;
                item=std::move(job);job.reset();working=true;}
            Result out;out.generation=item->generation;out.context=item->context;const auto started=Clock::now();
            try {
                if(item->replay){
                    need(!item->cut&&item->replay->binding.context==item->context&&!item->replay->steps.empty());
                    out.replay=item->replay->binding;out.phases=item->replay->steps.size();out.tick=item->replay->steps.back().after_tick;
                    out.payload=encode_source_replay_batch(*item->replay);
                }else {
                need(item->cut.has_value());out.tick=item->cut->match.simulation.ticks;
                const auto binding=source_authority_binding(item->context);
                need(item->cut->match.admitted_binding==binding&&item->cut->match.simulation.admitted_binding==binding);
                SourceAuthorityCodecLimits limits;limits.encoded_bytes=64ull*1024*1024;
                out.payload=encode_source_authority(*item->cut,limits);out.full_bytes=out.payload.size();
                out.image=AuthorityArchiveImage::read(out.payload);
                if(item->base){auto delta=encode_authority_delta(*item->base,*out.image);
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
    SourceAuthorityStreamMode mode,std::chrono::milliseconds timeout):impl_(std::make_unique<Impl>(a,std::move(s),target,mode,timeout)){}
SourceAuthorityPublisher::~SourceAuthorityPublisher()=default;
void SourceAuthorityPublisher::cancel(){impl_->own();need(!impl_->frame_active);impl_->discard();impl_->context.reset();}
SourceAuthorityPublisherState SourceAuthorityPublisher::state()const{
    impl_->own();auto stats=impl_->stats;stats.queued_phases=impl_->phases.size();stats.queued_bytes=impl_->queued_bytes;
    stats.tracking_replay=impl_->recorded_boundary.has_value();std::lock_guard lock(impl_->mutex);stats.encoding=impl_->working||bool(impl_->job);return stats;
}
void SourceAuthorityPublisher::update(){
    auto &s=*impl_;s.own();need(!s.frame_active);const auto current=s.session->authority_context();
    if(current!=s.context){s.discard();s.context=current;}
    if(!s.active(current)){
        // A readiness loss invalidates this candidate, never room membership.
        s.discard();return;
    }
    if(s.recorded_boundary&&s.authority.execution_boundary()!=*s.recorded_boundary){
        s.discard();s.stats.error="Unrecorded native work between replay phases";return;
    }
    for(const auto &receipt:s.session->validated_authority())if(s.pending&&receipt.peer==s.target&&receipt.context==s.pending->context&&receipt.seal==s.pending->seal){
        if(s.pending->replay){need(s.replay&&*s.replay==*s.pending->replay);s.replay->previous=receipt.seal;
            s.replay->first_sequence+=s.pending->phases;s.stats.acknowledged_phases+=s.pending->phases;
        }else {need(s.pending->image.has_value());s.base=s.pending->image;
            if(s.mode==SourceAuthorityStreamMode::Replay)s.replay=SourceReplayBinding{receipt.context,receipt.seal,receipt.seal,1};}
        s.stats.ready=receipt;++s.stats.validations;s.pending.reset();s.stats.awaiting_validation=false;
    }
    if(s.pending&&Clock::now()-s.pending->started>s.timeout){s.discard();s.stats.error="Shadow executor validation timed out";return;}
    std::optional<Impl::Result> output;
    {std::lock_guard lock(s.mutex);output=std::move(s.result);s.result.reset();}
    if(output&&output->generation==s.generation&&output->context==current){
    s.stats.encoding_microseconds=output->encoding_us;
    if(!output->error.empty()){const auto error=output->error;s.discard();s.stats.error=error;return;}
    try {
        need(!s.pending&&(output->image||output->replay));
        if(output->replay)need(s.replay&&*s.replay==*output->replay);
        const std::array<unsigned,1> targets{s.target};
        const auto bytes=output->payload.size();const auto seal=s.session->publish_authority(*current,output->tick,std::move(output->payload),targets);
        s.pending.emplace(Impl::Pending{*current,seal,std::move(output->image),output->replay,output->phases,Clock::now()});
        ++s.stats.publications;if(output->replay)++s.stats.replay_batches;
        s.stats.awaiting_validation=true;s.stats.payload_bytes=bytes;if(!output->replay)s.stats.full_bytes=output->full_bytes;
        s.stats.delta=output->delta;s.stats.error.clear();
    }catch(const std::exception &e){const std::string error=e.what();s.discard();s.stats.error=error;return;}
    }
    if(s.replay&&!s.pending&&s.available()&&!s.phases.empty())try{
        SourceReplayBatch batch{*s.replay,{}};std::uint64_t bytes{};
        while(!s.phases.empty()&&batch.steps.size()<32){const auto size=source_replay_phase_allocation(s.phases.front().phase)+sizeof(SourceReplayStep);
            if(!batch.steps.empty()&&size>16ull*1024*1024-bytes)break;
            bytes+=size;s.queued_bytes-=size;batch.steps.push_back(std::move(s.phases.front()));s.phases.pop_front();}
        Impl::Job next;next.generation=s.generation;next.context=*current;next.replay=std::move(batch);
        {std::lock_guard lock(s.mutex);s.job=std::move(next);}s.condition.notify_one();
    }catch(const std::exception &e){const std::string error=e.what();s.discard();s.stats.error=error;}
}
bool SourceAuthorityPublisher::capture(){
    auto &s=*impl_;s.own();update();if(!s.active(s.context)||s.pending||!s.available())return false;
    try {
        const auto started=Clock::now();auto cut=s.authority.checkpoint(source_authority_binding(*s.context));s.stats.capture_microseconds=micros(started);
        Impl::Job next;next.generation=s.generation;next.context=*s.context;next.cut=std::move(cut);next.base=s.base;
        s.replay.reset();s.phases.clear();s.queued_bytes=0;s.recorded_boundary.reset();
        if(s.mode==SourceAuthorityStreamMode::Replay)s.recorded_boundary=s.authority.execution_boundary();
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
            impl_.reset();s.frame_active=false;s.discard();s.stats.error="Host frame interrupted";return false;
        }
        auto phase=impl_->recording.finish();s.append(impl_->before,std::move(phase));
        impl_.reset();s.frame_active=false;return true;
    }catch(const std::exception &e){
        impl_.reset();s.frame_active=false;
        try{s.discard();s.stats.error=e.what();}catch(...){}
    }catch(...){
        impl_.reset();s.frame_active=false;
        try{s.discard();s.stats.error="Native frame recording failed";}catch(...){}
    }
    return false;
}
std::unique_ptr<SourceAuthorityPublisher::Frame> SourceAuthorityPublisher::record_frame(){
    auto &s=*impl_;s.own();need(!s.frame_active);
    try{update();if(!s.recorded_boundary)return {};return std::unique_ptr<Frame>(new Frame(*this));}
    catch(const std::exception &e){try{s.discard();s.stats.error=e.what();}catch(...){}return {};}
    catch(...){try{s.discard();s.stats.error="Native frame recording could not start";}catch(...){}return {};}
}
}

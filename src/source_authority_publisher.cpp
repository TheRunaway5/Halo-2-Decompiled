#include "halo2/source_authority_publisher.hpp"
#include <openssl/sha.h>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace halo2 {
namespace {
using Clock=std::chrono::steady_clock;
using ArchiveBytes=std::vector<std::byte>;
void need(bool ok){if(!ok)throw std::invalid_argument("Invalid source authority publisher ownership or endpoint");}
void put(ArchiveBytes &p,std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)p.push_back(std::byte(value>>(8*i)));}
bool member(const AuthorityTransferContext &c,unsigned peer){return peer<16&&(c.peers&(1u<<peer));}
std::uint64_t micros(Clock::time_point started){return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-started).count();}
}
SourceMatchAuthority::AuthorityDigest source_authority_binding(const AuthorityTransferContext &c){
    need(member(c,c.host));ArchiveBytes input;for(char value:std::string_view("H2 SOURCE AUTHORITY 1"))input.push_back(std::byte(value));
    input.insert(input.end(),c.session.begin(),c.session.end());put(input,c.epoch,4);put(input,c.membership_revision,8);put(input,c.peers,2);put(input,c.host,1);
    SourceMatchAuthority::AuthorityDigest result{};
    need(SHA256(reinterpret_cast<const unsigned char *>(input.data()),input.size(),reinterpret_cast<unsigned char *>(result.data())));return result;
}
struct SourceAuthorityPublisher::Impl {
    SourceMatchAuthority &authority;const std::shared_ptr<OnlineMatchSession> session;const unsigned target;
    const std::chrono::milliseconds timeout;const std::thread::id executor=std::this_thread::get_id();
    std::optional<AuthorityTransferContext> context;std::optional<AuthorityArchiveImage> base;
    SourceAuthorityPublisherState stats;
    struct Pending {AuthorityTransferContext context;AuthorityCutSeal seal;AuthorityArchiveImage image;Clock::time_point started;};
    std::optional<Pending> pending;
    struct Job {std::uint64_t generation;AuthorityTransferContext context;SourceMatchAuthority::Checkpoint cut;std::optional<AuthorityArchiveImage> base;};
    struct Result {std::uint64_t generation{},tick{},encoding_us{},full_bytes{};AuthorityTransferContext context;
        std::optional<AuthorityArchiveImage> image;ArchiveBytes payload;bool delta{};std::string error;};
    mutable std::mutex mutex;std::condition_variable_any condition;
    std::uint64_t generation{};bool working{};std::optional<Job> job;std::optional<Result> result;
    // Join before destroying the captured value queue or any session borrower.
    std::jthread worker;
    Impl(SourceMatchAuthority &a,std::shared_ptr<OnlineMatchSession> s,unsigned t,std::chrono::milliseconds wait)
        :authority(a),session(std::move(s)),target(t),timeout(wait){
        need(session&&session->direct_udp()&&target<16&&target!=session->self()&&wait>=std::chrono::seconds(1)&&wait<=std::chrono::minutes(5));
        worker=std::jthread([this](std::stop_token stop){encode(stop);});
    }
    ~Impl(){worker.request_stop();condition.notify_all();if(worker.joinable())worker.join();}
    void own()const{need(std::this_thread::get_id()==executor);}
    void discard(){std::lock_guard lock(mutex);++generation;job.reset();result.reset();pending.reset();base.reset();stats.ready.reset();stats.awaiting_validation=false;}
    bool available()const{std::lock_guard lock(mutex);return !working&&!job&&!result;}
    bool active(const std::optional<AuthorityTransferContext> &c)const{
        return c&&c->host==session->self()&&member(*c,target)&&session->phase()==OnlineMatchSession::Phase::Running&&
            session->authority_supported(target)&&session->source_players_confirmed(target);
    }
    void encode(std::stop_token stop){
        while(!stop.stop_requested()){
            std::optional<Job> item;
            {std::unique_lock lock(mutex);condition.wait(lock,stop,[&]{return bool(job);});if(stop.stop_requested())return;
                item=std::move(job);job.reset();working=true;}
            Result out;out.generation=item->generation;out.context=item->context;out.tick=item->cut.match.simulation.ticks;const auto started=Clock::now();
            try {
                const auto binding=source_authority_binding(item->context);
                need(item->cut.match.admitted_binding==binding&&item->cut.match.simulation.admitted_binding==binding);
                SourceAuthorityCodecLimits limits;limits.encoded_bytes=64ull*1024*1024;
                out.payload=encode_source_authority(item->cut,limits);out.full_bytes=out.payload.size();
                out.image=AuthorityArchiveImage::read(out.payload);
                if(item->base){auto delta=encode_authority_delta(*item->base,*out.image);
                    if(delta.size()<out.payload.size()){out.payload=std::move(delta);out.delta=true;}}
            }catch(const std::exception &e){out.error=e.what();out.image.reset();out.payload.clear();}
            out.encoding_us=micros(started);
            {std::lock_guard lock(mutex);working=false;if(out.generation==generation&&!stop.stop_requested())result=std::move(out);}
        }
    }
};
SourceAuthorityPublisher::SourceAuthorityPublisher(SourceMatchAuthority &a,std::shared_ptr<OnlineMatchSession> s,unsigned target,std::chrono::milliseconds timeout)
    :impl_(std::make_unique<Impl>(a,std::move(s),target,timeout)){}
SourceAuthorityPublisher::~SourceAuthorityPublisher()=default;
void SourceAuthorityPublisher::cancel(){impl_->own();impl_->discard();impl_->context.reset();}
SourceAuthorityPublisherState SourceAuthorityPublisher::state()const{
    impl_->own();auto stats=impl_->stats;std::lock_guard lock(impl_->mutex);stats.encoding=impl_->working||bool(impl_->job);return stats;
}
void SourceAuthorityPublisher::update(){
    auto &s=*impl_;s.own();const auto current=s.session->authority_context();
    if(current!=s.context){s.discard();s.context=current;}
    if(!s.active(current)){
        // A readiness loss invalidates this candidate, never room membership.
        s.discard();return;
    }
    for(const auto &receipt:s.session->validated_authority())if(s.pending&&receipt.peer==s.target&&receipt.context==s.pending->context&&receipt.seal==s.pending->seal){
        s.base=s.pending->image;s.stats.ready=receipt;++s.stats.validations;s.pending.reset();s.stats.awaiting_validation=false;
    }
    if(s.pending&&Clock::now()-s.pending->started>s.timeout){s.pending.reset();s.stats.awaiting_validation=false;s.stats.error="Shadow executor validation timed out";}
    std::optional<Impl::Result> output;
    {std::lock_guard lock(s.mutex);output=std::move(s.result);s.result.reset();}
    if(!output)return;
    if(output->generation!=s.generation||output->context!=current)return;
    s.stats.encoding_microseconds=output->encoding_us;
    if(!output->error.empty()){s.stats.error=output->error;return;}
    try {
        need(!s.pending&&output->image.has_value());const std::array<unsigned,1> targets{s.target};
        const auto bytes=output->payload.size();const auto seal=s.session->publish_authority(*current,output->tick,std::move(output->payload),targets);
        s.pending.emplace(Impl::Pending{*current,seal,std::move(*output->image),Clock::now()});
        ++s.stats.publications;s.stats.awaiting_validation=true;s.stats.payload_bytes=bytes;s.stats.full_bytes=output->full_bytes;
        s.stats.delta=output->delta;s.stats.error.clear();
    }catch(const std::exception &e){s.stats.error=e.what();}
}
bool SourceAuthorityPublisher::capture(){
    auto &s=*impl_;s.own();update();if(!s.active(s.context)||s.pending||!s.available())return false;
    try {
        const auto started=Clock::now();auto cut=s.authority.checkpoint(source_authority_binding(*s.context));s.stats.capture_microseconds=micros(started);
        {std::lock_guard lock(s.mutex);s.job.emplace(Impl::Job{s.generation,*s.context,std::move(cut),s.base});}
        ++s.stats.captures;s.stats.error.clear();s.condition.notify_one();return true;
    }catch(const std::exception &e){s.stats.error=e.what();return false;}
}
}

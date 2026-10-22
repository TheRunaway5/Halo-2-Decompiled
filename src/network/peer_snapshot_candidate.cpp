#include "halo2/peer_snapshot_candidate.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <bit>
#include <limits>
#include <mutex>
#include <stdexcept>

namespace halo2 {
namespace {
using Bytes=std::vector<std::byte>;
constexpr unsigned kind=12,chunk_bytes=1024,header_bytes=67;
enum Type:unsigned {Arm,Armed,Offer,Chunk,Ack,Captured,Release,Abort,Released,Aborted,Reject,types};
void need(bool value){if(!value)throw std::invalid_argument("Invalid peer snapshot candidate");}
bool member(const AuthorityTransferContext &c,unsigned peer){return peer<16&&(c.peers&(1u<<peer));}
std::uint16_t bit(unsigned peer){return std::uint16_t(1u<<peer);}
unsigned chunks(std::uint64_t bytes){return unsigned((bytes+chunk_bytes-1)/chunk_bytes);}
void put(Bytes &b,std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)b.push_back(std::byte(value>>(8*i)));}
struct Reader {
    std::span<const std::byte> bytes;std::size_t at{};
    auto raw(std::size_t n){need(at<=bytes.size()&&n<=bytes.size()-at);auto p=bytes.subspan(at,n);at+=n;return p;}
    std::uint64_t get(unsigned n){std::uint64_t value{};const auto p=raw(n);for(unsigned i=0;i<n;++i)value|=std::uint64_t(std::to_integer<unsigned>(p[i]))<<(8*i);return value;}
    bool done()const{return at==bytes.size();}
};
AuthorityTransferDigest hash(std::span<const std::byte> bytes){AuthorityTransferDigest result{};
    need(SHA256(reinterpret_cast<const unsigned char *>(bytes.data()),bytes.size(),reinterpret_cast<unsigned char *>(result.data())));return result;}
Bytes header(const PeerSnapshotToken &t,unsigned target,unsigned type){
    Bytes b;b.reserve(header_bytes+4+chunk_bytes);put(b,0x324e3248,4);put(b,native_multiplayer_protocol,2);put(b,kind,1);put(b,1,1);put(b,type,1);
    b.insert(b.end(),t.context.session.begin(),t.context.session.end());put(b,t.context.epoch,4);put(b,t.context.membership_revision,8);
    put(b,t.context.peers,2);put(b,t.context.host,1);put(b,target,1);put(b,t.recipients,2);put(b,t.nonce,8);return b;
}
struct Message {unsigned type{},target{};PeerSnapshotToken token;};
Message decode(Reader &r){
    need(r.bytes.size()>=header_bytes&&r.bytes.size()<=1400);
    need(r.get(4)==0x324e3248&&r.get(2)==native_multiplayer_protocol&&r.get(1)==kind&&r.get(1)==1);
    Message m;m.type=unsigned(r.get(1));need(m.type<types);
    std::ranges::copy(r.raw(32),m.token.context.session.begin());m.token.context.epoch=std::uint32_t(r.get(4));
    m.token.context.membership_revision=r.get(8);m.token.context.peers=std::uint16_t(r.get(2));m.token.context.host=unsigned(r.get(1));
    m.target=unsigned(r.get(1));m.token.recipients=std::uint16_t(r.get(2));m.token.nonce=r.get(8);return m;
}
}
struct PeerSnapshotCandidate::Impl {
    const unsigned self;
    const AuthorityTransferContext binding;
    const std::uint16_t recipients;
    const PeerSnapshotLimits limits;
    const std::shared_ptr<const void> identity=std::make_shared<const unsigned char>(0);
    mutable std::mutex mutex;
    PeerSnapshotState value;
    std::uint64_t highest_nonce{},clock{},progress{},publishing_bytes{},local_bytes{};
    bool publishing{},checked_out{};
    unsigned cursor{};
    std::array<std::array<std::uint64_t,types>,16> emitted_at{};
    std::array<std::array<bool,types>,16> emitted{};
    struct Sender {
        std::unique_ptr<const Bytes> bytes;
        AuthorityTransferDigest digest{};
        std::uint64_t extent{};
        std::vector<unsigned char> sent,acked;
        std::vector<std::uint64_t> sent_at;
        unsigned begin{},scan{},sent_count{};
        bool offered{};
    };
    std::array<std::optional<Sender>,16> senders;
    struct Incoming {
        AuthorityTransferDigest digest{};
        std::uint64_t extent{};
        Bytes bytes;
        std::vector<unsigned char> received,dirty;
        unsigned count{},dirty_cursor{};
        bool hashing{},complete{},ack_all{};
    };
    std::optional<Incoming> incoming;
    std::unique_ptr<const Bytes> completed;
    Impl(unsigned s,AuthorityTransferContext c,PeerSnapshotLimits l):self(s),binding(c),
        recipients(c.host<16?std::uint16_t(c.peers&~bit(c.host)):0),limits(l){
        need(member(c,s)&&member(c,c.host)&&std::popcount(c.peers)>=2&&
            std::ranges::any_of(c.session,[](auto b){return b!=std::byte{};}));
        need(l.maximum_archive_bytes&&l.maximum_archive_bytes<=9ull*1024*1024&&l.retry_ms&&l.retry_ms<=60'000&&
            l.idle_ms>=l.retry_ms&&l.idle_ms<=86'400'000&&l.window_chunks&&l.window_chunks<=256);
        value.context=c;value.recipients=recipients;
        value.maximum_buffered_bytes=l.maximum_archive_bytes*(s==c.host?std::popcount(recipients):1);
    }
    bool recipient(unsigned p)const{return p<16&&(recipients&bit(p));}
    bool exact(const PeerSnapshotToken &t)const{return t.context==binding&&t.recipients==recipients&&t.nonce;}
    bool matches(const PeerSnapshotToken &t)const{return exact(t)&&value.pending&&*value.pending==t;}
    bool ended()const{return value.pending&&value.closing&&
        (self==binding.host?value.retired==recipients:bool(value.retired&bit(self)));}
    void observe(std::uint64_t now){clock=std::max(clock,now);
        value.stalled=value.pending&&!ended()&&clock-progress>=limits.idle_ms;}
    void changed(){progress=clock;value.stalled=false;}
    void install(const PeerSnapshotToken &t){
        const auto rejected_packets=value.rejected_packets,maximum=value.maximum_buffered_bytes;
        value={};value.context=binding;value.recipients=recipients;value.pending=t;
        value.rejected_packets=rejected_packets;value.maximum_buffered_bytes=maximum;
        highest_nonce=t.nonce;for(auto &s:senders)s.reset();incoming.reset();completed.reset();
        local_bytes=0;checked_out=false;emitted={};emitted_at={};changed();
    }
    bool close(PeerSnapshotClose mode){
        if(!value.pending||(mode!=PeerSnapshotClose::Release&&mode!=PeerSnapshotClose::Abort))return false;
        if(value.closing)return *value.closing==mode;
        if(mode==PeerSnapshotClose::Release&&value.captured!=recipients)return false;
        value.closing=mode;
        for(auto &s:senders)if(s)s->bytes.reset();
        if(incoming&&!incoming->hashing&&!checked_out){incoming.reset();completed.reset();local_bytes=0;}
        changed();return true;
    }
    std::uint64_t buffered()const{
        std::uint64_t result=local_bytes+publishing_bytes;
        for(const auto &s:senders)if(s&&s->bytes)result+=s->extent;
        return result;
    }
    bool due(unsigned peer,unsigned type)const{return !emitted[peer][type]||clock-emitted_at[peer][type]>=limits.retry_ms;}
    void mark(unsigned peer,unsigned type){emitted[peer][type]=true;emitted_at[peer][type]=clock;}
    bool make(unsigned peer,unsigned type,Bytes &out){
        if(!value.pending||peer==self||!member(binding,peer))return false;
        const bool host=self==binding.host;
        if(host){
            if(!recipient(peer))return false;
            // A recipient which lost every Arm still needs that exact token
            // to perform foreground retirement when the host aborts early.
            if(type==Arm){if((value.retired&bit(peer))||(value.armed&bit(peer))||!due(peer,type))return false;}
            else if(type==Release||type==Abort){
                if(!value.closing||(*value.closing==PeerSnapshotClose::Release)!=(type==Release)||
                    (value.retired&bit(peer))||!due(peer,type))return false;
            }else if(type==Offer){
                const auto &s=senders[peer];if(value.closing||!s||!s->bytes||(value.captured&bit(peer))||!due(peer,type))return false;
                out=header(*value.pending,peer,type);put(out,s->extent,8);out.insert(out.end(),s->digest.begin(),s->digest.end());
                put(out,s->sent.size(),4);senders[peer]->offered=true;mark(peer,type);return true;
            }else if(type==Chunk){
                auto &s=senders[peer];if(value.closing||!s||!s->bytes||!s->offered||(value.captured&bit(peer)))return false;
                const auto end=std::min<unsigned>(s->sent.size(),s->begin+limits.window_chunks);
                for(unsigned n=0;n<end-s->begin;++n){
                    if(s->scan<s->begin||s->scan>=end)s->scan=s->begin;const auto index=s->scan++;
                    if(s->acked[index]||(s->sent[index]&&clock-s->sent_at[index]<limits.retry_ms))continue;
                    out=header(*value.pending,peer,type);put(out,index,4);
                    const auto start=std::size_t(index)*chunk_bytes,extent=std::min<std::size_t>(chunk_bytes,s->extent-start);
                    out.insert(out.end(),s->bytes->begin()+start,s->bytes->begin()+start+extent);
                    if(!s->sent[index])++s->sent_count;s->sent[index]=1;s->sent_at[index]=clock;return true;
                }return false;
            }else return false;
        }else{
            if(peer!=binding.host)return false;
            const auto own=bit(self);
            if(type==Armed){if(value.closing||!(value.armed&own)||(value.rejected&own)||!due(peer,type))return false;}
            else if(type==Captured){if(value.closing||!(value.captured&own)||!incoming||!due(peer,type))return false;
                out=header(*value.pending,peer,type);put(out,incoming->extent,8);out.insert(out.end(),incoming->digest.begin(),incoming->digest.end());mark(peer,type);return true;
            }else if(type==Released||type==Aborted){
                if(!value.closing||!(value.retired&own)||(*value.closing==PeerSnapshotClose::Release)!=(type==Released)||!due(peer,type))return false;
            }else if(type==Reject){if(value.closing||!(value.rejected&own)||!due(peer,type))return false;}
            else if(type==Ack){
                if(value.closing||(value.rejected&own)||!incoming||incoming->hashing)return false;
                out=header(*value.pending,peer,type);
                if(incoming->ack_all){put(out,UINT32_MAX,4);put(out,0,8);incoming->ack_all=false;return true;}
                for(unsigned n=0;n<incoming->dirty.size();++n){const auto group=incoming->dirty_cursor++%incoming->dirty.size();
                    if(!incoming->dirty[group])continue;incoming->dirty[group]=0;const unsigned base=group*64;std::uint64_t bits{};
                    for(unsigned i=0;i<64&&base+i<incoming->received.size();++i)if(incoming->received[base+i])bits|=std::uint64_t{1}<<i;
                    put(out,base,4);put(out,bits,8);return true;
                }out.clear();return false;
            }else return false;
        }
        out=header(*value.pending,peer,type);mark(peer,type);return true;
    }
};

PeerSnapshotCandidate::Delivery::Delivery(PeerSnapshotToken t,unsigned peer,std::unique_ptr<const Bytes> bytes,std::shared_ptr<const void> identity)
    :token_(std::move(t)),recipient_(peer),bytes_(std::move(bytes)),identity_(std::move(identity)){}
PeerSnapshotCandidate::Delivery::Delivery(Delivery &&)noexcept=default;
PeerSnapshotCandidate::Delivery &PeerSnapshotCandidate::Delivery::operator=(Delivery &&)noexcept=default;
PeerSnapshotCandidate::Delivery::~Delivery()=default;
std::span<const std::byte> PeerSnapshotCandidate::Delivery::bytes()const noexcept{return bytes_?std::span<const std::byte>(*bytes_):std::span<const std::byte>{};}
PeerSnapshotCandidate::PeerSnapshotCandidate(unsigned self,AuthorityTransferContext context,PeerSnapshotLimits limits)
    :impl_(std::make_unique<Impl>(self,context,limits)){}
PeerSnapshotCandidate::~PeerSnapshotCandidate()=default;
unsigned PeerSnapshotCandidate::self()const noexcept{return impl_->self;}
std::optional<PeerSnapshotToken> PeerSnapshotCandidate::begin(std::uint64_t now){
    auto &s=*impl_;std::lock_guard lock(s.mutex);s.observe(now);
    if(s.self!=s.binding.host||s.publishing||(s.value.pending&&!s.ended())||s.highest_nonce==UINT64_MAX)return {};
    PeerSnapshotToken t{s.binding,s.recipients,s.highest_nonce+1};s.install(t);return t;
}
bool PeerSnapshotCandidate::acknowledge_arm(const PeerSnapshotToken &t){
    auto &s=*impl_;std::lock_guard lock(s.mutex);
    if(!s.recipient(s.self)||!s.matches(t)||s.value.closing||(s.value.rejected&bit(s.self)))return false;
    if(!(s.value.armed&bit(s.self))){s.value.armed|=bit(s.self);s.changed();}return true;
}
bool PeerSnapshotCandidate::publish(const PeerSnapshotToken &t,std::vector<PeerSnapshotArchive> archives,std::uint64_t now){
    auto &s=*impl_;std::uint16_t selected{};std::uint64_t extent{};
    for(const auto &a:archives){if(a.peer>=16||!s.recipient(a.peer)||(selected&bit(a.peer))||a.bytes.empty()||a.bytes.size()>s.limits.maximum_archive_bytes)return false;
        selected|=bit(a.peer);extent+=a.bytes.size();}
    if(selected!=s.recipients||archives.size()!=std::size_t(std::popcount(s.recipients)))return false;
    {std::lock_guard lock(s.mutex);s.observe(now);
        if(s.self!=s.binding.host||!s.matches(t)||s.value.closing||s.value.published||s.publishing||s.value.armed!=s.recipients)return false;
        s.publishing=true;s.publishing_bytes=extent;}
    std::array<std::optional<Impl::Sender>,16> prepared;
    try {
        for(auto &a:archives){Impl::Sender sender;sender.extent=a.bytes.size();sender.digest=hash(a.bytes);
            sender.bytes=std::make_unique<const Bytes>(std::move(a.bytes));const auto count=chunks(sender.extent);
            sender.sent.resize(count);sender.acked.resize(count);sender.sent_at.resize(count);prepared[a.peer]=std::move(sender);}
    }catch(...){
        // Keep the reservation until every partially prepared archive is gone.
        // Another thread must not admit a successor while these bytes retire.
        prepared={};archives.clear();std::lock_guard lock(s.mutex);s.publishing=false;s.publishing_bytes=0;throw;
    }
    std::unique_lock lock(s.mutex);
    if(!s.matches(t)||s.value.closing){
        lock.unlock();prepared={};archives.clear();lock.lock();s.publishing=false;s.publishing_bytes=0;return false;
    }
    s.publishing=false;s.publishing_bytes=0;
    s.senders=std::move(prepared);s.value.published=true;s.changed();return true;
}
std::optional<PeerSnapshotCandidate::Delivery> PeerSnapshotCandidate::take_delivery(){
    auto &s=*impl_;std::lock_guard lock(s.mutex);
    if(!s.value.pending||s.value.closing||s.checked_out||!s.completed)return {};
    Delivery result{*s.value.pending,s.self,std::move(s.completed),s.identity};s.checked_out=true;return std::optional<Delivery>(std::move(result));
}
bool PeerSnapshotCandidate::acknowledge_captured(Delivery &&delivery){
    std::unique_ptr<const Bytes> retired;
    auto &s=*impl_;{std::lock_guard lock(s.mutex);
        if(!s.recipient(s.self)||!s.matches(delivery.token_)||s.value.closing||(s.value.rejected&bit(s.self))||
            !s.checked_out||delivery.identity_!=s.identity||delivery.recipient_!=s.self||!delivery.bytes_||!s.incoming||!s.incoming->complete)return false;
        retired=std::move(delivery.bytes_);delivery.identity_.reset();retired.reset();s.checked_out=false;s.local_bytes=0;
        s.value.captured|=bit(s.self);s.changed();}return true;
}
bool PeerSnapshotCandidate::close(const PeerSnapshotToken &t,PeerSnapshotClose mode){
    auto &s=*impl_;std::lock_guard lock(s.mutex);return s.self==s.binding.host&&s.matches(t)&&s.close(mode);
}
bool PeerSnapshotCandidate::reject(const PeerSnapshotToken &t){
    auto &s=*impl_;std::lock_guard lock(s.mutex);
    if(!s.recipient(s.self)||!s.matches(t)||s.value.closing)return false;
    if(!(s.value.rejected&bit(s.self))){s.value.rejected|=bit(s.self);s.changed();}return true;
}
bool PeerSnapshotCandidate::can_acknowledge_retired_locked(const PeerSnapshotToken &t,const std::optional<Delivery> &delivery)const{
    const auto &s=*impl_;
    if(!s.recipient(s.self)||!s.matches(t)||!s.value.closing||(s.incoming&&s.incoming->hashing))return false;
    if(s.checked_out)
        return delivery&&delivery->identity_==s.identity&&delivery->recipient_==s.self&&delivery->token_==t&&bool(delivery->bytes_);
    return !delivery;
}
bool PeerSnapshotCandidate::can_acknowledge_retired(const PeerSnapshotToken &t,const std::optional<Delivery> &delivery)const{
    std::lock_guard lock(impl_->mutex);return can_acknowledge_retired_locked(t,delivery);
}
bool PeerSnapshotCandidate::acknowledge_retired(const PeerSnapshotToken &t,std::optional<Delivery> &&delivery){
    std::unique_ptr<const Bytes> retired;
    auto &s=*impl_;{std::lock_guard lock(s.mutex);
        if(!can_acknowledge_retired_locked(t,delivery))return false;
        if(delivery){retired=std::move(delivery->bytes_);delivery->identity_.reset();delivery.reset();retired.reset();}
        s.completed.reset();s.incoming.reset();s.checked_out=false;s.local_bytes=0;
        if(!(s.value.retired&bit(s.self))){s.value.retired|=bit(s.self);s.changed();}}return true;
}
PeerSnapshotState PeerSnapshotCandidate::state()const{
    const auto &s=*impl_;std::lock_guard lock(s.mutex);auto result=s.value;result.delivery_outstanding=s.checked_out;result.buffered_bytes=s.buffered();return result;
}
std::vector<AuthorityTransferPacket> PeerSnapshotCandidate::poll(std::uint64_t now,unsigned budget){
    auto &s=*impl_;std::lock_guard lock(s.mutex);s.observe(now);budget=std::min(budget,64u);
    std::vector<AuthorityTransferPacket> result;result.reserve(budget);
    while(result.size()<budget){bool progress{};
        for(unsigned scanned=0;scanned<16*types&&result.size()<budget;++scanned){const auto index=s.cursor++%(16*types);Bytes packet;
            if(s.make(index/types,index%types,packet)){result.push_back({index/types,std::move(packet)});progress=true;}}
        if(!progress)break;
    }return result;
}
bool PeerSnapshotCandidate::is_packet(std::span<const std::byte> p)noexcept{
    return p.size()>=7&&p[0]==std::byte{0x48}&&p[1]==std::byte{0x32}&&p[2]==std::byte{0x4e}&&p[3]==std::byte{0x32}&&p[6]==std::byte{kind};
}
bool PeerSnapshotCandidate::receive(unsigned peer,std::span<const std::byte> packet,std::uint64_t now){
    if(!is_packet(packet))return false;
    auto &s=*impl_;std::unique_ptr<const Bytes> finished;PeerSnapshotToken finished_token;AuthorityTransferDigest expected_digest{};
    try {
        Reader r{packet};const auto m=decode(r);
        {std::lock_guard lock(s.mutex);s.observe(now);
            need(peer!=s.self&&member(s.binding,peer)&&m.target==s.self&&s.exact(m.token));
            const bool host_control=m.type==Arm||m.type==Offer||m.type==Chunk||m.type==Release||m.type==Abort;
            need(host_control?peer==s.binding.host:s.self==s.binding.host&&s.recipient(peer));
            if(m.type==Arm){need(r.done());
                if(s.matches(m.token))return true;
                need(m.token.nonce>s.highest_nonce&&(!s.value.pending||s.ended())&&!s.checked_out);
                s.install(m.token);return true;}
            need(s.matches(m.token));
            if(m.type==Release||m.type==Abort){need(r.done());
                // Release certifies only the host's observed Captured matrix.
                // Local capture must have happened explicitly before it latches.
                if(m.type==Release){need((s.value.captured&bit(s.self))!=0);
                    if(!s.value.closing){s.value.closing=PeerSnapshotClose::Release;s.changed();}
                    else need(*s.value.closing==PeerSnapshotClose::Release);
                }else need(s.close(PeerSnapshotClose::Abort));return true;}
            if(m.type==Released||m.type==Aborted){need(r.done()&&s.value.closing&&
                (*s.value.closing==PeerSnapshotClose::Release)==(m.type==Released));
                if(!(s.value.retired&bit(peer))){s.value.retired|=bit(peer);s.changed();}return true;}
            if(m.type==Reject){need(r.done()&&(!s.value.closing||*s.value.closing==PeerSnapshotClose::Abort));
                s.value.rejected|=bit(peer);need(s.close(PeerSnapshotClose::Abort));return true;}
            need(!s.value.closing);
            if(m.type==Armed){need(r.done());if(!(s.value.armed&bit(peer))){s.value.armed|=bit(peer);s.changed();}}
            else if(m.type==Offer){
                const auto extent=r.get(8);AuthorityTransferDigest digest{};std::ranges::copy(r.raw(32),digest.begin());const auto count=r.get(4);
                need(r.done()&&(s.value.armed&bit(s.self))&&!(s.value.rejected&bit(s.self))&&extent&&extent<=s.limits.maximum_archive_bytes&&count==chunks(extent));
                if(s.incoming){need(s.incoming->extent==extent&&s.incoming->digest==digest);
                    if(s.incoming->complete)s.incoming->ack_all=true;return true;}
                Impl::Incoming incoming;incoming.extent=extent;incoming.digest=digest;incoming.bytes.resize(std::size_t(extent));
                incoming.received.resize(std::size_t(count));incoming.dirty.resize(std::size_t((count+63)/64));
                s.incoming=std::move(incoming);s.local_bytes=extent;s.value.published=true;s.changed();
            }else if(m.type==Chunk){
                need(s.value.armed&bit(s.self));if(!s.incoming||(s.value.rejected&bit(s.self)))return true;
                auto &incoming=*s.incoming;const auto index=r.get(4);need(index<incoming.received.size());
                const auto start=std::size_t(index)*chunk_bytes,extent=std::min<std::size_t>(chunk_bytes,incoming.extent-start);const auto data=r.raw(extent);need(r.done());
                if(incoming.hashing)return true;
                if(incoming.complete){incoming.ack_all=true;return true;}
                if(incoming.received[index])need(std::equal(data.begin(),data.end(),incoming.bytes.begin()+start));
                else {std::copy(data.begin(),data.end(),incoming.bytes.begin()+start);incoming.received[index]=1;++incoming.count;s.changed();}
                incoming.dirty[index/64]=1;
                if(incoming.count==incoming.received.size()){
                    finished=std::make_unique<const Bytes>(std::move(incoming.bytes));finished_token=m.token;expected_digest=incoming.digest;incoming.hashing=true;
                }
            }else if(m.type==Ack){
                auto &sender=s.senders[peer];const auto base=r.get(4),bits=r.get(8);need(r.done()&&sender);
                if(base==UINT32_MAX){need(!bits&&sender->sent_count==sender->sent.size());std::fill(sender->acked.begin(),sender->acked.end(),1);}
                else {need(base%64==0&&base<sender->sent.size());
                    for(unsigned i=0;i<64;++i)if(bits&(std::uint64_t{1}<<i))need(base+i<sender->sent.size()&&sender->sent[base+i]);
                    for(unsigned i=0;i<64&&base+i<sender->sent.size();++i)if(bits&(std::uint64_t{1}<<i))sender->acked[base+i]=1;}
                const auto before=sender->begin;while(sender->begin<sender->acked.size()&&sender->acked[sender->begin])++sender->begin;
                if(sender->begin!=before)s.changed();if(sender->begin==sender->acked.size())s.value.delivered|=bit(peer);
            }else if(m.type==Captured){
                const auto extent=r.get(8);AuthorityTransferDigest digest{};std::ranges::copy(r.raw(32),digest.begin());auto &sender=s.senders[peer];
                need(r.done()&&sender&&sender->extent==extent&&sender->digest==digest&&sender->sent_count==sender->sent.size());
                if(!(s.value.captured&bit(peer))){s.value.delivered|=bit(peer);s.value.captured|=bit(peer);sender->bytes.reset();s.changed();}
            }
        }
        if(finished){const auto actual=hash(*finished);std::lock_guard lock(s.mutex);
            if(!s.matches(finished_token)||!s.incoming||!s.incoming->hashing)return true;
            if(s.value.closing){finished.reset();s.incoming->hashing=false;s.local_bytes=0;return true;}
            if(actual!=expected_digest){finished.reset();s.incoming->hashing=false;s.local_bytes=0;s.value.rejected|=bit(s.self);s.changed();return true;}
            s.incoming->hashing=false;
            s.incoming->complete=true;s.incoming->ack_all=true;std::ranges::fill(s.incoming->dirty,0);
            s.completed=std::move(finished);s.value.delivered|=bit(s.self);s.changed();
        }
    }catch(const std::exception &){std::lock_guard lock(s.mutex);
        if(finished&&s.matches(finished_token)&&s.incoming&&s.incoming->hashing){
            finished.reset();s.incoming->hashing=false;s.local_bytes=0;s.value.rejected|=bit(s.self);s.changed();
        }
        if(s.value.rejected_packets!=UINT64_MAX)++s.value.rejected_packets;}
    return true;
}
}

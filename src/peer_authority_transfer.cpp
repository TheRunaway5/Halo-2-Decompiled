#include "halo2/peer_authority_transfer.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <bit>
#include <deque>
#include <mutex>
#include <set>
#include <stdexcept>

namespace halo2 {
namespace {
using Bytes=std::vector<std::byte>;
constexpr unsigned chunk_bytes=1024,header_bytes=65;
enum Type:unsigned {Hello,HelloAck,Offer,Chunk,Ack,Ready,Reject};
void need(bool ok){if(!ok)throw std::invalid_argument("Invalid peer authority transfer");}
void put(Bytes &p,std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)p.push_back(std::byte(value>>(8*i)));}
struct Reader {
    std::span<const std::byte> bytes;std::size_t at{};
    std::span<const std::byte> raw(std::size_t n){need(n<=bytes.size()-at);auto p=bytes.subspan(at,n);at+=n;return p;}
    std::uint64_t get(unsigned n){std::uint64_t value{};auto p=raw(n);for(unsigned i=0;i<n;++i)value|=std::to_integer<std::uint64_t>(p[i])<<(8*i);return value;}
    bool done()const{return at==bytes.size();}
};
AuthorityTransferDigest hash(std::span<const std::byte> value){AuthorityTransferDigest d{};
    need(SHA256(reinterpret_cast<const unsigned char *>(value.data()),value.size(),reinterpret_cast<unsigned char *>(d.data())));return d;}
bool member(const AuthorityTransferContext &c,unsigned peer){return peer<16&&(c.peers&(1u<<peer));}
void valid(unsigned self,const AuthorityTransferContext &c){
    need(member(c,self)&&member(c,c.host)&&std::popcount(c.peers)>=1&&
        std::any_of(c.session.begin(),c.session.end(),[](auto b){return b!=std::byte{};}));
}
std::uint64_t elapsed(std::uint64_t now,std::uint64_t then){return now>=then?now-then:0;}
unsigned chunks(std::uint64_t bytes){return unsigned((bytes+chunk_bytes-1)/chunk_bytes);}
Bytes header(const AuthorityTransferContext &c,unsigned target,Type type,std::uint64_t id){
    Bytes p;p.reserve(1100);put(p,0x324e3248,4);put(p,native_multiplayer_protocol,2);put(p,7,1);put(p,1,1);put(p,type,1);
    p.insert(p.end(),c.session.begin(),c.session.end());put(p,c.epoch,4);put(p,c.membership_revision,8);put(p,c.peers,2);
    put(p,c.host,1);put(p,target,1);put(p,id,8);return p;
}
void put_seal(Bytes &p,const AuthorityCutSeal &s){put(p,s.tick,8);put(p,s.bytes,8);p.insert(p.end(),s.digest.begin(),s.digest.end());}
AuthorityCutSeal get_seal(Reader &r,std::uint64_t id){AuthorityCutSeal s{id,r.get(8),r.get(8),{}};auto p=r.raw(32);std::copy(p.begin(),p.end(),s.digest.begin());return s;}
}
struct PeerAuthorityTransfer::Impl {
    unsigned self;AuthorityTransferContext binding;const AuthorityTransferLimits limits;
    mutable std::mutex mutex;
    std::array<std::uint64_t,16> capabilities{},hello_at{};
    std::array<std::uint32_t,16> features{};
    std::array<bool,16> hello_sent{},hello_reply{};
    std::uint64_t next_id=1,last_tick{};
    struct Sender {
        AuthorityCutSeal seal;std::shared_ptr<const Bytes> bytes;
        std::vector<std::uint8_t> sent,acked;std::vector<std::uint64_t> sent_at;
        unsigned begin{},scan{},sent_count{};
        std::uint64_t started{},progress{},offered_at{};
        bool offered{},validated{},failed{};
    };
    std::array<std::optional<Sender>,16> senders;
    struct Incoming {
        AuthorityCutSeal seal;Bytes bytes;std::vector<std::uint8_t> received;
        std::set<unsigned> dirty;unsigned count{};
        std::uint64_t started{},progress{};
        bool hashing{},complete{},ack_all{};
    };
    std::optional<Incoming> incoming;
    std::uint64_t highest_id{},highest_tick{},offer_at{};bool accepted_offer{};
    std::optional<ReceivedAuthorityCut> completed;
    std::optional<AuthorityCutSeal> delivered,validated;
    bool ready_sent{};std::uint64_t ready_at{};
    std::optional<std::uint64_t> rejected;
    std::deque<ValidatedAuthorityCut> receipts;
    unsigned cursor{};double credit{};std::uint64_t credit_at{};bool credit_started{};
    Impl(unsigned s,AuthorityTransferContext c,AuthorityTransferLimits l):self(s),binding(c),limits(l),credit(double(l.burst_bytes)){
        valid(s,c);need(l.maximum_bytes&&l.maximum_bytes<=512ull*1024*1024&&l.bytes_per_second&&
            l.bytes_per_second<=1024ull*1024*1024&&l.burst_bytes>=1100&&l.burst_bytes<=1024*1024&&
            l.retry_ms>=10&&l.offer_ms>=l.retry_ms&&l.idle_ms>=l.offer_ms&&l.lifetime_ms>=l.idle_ms&&
            l.window_chunks&&l.window_chunks<=256);
    }
    void reset(AuthorityTransferContext c){binding=c;capabilities.fill(0);features.fill(0);hello_sent.fill(false);hello_reply.fill(false);
        for(auto &s:senders)s.reset();incoming.reset();completed.reset();delivered.reset();validated.reset();rejected.reset();receipts.clear();
        next_id=1;last_tick=highest_id=highest_tick=0;accepted_offer=ready_sent=credit_started=false;credit=double(limits.burst_bytes);}
    void expire(std::uint64_t now){
        if(incoming&&!incoming->complete&&!incoming->hashing&&
            (elapsed(now,incoming->progress)>limits.idle_ms||elapsed(now,incoming->started)>limits.lifetime_ms))incoming.reset();
        for(auto &s:senders)if(s&&!s->validated&&!s->failed&&
            (elapsed(now,s->progress)>limits.idle_ms||elapsed(now,s->started)>limits.lifetime_ms)){s->failed=true;s->bytes.reset();}
    }
};
PeerAuthorityTransfer::PeerAuthorityTransfer(unsigned self,AuthorityTransferContext c,AuthorityTransferLimits limits)
    :impl_(std::make_unique<Impl>(self,c,limits)){}
PeerAuthorityTransfer::~PeerAuthorityTransfer()=default;
AuthorityTransferContext PeerAuthorityTransfer::context()const{std::lock_guard lock(impl_->mutex);return impl_->binding;}
bool PeerAuthorityTransfer::replace_context(const AuthorityTransferContext &expected,AuthorityTransferContext next){
    valid(impl_->self,next);std::lock_guard lock(impl_->mutex);
    if(impl_->binding!=expected||next.session!=expected.session||next.epoch<expected.epoch||
        (next.epoch==expected.epoch&&(next.host!=expected.host||next.membership_revision<=expected.membership_revision)))return false;
    impl_->reset(next);return true;
}
bool PeerAuthorityTransfer::supported(unsigned peer)const{std::lock_guard lock(impl_->mutex);return peer<16&&impl_->capabilities[peer]!=0;}
bool PeerAuthorityTransfer::feature_supported(unsigned peer,std::uint32_t feature)const{
    std::lock_guard lock(impl_->mutex);return peer<16&&feature&&impl_->capabilities[peer]&&
        (impl_->features[peer]&feature)==feature;
}
AuthorityCutSeal PeerAuthorityTransfer::publish(const AuthorityTransferContext &expected,std::uint64_t tick,
    Bytes archive,std::span<const unsigned> targets,std::uint64_t now){
    need(!archive.empty()&&archive.size()<=impl_->limits.maximum_bytes&&!targets.empty()&&targets.size()<=15);
    const auto digest=hash(archive);auto owned=std::make_shared<const Bytes>(std::move(archive));
    std::lock_guard lock(impl_->mutex);auto &s=*impl_;
    need(s.binding==expected&&s.self==s.binding.host&&tick>=s.last_tick&&s.next_id!=UINT64_MAX);
    std::uint16_t selected=0;
    for(auto peer:targets){need(peer!=s.self&&member(s.binding,peer)&&s.capabilities[peer]>=owned->size()&&!(selected&(1u<<peer)));selected|=std::uint16_t(1u<<peer);}
    AuthorityCutSeal seal{s.next_id++,tick,owned->size(),digest};s.last_tick=tick;
    for(auto &sender:s.senders)sender.reset();
    for(auto peer:targets){Impl::Sender sender;sender.seal=seal;sender.bytes=owned;const auto count=chunks(seal.bytes);
        sender.sent.resize(count);sender.acked.resize(count);sender.sent_at.resize(count);sender.started=sender.progress=now;s.senders[peer]=std::move(sender);}
    return seal;
}
bool PeerAuthorityTransfer::is_packet(std::span<const std::byte> bytes){
    if(bytes.size()<7)return false;Reader r{bytes};return r.get(4)==0x324e3248&&r.get(2)>0&&r.get(1)==7;
}
std::vector<AuthorityTransferPacket> PeerAuthorityTransfer::poll(std::uint64_t now,unsigned budget){
    std::lock_guard lock(impl_->mutex);auto &s=*impl_;s.expire(now);budget=std::min(budget,64u);
    if(s.credit_started)s.credit=std::min(double(s.limits.burst_bytes),s.credit+double(std::min<std::uint64_t>(elapsed(now,s.credit_at),1000))*double(s.limits.bytes_per_second)/1000.0);
    s.credit_started=true;s.credit_at=now;
    std::vector<AuthorityTransferPacket> result;
    auto emit=[&](unsigned peer,Bytes p){result.push_back({peer,std::move(p)});};
    while(result.size()<budget){bool progress=false;
        for(unsigned attempt=0;attempt<16&&result.size()<budget;++attempt){const unsigned peer=s.cursor++%16;
            if(peer==s.self||!member(s.binding,peer))continue;
            if(s.hello_reply[peer]){auto p=header(s.binding,peer,HelloAck,0);put(p,s.limits.maximum_bytes,8);if(s.limits.features)put(p,s.limits.features,4);emit(peer,std::move(p));s.hello_reply[peer]=false;progress=true;continue;}
            if(!s.capabilities[peer]&&(!s.hello_sent[peer]||elapsed(now,s.hello_at[peer])>=1000)){
                auto p=header(s.binding,peer,Hello,0);put(p,s.limits.maximum_bytes,8);if(s.limits.features)put(p,s.limits.features,4);emit(peer,std::move(p));s.hello_sent[peer]=true;s.hello_at[peer]=now;progress=true;continue;}
            if(peer==s.binding.host&&s.self!=peer){
                if(s.rejected){auto p=header(s.binding,peer,Reject,*s.rejected);emit(peer,std::move(p));s.rejected.reset();progress=true;continue;}
                if(s.incoming&&s.incoming->ack_all){auto p=header(s.binding,peer,Ack,s.incoming->seal.id);put(p,UINT32_MAX,4);put(p,0,8);
                    emit(peer,std::move(p));s.incoming->ack_all=false;progress=true;continue;}
                if(s.incoming&&!s.incoming->dirty.empty()){
                    auto &incoming=*s.incoming;const auto base=*incoming.dirty.begin();incoming.dirty.erase(incoming.dirty.begin());std::uint64_t bits{};
                    for(unsigned i=0;i<64&&base+i<incoming.received.size();++i)if(incoming.received[base+i])bits|=std::uint64_t{1}<<i;
                    auto p=header(s.binding,peer,Ack,incoming.seal.id);put(p,base,4);put(p,bits,8);emit(peer,std::move(p));progress=true;continue;
                }
                if(s.validated&&(!s.ready_sent||elapsed(now,s.ready_at)>=s.limits.offer_ms)){
                    auto p=header(s.binding,peer,Ready,s.validated->id);put_seal(p,*s.validated);emit(peer,std::move(p));s.ready_sent=true;s.ready_at=now;progress=true;continue;}
            }
            auto &sender=s.senders[peer];if(!sender||sender->failed||sender->validated)continue;
            if(!sender->offered||elapsed(now,sender->offered_at)>=s.limits.offer_ms){auto p=header(s.binding,peer,Offer,sender->seal.id);put_seal(p,sender->seal);
                put(p,sender->sent.size(),4);emit(peer,std::move(p));sender->offered=true;sender->offered_at=now;progress=true;continue;}
            const auto end=std::min<unsigned>(sender->sent.size(),sender->begin+s.limits.window_chunks);
            for(unsigned tried=0;tried<end-sender->begin;++tried){
                if(sender->scan<sender->begin||sender->scan>=end)sender->scan=sender->begin;const auto index=sender->scan++;
                if(sender->acked[index]||(sender->sent[index]&&elapsed(now,sender->sent_at[index])<s.limits.retry_ms))continue;
                const auto start=std::uint64_t(index)*chunk_bytes;const auto n=std::min<std::uint64_t>(chunk_bytes,sender->seal.bytes-start);
                if(s.credit<double(header_bytes+4+n))break;
                auto p=header(s.binding,peer,Chunk,sender->seal.id);put(p,index,4);p.insert(p.end(),sender->bytes->begin()+start,sender->bytes->begin()+start+n);
                s.credit-=p.size();emit(peer,std::move(p));if(!sender->sent[index])++sender->sent_count;sender->sent[index]=1;sender->sent_at[index]=now;progress=true;break;
            }
        }
        if(!progress)break;
    }
    return result;
}
bool PeerAuthorityTransfer::receive(unsigned peer,std::span<const std::byte> bytes,std::uint64_t now){
    if(!is_packet(bytes))return false;
    std::shared_ptr<const Bytes> finished;AuthorityTransferContext finished_context;AuthorityCutSeal finished_seal;
    try {
        need(bytes.size()>=header_bytes&&bytes.size()<=1400);Reader r{bytes};
        need(r.get(4)==0x324e3248&&r.get(2)==native_multiplayer_protocol&&r.get(1)==7&&r.get(1)==1);
        const auto type=r.get(1);need(type<=Reject);AuthorityTransferContext context;
        auto digest=r.raw(32);std::copy(digest.begin(),digest.end(),context.session.begin());
        context.epoch=std::uint32_t(r.get(4));context.membership_revision=r.get(8);context.peers=std::uint16_t(r.get(2));
        context.host=unsigned(r.get(1));const auto target=r.get(1),id=r.get(8);
        {
            std::lock_guard lock(impl_->mutex);auto &s=*impl_;
            need(context==s.binding&&target==s.self&&peer!=s.self&&member(context,peer));s.expire(now);
            if(type==Hello||type==HelloAck){const auto maximum=r.get(8);const auto features=r.done()?0u:std::uint32_t(r.get(4));
                need(!id&&r.done()&&maximum&&maximum<=512ull*1024*1024);
                s.capabilities[peer]=std::min(maximum,s.limits.maximum_bytes);s.features[peer]=features;
                if(type==Hello)s.hello_reply[peer]=true;return true;}
            need(id&&s.capabilities[peer]);
            if(type==Offer){
                need(peer==context.host&&s.self!=context.host);const auto seal=get_seal(r,id);const auto count=r.get(4);
                need(r.done()&&seal.bytes&&seal.bytes<=s.limits.maximum_bytes&&count==chunks(seal.bytes));
                if(s.incoming&&s.incoming->seal.id==id){need(s.incoming->seal==seal);
                    if(s.incoming->complete)s.incoming->ack_all=true;return true;}
                if(id<=s.highest_id||seal.tick<s.highest_tick||(s.accepted_offer&&elapsed(now,s.offer_at)<250))return true;
                Impl::Incoming incoming;incoming.seal=seal;incoming.bytes.resize(std::size_t(seal.bytes));incoming.received.resize(std::size_t(count));
                incoming.started=incoming.progress=now;s.incoming=std::move(incoming);s.highest_id=id;s.highest_tick=seal.tick;s.offer_at=now;s.accepted_offer=true;
            }else if(type==Chunk){
                need(peer==context.host&&s.self!=context.host);if(!s.incoming||s.incoming->seal.id!=id)return true;
                auto &incoming=*s.incoming;const auto index=r.get(4);need(index<incoming.received.size());
                const auto start=index*chunk_bytes,n=std::min<std::uint64_t>(chunk_bytes,incoming.seal.bytes-start);const auto data=r.raw(std::size_t(n));need(r.done());
                if(incoming.complete){incoming.ack_all=true;return true;}if(incoming.hashing)return true;
                if(incoming.received[index]){need(std::equal(data.begin(),data.end(),incoming.bytes.begin()+start));}
                else {std::copy(data.begin(),data.end(),incoming.bytes.begin()+start);incoming.received[index]=1;++incoming.count;incoming.progress=now;}
                incoming.dirty.insert(unsigned(index/64)*64);
                if(incoming.count==incoming.received.size()){
                    incoming.hashing=true;finished=std::make_shared<const Bytes>(std::move(incoming.bytes));finished_context=context;finished_seal=incoming.seal;
                }
            }else if(type==Ack){
                need(s.self==context.host);auto &sender=s.senders[peer];if(!sender||sender->seal.id!=id)return true;
                const auto base=r.get(4),bits=r.get(8);need(r.done());
                if(base==UINT32_MAX){need(!bits&&sender->sent_count==sender->sent.size());std::fill(sender->acked.begin(),sender->acked.end(),1);}
                else {need(base%64==0&&base<sender->sent.size());
                    for(unsigned i=0;i<64;++i)if(bits&(std::uint64_t{1}<<i))need(base+i<sender->sent.size()&&sender->sent[base+i]);
                    for(unsigned i=0;i<64&&base+i<sender->sent.size();++i)if(bits&(std::uint64_t{1}<<i))sender->acked[base+i]=1;
                }
                const auto before=sender->begin;while(sender->begin<sender->acked.size()&&sender->acked[sender->begin])++sender->begin;
                if(sender->begin!=before)sender->progress=now;
            }else if(type==Ready){
                need(s.self==context.host);const auto seal=get_seal(r,id);need(r.done());auto &sender=s.senders[peer];
                if(!sender||sender->seal.id!=id)return true;need(sender->seal==seal&&sender->sent_count==sender->sent.size());
                if(!sender->validated&&!sender->failed){sender->validated=true;sender->bytes.reset();if(s.receipts.size()==32)s.receipts.pop_front();s.receipts.push_back({peer,context,seal});}
            }else if(type==Reject){need(s.self==context.host&&r.done());auto &sender=s.senders[peer];
                if(sender&&sender->seal.id==id){sender->failed=true;sender->bytes.reset();}}
        }
        if(finished){const auto actual=hash(*finished);std::lock_guard lock(impl_->mutex);auto &s=*impl_;
            if(s.binding!=finished_context||!s.incoming||s.incoming->seal!=finished_seal)return true;
            if(actual!=finished_seal.digest){s.rejected=finished_seal.id;s.incoming.reset();return true;}
            s.incoming->complete=true;s.incoming->hashing=false;s.incoming->ack_all=true;s.incoming->dirty.clear();
            s.completed=ReceivedAuthorityCut{peer,finished_context,finished_seal,std::move(finished)};
        }
    }catch(const std::exception &){} // A malformed peer packet cannot end the room.
    return true;
}
std::optional<ReceivedAuthorityCut> PeerAuthorityTransfer::take_completed(){
    std::lock_guard lock(impl_->mutex);auto &s=*impl_;auto value=std::move(s.completed);s.completed.reset();if(value)s.delivered=value->seal;return value;
}
bool PeerAuthorityTransfer::acknowledge_validated(const AuthorityTransferContext &context,const AuthorityCutSeal &seal){
    std::lock_guard lock(impl_->mutex);auto &s=*impl_;
    if(s.binding!=context||s.self==context.host||!s.delivered||*s.delivered!=seal||(s.validated&&s.validated->id>seal.id))return false;
    s.validated=seal;s.ready_sent=false;return true;
}
std::vector<ValidatedAuthorityCut> PeerAuthorityTransfer::validated_cuts(){
    std::lock_guard lock(impl_->mutex);std::vector<ValidatedAuthorityCut> result(impl_->receipts.begin(),impl_->receipts.end());impl_->receipts.clear();return result;
}
}

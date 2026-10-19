#include "halo2/peer_common_cut.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include <algorithm>
#include <bit>
#include <mutex>
#include <stdexcept>

namespace halo2 {
namespace {
using Bytes=std::vector<std::byte>;
enum Type:unsigned {Offer,Associated,Commit,Applied,Release,Released,Reject,Abort,Aborted,types};
constexpr unsigned kind=11,packet_bytes=116;
constexpr std::uint64_t retry_ms=100;
void need(bool value){if(!value)throw std::invalid_argument("Invalid peer common-cut association or packet");}
bool member(const AuthorityTransferContext &c,unsigned peer){return peer<16&&(c.peers&(1u<<peer));}
template<class T>bool nonzero(const T &v){return std::ranges::any_of(v,[](auto b){return b!=std::byte{};});}
bool valid(const AuthorityCutSeal &seal){return seal.id&&seal.bytes&&seal.bytes<=64ull*1024*1024&&nonzero(seal.digest);}
void put(Bytes &out,std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)out.push_back(std::byte(value>>(8*i)));}
struct Reader {
    std::span<const std::byte> bytes;std::size_t at{};
    auto raw(unsigned n){need(at<=bytes.size()&&n<=bytes.size()-at);auto out=bytes.subspan(at,n);at+=n;return out;}
    std::uint64_t get(unsigned n){std::uint64_t value{};auto in=raw(n);for(unsigned i=0;i<n;++i)value|=std::uint64_t(std::to_integer<unsigned>(in[i]))<<(8*i);return value;}
};
void encode_context(Bytes &b,const AuthorityTransferContext &c){b.insert(b.end(),c.session.begin(),c.session.end());
    put(b,c.epoch,4);put(b,c.membership_revision,8);put(b,c.peers,2);put(b,c.host,1);}
auto decode_context(Reader &r){AuthorityTransferContext c;std::ranges::copy(r.raw(32),c.session.begin());
    c.epoch=std::uint32_t(r.get(4));c.membership_revision=r.get(8);c.peers=std::uint16_t(r.get(2));c.host=unsigned(r.get(1));return c;}
void encode_cut(Bytes &b,const PeerCommonCut &cut){put(b,cut.recipients,2);put(b,cut.seal.id,8);put(b,cut.seal.tick,8);
    put(b,cut.seal.bytes,8);b.insert(b.end(),cut.seal.digest.begin(),cut.seal.digest.end());}
struct Message {unsigned type{},sender{},target{};PeerCommonCut cut;};
auto decode(std::span<const std::byte> packet){need(packet.size()==packet_bytes);Reader r{packet};
    need(r.get(4)==0x324e3248&&r.get(2)==native_multiplayer_protocol&&r.get(1)==kind&&r.get(1)==1);
    Message m;m.type=unsigned(r.get(1));need(m.type<types);m.cut.context=decode_context(r);
    m.sender=unsigned(r.get(1));m.target=unsigned(r.get(1));m.cut.recipients=std::uint16_t(r.get(2));
    m.cut.seal.id=r.get(8);m.cut.seal.tick=r.get(8);m.cut.seal.bytes=r.get(8);
    std::ranges::copy(r.raw(32),m.cut.seal.digest.begin());need(r.at==packet.size());return m;}
}
struct PeerCommonCutAgreement::Impl {
    const unsigned self;
    const AuthorityTransferContext binding;
    const std::uint16_t recipients;
    mutable std::mutex mutex;
    PeerCommonCutState value;
    bool local_rejected{},dirty=true,emitted{};
    std::uint64_t clock{},emitted_at{},highest_id{},highest_tick{};
    std::array<std::array<std::optional<Bytes>,types>,16> packets;
    unsigned cursor{};
    Impl(unsigned s,AuthorityTransferContext c):self(s),binding(c),
        recipients(c.host<16?std::uint16_t(c.peers&~(1u<<c.host)):0){
        need(member(c,s)&&member(c,c.host)&&nonzero(c.session)&&std::popcount(c.peers)>=2);
        value.context=c;value.recipients=recipients;
    }
    bool recipient(unsigned peer)const{return peer<16&&(recipients&(1u<<peer));}
    bool exact(const PeerCommonCut &cut)const{return cut.context==binding&&cut.recipients==recipients&&valid(cut.seal);}
    bool matches(const PeerCommonCut &cut)const{return exact(cut)&&value.pending&&*value.pending==cut;}
    std::uint16_t bit(unsigned peer)const{return std::uint16_t(1u<<peer);}
    bool ended()const {
        if(!value.pending)return true;
        if(self==binding.host)return value.abort?value.aborted==recipients:value.release&&value.released==recipients;
        return value.abort?bool(value.aborted&bit(self)):value.release&&bool(value.released&bit(self));
    }
    void progress(){
        if(self!=binding.host||!value.pending||value.abort)return;
        if(!value.commit&&value.native_all_validated&&value.associated==recipients){value.commit=true;dirty=true;}
        if(value.commit&&!value.release&&value.applied==recipients){value.release=true;dirty=true;}
        if(value.release&&value.released==recipients)value.committed=value.pending;
    }
    bool offer(const PeerCommonCut &cut){
        if(!exact(cut))return false;
        if(value.pending&&*value.pending==cut)return true;
        if(!ended()||cut.seal.id<=highest_id||cut.seal.tick<highest_tick)return false;
        highest_id=cut.seal.id;highest_tick=cut.seal.tick;
        value.pending=cut;value.associated=value.applied=value.released=value.aborted=0;
        value.native_all_validated=value.commit=value.release=value.abort=false;
        local_rejected=false;for(auto &peer:packets)for(auto &packet:peer)packet.reset();dirty=true;return true;
    }
    bool abort(const PeerCommonCut &cut){
        if(!matches(cut)||value.commit||value.release)return false;
        if(value.abort)return true;
        value.abort=true;for(auto &peer:packets)for(auto &packet:peer)packet.reset();dirty=true;return true;
    }
    void send(unsigned target,unsigned type){
        if(target==self||!member(binding,target)||!value.pending)return;
        Bytes b;b.reserve(packet_bytes);put(b,0x324e3248,4);put(b,native_multiplayer_protocol,2);put(b,kind,1);put(b,1,1);
        put(b,type,1);encode_context(b,binding);put(b,self,1);put(b,target,1);encode_cut(b,*value.pending);
        packets[target][type]=std::move(b);
    }
    void broadcast(unsigned type){for(unsigned peer=0;peer<16;++peer)send(peer,type);}
    void refresh(){
        if(!value.pending)return;
        if(self==binding.host){
            broadcast(Offer);
            if(value.abort)broadcast(Abort);
            else {if(value.commit)broadcast(Commit);if(value.release)broadcast(Release);}
        }else {
            const auto own=bit(self);
            if(!value.abort){
                if(value.associated&own)broadcast(Associated);
                if(value.applied&own)broadcast(Applied);
                if(value.released&own)broadcast(Released);
            }
            if(local_rejected&&!value.abort)send(binding.host,Reject);
            if(value.aborted&own)broadcast(Aborted);
        }
    }
};
PeerCommonCutAgreement::PeerCommonCutAgreement(unsigned self,AuthorityTransferContext context):impl_(std::make_unique<Impl>(self,context)){}
PeerCommonCutAgreement::~PeerCommonCutAgreement()=default;
AuthorityTransferContext PeerCommonCutAgreement::context()const{return impl_->binding;}
bool PeerCommonCutAgreement::offer(const PeerCommonCut &cut){auto &s=*impl_;std::lock_guard lock(s.mutex);
    return s.self==s.binding.host&&s.offer(cut);}
bool PeerCommonCutAgreement::confirm_native(const PeerCommonCut &cut){auto &s=*impl_;std::lock_guard lock(s.mutex);
    if(s.self!=s.binding.host||!s.matches(cut)||s.value.abort)return false;
    if(!s.value.native_all_validated){s.value.native_all_validated=true;s.dirty=true;s.progress();}return true;}
bool PeerCommonCutAgreement::associate(const PeerCommonCut &cut){auto &s=*impl_;std::lock_guard lock(s.mutex);
    if(!s.recipient(s.self)||!s.matches(cut)||s.value.abort||s.local_rejected)return false;
    if(!(s.value.associated&s.bit(s.self))){s.value.associated|=s.bit(s.self);s.dirty=true;}return true;}
bool PeerCommonCutAgreement::apply(const PeerCommonCut &cut){auto &s=*impl_;std::lock_guard lock(s.mutex);
    if(!s.recipient(s.self)||!s.matches(cut)||!s.value.commit||s.value.abort||s.value.associated!=s.recipients)return false;
    if(!(s.value.applied&s.bit(s.self))){s.value.applied|=s.bit(s.self);s.dirty=true;}return true;}
bool PeerCommonCutAgreement::release(const PeerCommonCut &cut){auto &s=*impl_;std::lock_guard lock(s.mutex);
    if(!s.recipient(s.self)||!s.matches(cut)||!s.value.release||s.value.abort||s.value.applied!=s.recipients)return false;
    if(!(s.value.released&s.bit(s.self))){s.value.released|=s.bit(s.self);s.value.committed=s.value.pending;s.dirty=true;}return true;}
bool PeerCommonCutAgreement::reject(const PeerCommonCut &cut){auto &s=*impl_;std::lock_guard lock(s.mutex);
    if(!s.recipient(s.self)||!s.matches(cut)||s.value.commit||s.value.abort||(s.value.associated&s.bit(s.self)))return false;
    if(!s.local_rejected){s.local_rejected=true;s.dirty=true;}return true;}
bool PeerCommonCutAgreement::abort(const PeerCommonCut &cut){auto &s=*impl_;std::lock_guard lock(s.mutex);
    return s.self==s.binding.host&&s.abort(cut);}
bool PeerCommonCutAgreement::acknowledge_abort(const PeerCommonCut &cut){auto &s=*impl_;std::lock_guard lock(s.mutex);
    if(!s.recipient(s.self)||!s.matches(cut)||!s.value.abort||s.value.commit)return false;
    if(!(s.value.aborted&s.bit(s.self))){s.value.aborted|=s.bit(s.self);s.dirty=true;}return true;}
std::vector<AuthorityTransferPacket> PeerCommonCutAgreement::poll(std::uint64_t now,unsigned budget){auto &s=*impl_;std::lock_guard lock(s.mutex);
    s.clock=std::max(s.clock,now);
    if(s.dirty||!s.emitted||s.clock-s.emitted_at>=retry_ms){s.refresh();s.dirty=false;s.emitted=true;s.emitted_at=s.clock;}
    std::vector<AuthorityTransferPacket> out;budget=std::min(budget,128u);
    for(unsigned scanned=0;scanned<16*types&&out.size()<budget;++scanned){const auto index=s.cursor++%(16*types);auto &packet=s.packets[index/types][index%types];
        if(packet){out.push_back({index/types,std::move(*packet)});packet.reset();}}
    return out;
}
bool PeerCommonCutAgreement::is_packet(std::span<const std::byte> p)noexcept{return p.size()>=7&&p[0]==std::byte{0x48}&&p[1]==std::byte{0x32}&&p[2]==std::byte{0x4e}&&p[3]==std::byte{0x32}&&p[6]==std::byte{kind};}
bool PeerCommonCutAgreement::receive(unsigned peer,std::span<const std::byte> packet,std::uint64_t now){
    if(!is_packet(packet))return false;
    auto &s=*impl_;std::lock_guard lock(s.mutex);s.clock=std::max(s.clock,now);
    try {
        const auto m=decode(packet);need(m.sender==peer&&m.target==s.self&&peer!=s.self&&member(s.binding,peer)&&s.exact(m.cut));
        const bool host_control=m.type==Offer||m.type==Commit||m.type==Release||m.type==Abort;
        need(host_control?peer==s.binding.host:s.recipient(peer));
        if(m.type==Offer){need(s.offer(m.cut));return true;}
        need(s.matches(m.cut));
        if(m.type==Abort){need(s.abort(m.cut));return true;}
        if(m.type==Aborted){need(s.value.abort&&!s.value.commit);s.value.aborted|=s.bit(peer);return true;}
        if(m.type==Reject){need(s.self==s.binding.host&&!s.value.commit&&!(s.value.associated&s.bit(peer)));need(s.abort(m.cut));return true;}
        need(!s.value.abort);
        if(m.type==Associated){s.value.associated|=s.bit(peer);s.progress();}
        else if(m.type==Commit){need(s.recipient(s.self)&&!s.local_rejected&&(s.value.associated&s.bit(s.self)));
            if(!s.value.commit){s.value.native_all_validated=true;s.value.commit=true;s.dirty=true;}}
        else if(m.type==Applied){need(s.value.commit&&(s.value.associated&s.bit(peer)));s.value.applied|=s.bit(peer);s.progress();}
        else if(m.type==Release){need(s.recipient(s.self)&&s.value.commit&&(s.value.applied&s.bit(s.self)));
            if(!s.value.release){s.value.release=true;s.dirty=true;}}
        else if(m.type==Released){need(s.value.release&&(s.value.applied&s.bit(peer)));s.value.released|=s.bit(peer);s.progress();}
    }catch(const std::exception &){if(s.value.rejected<UINT64_MAX)++s.value.rejected;}
    return true;
}
PeerCommonCutState PeerCommonCutAgreement::state()const{const auto &s=*impl_;std::lock_guard lock(s.mutex);return s.value;}
}

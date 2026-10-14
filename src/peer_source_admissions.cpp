#include "halo2/peer_source_admissions.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <bit>
#include <mutex>
#include <stdexcept>
#include <string_view>

namespace halo2 {
namespace {
using Packet=std::vector<std::byte>;
constexpr unsigned header_bytes=89,maximum_packet=522;
enum Type:unsigned {Offer,Ack};
void need(bool value){if(!value)throw std::invalid_argument("Invalid native source admission exchange");}
void put(Packet &p,std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)p.push_back(std::byte(value>>(8*i)));}
struct Reader {
    std::span<const std::byte> bytes;std::size_t at{};
    std::span<const std::byte> raw(unsigned n){need(n<=bytes.size()-at);auto value=bytes.subspan(at,n);at+=n;return value;}
    std::uint64_t get(unsigned n){std::uint64_t value{};auto p=raw(n);for(unsigned i=0;i<n;++i)value|=std::to_integer<std::uint64_t>(p[i])<<(8*i);return value;}
    bool done()const{return at==bytes.size();}
};
bool member(const AuthorityTransferContext &c,unsigned peer){return peer<16&&(c.peers&(1u<<peer));}
void context_bytes(Packet &p,const AuthorityTransferContext &c){
    p.insert(p.end(),c.session.begin(),c.session.end());put(p,c.epoch,4);put(p,c.membership_revision,8);put(p,c.peers,2);put(p,c.host,1);
}
Packet header(const AuthorityTransferContext &c,unsigned target,Type type,const AuthorityTransferDigest &digest){
    Packet p;p.reserve(maximum_packet);put(p,0x324e3248,4);put(p,native_multiplayer_protocol,2);put(p,8,1);put(p,1,1);put(p,type,1);
    context_bytes(p,c);put(p,target,1);p.insert(p.end(),digest.begin(),digest.end());return p;
}
AuthorityTransferDigest hash(const AuthorityTransferContext &c,std::span<const std::byte> body){
    Packet p;for(char v:std::string_view("H2 SOURCE ADMISSIONS 1"))p.push_back(std::byte(v));
    context_bytes(p,c);p.insert(p.end(),body.begin(),body.end());AuthorityTransferDigest digest{};
    need(SHA256(reinterpret_cast<const unsigned char *>(p.data()),p.size(),reinterpret_cast<unsigned char *>(digest.data())));return digest;
}
void validate_roster(unsigned self,const AuthorityTransferContext &c,std::span<const OnlineAdmission> roster){
    need(roster.size()>=2&&roster.size()<=16&&member(c,self)&&member(c,c.host)&&std::popcount(c.peers)>=1&&
        std::any_of(c.session.begin(),c.session.end(),[](auto b){return b!=std::byte{};}));
    std::uint16_t physical{};
    for(unsigned slot=0;slot<roster.size();++slot){const auto &p=roster[slot];const auto owner=online_admission_peer(p);
        need(p.slot==slot&&owner<roster.size()&&online_admission_peer(roster[owner])==owner&&!roster[owner].guest);
        need(std::count_if(roster.begin(),roster.end(),[&](const auto &v){return online_admission_peer(v)==owner;})<=4);
        if(owner==slot)physical|=std::uint16_t(1u<<slot);
    }
    need(!(c.peers&~physical));
}
Packet body(std::span<const SourcePlayerAdmission> players,std::span<const OnlineAdmission> roster){
    need(players.size()==roster.size());Packet p;put(p,players.size(),1);
    for(unsigned slot=0;slot<players.size();++slot){const auto &source=players[slot];const auto &accepted=roster[slot];
        need(source.slot==slot&&source.player!=0xffffffffu&&(source.player&65535u)==slot);
        for(unsigned before=0;before<slot;++before)need(source.identity!=players[before].identity);
        put(p,slot,1);put(p,online_admission_peer(accepted),1);put(p,accepted.guest,1);
        p.insert(p.end(),accepted.identity.begin(),accepted.identity.end());put(p,source.player,4);
        for(auto value:source.identity)p.push_back(std::byte(value));
    }
    return p;
}
std::vector<SourcePlayerAdmission> read_body(Reader &r,std::span<const OnlineAdmission> roster){
    need(r.get(1)==roster.size());std::vector<SourcePlayerAdmission> players;players.reserve(roster.size());
    for(unsigned slot=0;slot<roster.size();++slot){const auto &accepted=roster[slot];
        need(r.get(1)==slot&&r.get(1)==online_admission_peer(accepted)&&r.get(1)==unsigned(accepted.guest));
        need(std::ranges::equal(r.raw(8),accepted.identity));SourcePlayerAdmission entry{slot,std::uint32_t(r.get(4))};
        for(auto &value:entry.identity)value=std::uint8_t(r.get(1));players.push_back(entry);
    }
    need(r.done());(void)body(players,roster);return players;
}
}
struct PeerSourceAdmissions::Impl {
    const unsigned self;AuthorityTransferContext binding;const std::vector<OnlineAdmission> roster;
    mutable std::mutex mutex;std::optional<AcceptedSourcePlayers> value;Packet payload;
    std::uint16_t acknowledged{},sent{};std::array<std::uint64_t,16> sent_at{};unsigned cursor{};bool ack_pending{};
    Impl(unsigned s,AuthorityTransferContext c,std::span<const OnlineAdmission> r):self(s),binding(c),roster(r.begin(),r.end()){validate_roster(self,c,roster);}
    void reset(AuthorityTransferContext next){binding=next;value.reset();payload.clear();acknowledged=sent=0;sent_at.fill(0);ack_pending=false;}
};
PeerSourceAdmissions::PeerSourceAdmissions(unsigned self,AuthorityTransferContext c,std::span<const OnlineAdmission> roster)
    :impl_(std::make_unique<Impl>(self,c,roster)){}
PeerSourceAdmissions::~PeerSourceAdmissions()=default;
AuthorityTransferContext PeerSourceAdmissions::context()const{std::lock_guard lock(impl_->mutex);return impl_->binding;}
bool PeerSourceAdmissions::replace_context(const AuthorityTransferContext &expected,AuthorityTransferContext next){
    auto &s=*impl_;validate_roster(s.self,next,s.roster);std::lock_guard lock(s.mutex);
    if(s.binding!=expected||next.session!=expected.session||next.epoch<expected.epoch||
        (next.epoch==expected.epoch&&(next.host!=expected.host||next.membership_revision<=expected.membership_revision)))return false;
    s.reset(next);return true;
}
bool PeerSourceAdmissions::publish(const AuthorityTransferContext &context,std::span<const SourcePlayerAdmission> players){
    auto &s=*impl_;auto bytes=body(players,s.roster);const auto digest=hash(context,bytes);std::lock_guard lock(s.mutex);
    if(context!=s.binding||s.self!=context.host)return false;
    AcceptedSourcePlayers value{context,digest,{players.begin(),players.end()}};
    if(s.value)return *s.value==value;
    s.value=std::move(value);s.payload=std::move(bytes);return true;
}
std::optional<AcceptedSourcePlayers> PeerSourceAdmissions::accepted()const{std::lock_guard lock(impl_->mutex);return impl_->value;}
bool PeerSourceAdmissions::confirmed(unsigned peer)const{
    auto &s=*impl_;std::lock_guard lock(s.mutex);return s.self==s.binding.host&&s.value&&peer!=s.self&&member(s.binding,peer)&&(s.acknowledged&(1u<<peer));
}
std::vector<AuthorityTransferPacket> PeerSourceAdmissions::poll(std::uint64_t now,unsigned budget){
    auto &s=*impl_;std::lock_guard lock(s.mutex);std::vector<AuthorityTransferPacket> packets;budget=std::min(budget,16u);
    if(!s.value||!budget)return packets;
    if(s.self!=s.binding.host){if(s.ack_pending){packets.push_back({s.binding.host,header(s.binding,s.binding.host,Ack,s.value->digest)});s.ack_pending=false;}return packets;}
    for(unsigned i=0;i<16&&packets.size()<budget;++i){const auto peer=s.cursor++%16;
        if(peer==s.self||!member(s.binding,peer)||(s.acknowledged&(1u<<peer)))continue;
        if((s.sent&(1u<<peer))&&(now<s.sent_at[peer]||now-s.sent_at[peer]<250))continue;
        auto bytes=header(s.binding,peer,Offer,s.value->digest);bytes.insert(bytes.end(),s.payload.begin(),s.payload.end());
        packets.push_back({peer,std::move(bytes)});s.sent|=std::uint16_t(1u<<peer);s.sent_at[peer]=now;
    }
    return packets;
}
bool PeerSourceAdmissions::is_packet(std::span<const std::byte> packet){
    return packet.size()>=7&&packet[0]==std::byte{0x48}&&packet[1]==std::byte{0x32}&&
        packet[2]==std::byte{0x4e}&&packet[3]==std::byte{0x32}&&packet[6]==std::byte{8};
}
bool PeerSourceAdmissions::receive(unsigned peer,std::span<const std::byte> packet){
    if(!is_packet(packet))return false;
    try {
        need(packet.size()>=header_bytes&&packet.size()<=maximum_packet);Reader r{packet};
        need(r.get(4)==0x324e3248&&r.get(2)==native_multiplayer_protocol&&r.get(1)==8&&r.get(1)==1);
        const auto type=r.get(1);need(type<=Ack);AuthorityTransferContext c;auto session=r.raw(32);std::copy(session.begin(),session.end(),c.session.begin());
        c.epoch=std::uint32_t(r.get(4));c.membership_revision=r.get(8);c.peers=std::uint16_t(r.get(2));c.host=unsigned(r.get(1));const auto target=r.get(1);
        AuthorityTransferDigest digest{};auto raw_digest=r.raw(32);std::copy(raw_digest.begin(),raw_digest.end(),digest.begin());
        auto &s=*impl_;std::lock_guard lock(s.mutex);need(c==s.binding&&target==s.self&&peer!=s.self&&member(c,peer));
        if(type==Ack){need(r.done()&&s.self==c.host&&s.value&&digest==s.value->digest&&(s.sent&(1u<<peer)));s.acknowledged|=std::uint16_t(1u<<peer);}
        else {
            need(peer==c.host&&s.self!=c.host);const auto start=r.at;auto players=read_body(r,s.roster);
            need(hash(c,packet.subspan(start))==digest);AcceptedSourcePlayers value{c,digest,std::move(players)};
            if(s.value)need(*s.value==value);else s.value=std::move(value);
            s.ack_pending=true;
        }
    }catch(const std::exception &){} // Invalid bytes cannot replace accepted identities or evict peers.
    return true;
}
}

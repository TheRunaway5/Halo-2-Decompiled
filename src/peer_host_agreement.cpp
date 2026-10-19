#include "halo2/peer_host_agreement.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include <algorithm>
#include <bit>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>

namespace halo2 {
namespace {
using Bytes=std::vector<std::byte>;
enum Type:unsigned {Probe,Silence,Ready,Prepare,Promise,Accept,Accepted,Nack,types};
constexpr unsigned kind=10,maximum_packet=134;
constexpr std::uint64_t retry_ms=100,silence_ms=500;
constexpr std::uint64_t proposal_retry_ms=1000,proposer_spacing_ms=50;
void need(bool ok){if(!ok)throw std::invalid_argument("Invalid peer host agreement packet or context");}
bool member(const AuthorityTransferContext &c,unsigned p){return p<16&&(c.peers&(1u<<p));}
template<class T>bool nonzero(const T &v){return std::ranges::any_of(v,[](auto b){return b!=std::byte{};});}
void put(Bytes &out,std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)out.push_back(std::byte(value>>(8*i)));}
struct Reader {
    std::span<const std::byte> in;std::size_t at{};
    auto raw(unsigned n){need(n<=in.size()-at);auto v=in.subspan(at,n);at+=n;return v;}
    std::uint64_t get(unsigned n){auto b=raw(n);std::uint64_t value{};for(unsigned i=0;i<n;++i)value|=std::uint64_t(std::to_integer<unsigned>(b[i]))<<(8*i);return value;}
    void end(){need(at==in.size());}
};
void context(Bytes &b,const AuthorityTransferContext &c){b.insert(b.end(),c.session.begin(),c.session.end());
    put(b,c.epoch,4);put(b,c.membership_revision,8);put(b,c.peers,2);put(b,c.host,1);}
auto context(Reader &r){AuthorityTransferContext c;std::ranges::copy(r.raw(32),c.session.begin());c.epoch=r.get(4);
    c.membership_revision=r.get(8);c.peers=r.get(2);c.host=r.get(1);return c;}
void ballot(Bytes &b,PeerMigrationBallot n){put(b,n.round,8);put(b,n.proposer,1);}
auto ballot(Reader &r){return PeerMigrationBallot{r.get(8),unsigned(r.get(1))};}
void value(Bytes &b,const PeerMigrationValue &v){put(b,v.host,1);put(b,v.cut.id,8);put(b,v.cut.tick,8);put(b,v.cut.bytes,8);
    b.insert(b.end(),v.cut.digest.begin(),v.cut.digest.end());}
auto value(Reader &r){PeerMigrationValue v;v.host=r.get(1);v.cut.id=r.get(8);v.cut.tick=r.get(8);v.cut.bytes=r.get(8);
    std::ranges::copy(r.raw(32),v.cut.digest.begin());return v;}
struct Vote {PeerMigrationBallot ballot;PeerMigrationValue value;bool operator==(const Vote &)const=default;};
struct Message {unsigned type{},sender{},target{};AuthorityTransferContext context;PeerMigrationBallot ballot;
    std::optional<Vote> prior;std::optional<PeerMigrationValue> value;bool silent{};};
auto decode(std::span<const std::byte> b){need(b.size()>=58&&b.size()<=maximum_packet);Reader r{b};
    need(r.get(4)==0x324e3248&&r.get(2)==native_multiplayer_protocol&&r.get(1)==kind&&r.get(1)==1);
    Message m;m.type=r.get(1);need(m.type<types);m.context=context(r);m.sender=r.get(1);m.target=r.get(1);
    if(m.type==Silence){const auto flag=r.get(1);need(flag<=1);m.silent=flag;}
    if(m.type==Ready)m.value=value(r);
    if(m.type>=Prepare)m.ballot=ballot(r);
    if(m.type==Promise){const auto present=r.get(1);need(present<=1);if(present)m.prior=Vote{ballot(r),value(r)};}
    if(m.type==Accept||m.type==Accepted)m.value=value(r);
    r.end();return m;
}
}
struct PeerHostAgreement::Impl {
    const unsigned self;const AuthorityTransferContext context;
    const std::uint16_t electorate;const unsigned majority;
    mutable std::shared_mutex mutex;std::uint64_t host_at{},clock{},retry_at{},rejected{};bool emitted{};
    PeerMigrationBallot promised;std::optional<Vote> accepted;
    std::array<std::optional<AuthorityCutSeal>,16> ready;
    std::array<std::optional<Vote>,16> learned;
    struct Response {std::optional<Vote> prior;};
    std::array<std::optional<Response>,16> responses;
    std::array<std::uint64_t,16> silent_at{};std::uint16_t silent_seen{};
    std::array<std::array<std::optional<Bytes>,types>,16> pending;
    unsigned cursor{};std::uint64_t highest_round{};
    enum Stage {Idle,Preparing,Accepting};Stage stage=Idle;
    PeerMigrationBallot proposal;std::optional<PeerMigrationValue> preferred,selected;
    std::optional<PeerMigrationDecision> decision;
    std::optional<std::uint64_t> automatic_after;
    Impl(unsigned s,AuthorityTransferContext c,std::uint64_t now):self(s),context(c),
        electorate(c.host<16?std::uint16_t(c.peers&~(1u<<c.host)):0),majority(std::popcount(electorate)/2+1),host_at(now),clock(now){
        need(member(c,s)&&member(c,c.host)&&nonzero(c.session)&&std::popcount(c.peers)>=2&&c.epoch<UINT32_MAX&&c.membership_revision<UINT64_MAX);
    }
    bool voter(unsigned p)const{return p<16&&(electorate&(1u<<p));}
    void tick(std::uint64_t now){clock=std::max(clock,now);}
    std::uint64_t later(std::uint64_t delay)const{return delay>UINT64_MAX-clock?UINT64_MAX:clock+delay;}
    unsigned proposer_order()const{return std::popcount(unsigned(electorate)&((1u<<self)-1));}
    void progress_seen(){automatic_after=later(proposal_retry_ms+proposer_order()*proposer_spacing_ms);}
    bool silent(std::uint64_t now)const{return self!=context.host&&(accepted||decision||std::max(clock,now)-host_at>silence_ms);}
    void valid(PeerMigrationBallot n)const{need(n.round&&voter(n.proposer));}
    void valid(const PeerMigrationValue &v)const{need(voter(v.host)&&v.cut.id&&v.cut.bytes&&v.cut.bytes<=64ull*1024*1024&&nonzero(v.cut.digest));}
    bool known(const PeerMigrationValue &v)const{return voter(v.host)&&ready[v.host]&&*ready[v.host]==v.cut;}
    Bytes header(unsigned to,unsigned type)const{Bytes b;put(b,0x324e3248,4);put(b,native_multiplayer_protocol,2);put(b,kind,1);put(b,1,1);
        put(b,type,1);::halo2::context(b,context);put(b,self,1);put(b,to,1);return b;}
    template<class F>void send(unsigned to,unsigned type,F append){if(to==self||!member(context,to))return;
        auto b=header(to,type);append(b);need(b.size()<=maximum_packet);pending[to][type]=std::move(b);}
    template<class F>void broadcast(unsigned type,F append){for(unsigned to=0;to<16;++to)send(to,type,append);}
    void learn(unsigned peer,Vote vote){
        if(learned[peer]){if(vote.ballot<learned[peer]->ballot)return;
            need(vote.ballot!=learned[peer]->ballot||vote.value==learned[peer]->value);}
        if(decision)need(vote.value==decision->value);
        if(!learned[peer]||*learned[peer]!=vote)progress_seen();
        learned[peer]=vote;std::uint16_t votes{};
        for(unsigned p=0;p<16;++p)if(voter(p)&&learned[p]&&*learned[p]==vote)votes|=std::uint16_t(1u<<p);
        if(unsigned(std::popcount(votes))<majority)return;
        if(!decision){auto next=context;++next.epoch;++next.membership_revision;next.peers=electorate;next.host=vote.value.host;
            decision=PeerMigrationDecision{context,next,vote.ballot,vote.value,votes};}
    }
    void accept(PeerMigrationBallot n,const PeerMigrationValue &v){
        need(voter(self)&&silent(clock)&&n>=promised&&known(v)&&(!decision||decision->value==v));
        if(accepted&&accepted->ballot==n)need(accepted->value==v);
        if(!accepted||accepted->ballot!=n)progress_seen();
        promised=n;highest_round=std::max(highest_round,n.round);accepted=Vote{n,v};learn(self,*accepted);
        broadcast(Accepted,[&](Bytes &b){ballot(b,n);value(b,v);});
    }
    void progress(){
        if(stage!=Preparing)return;
        unsigned count{};std::optional<Vote> highest;
        for(const auto &r:responses)if(r){++count;if(r->prior){
            if(highest&&highest->ballot==r->prior->ballot)need(highest->value==r->prior->value);
            if(!highest||r->prior->ballot>highest->ballot)highest=r->prior;}}
        if(count<majority)return;
        const auto v=highest?highest->value:*preferred;if(!known(v))return;
        selected=v;stage=Accepting;progress_seen();
        if(proposal>=promised&&silent(clock))accept(proposal,v);
        broadcast(Accept,[&](Bytes &b){ballot(b,proposal);value(b,v);});
    }
    void promise(unsigned to,PeerMigrationBallot n){
        if(n<promised){send(to,Nack,[&](Bytes &b){ballot(b,promised);});return;}
        if(n>promised)progress_seen();
        promised=n;highest_round=std::max(highest_round,n.round);
        if(to==self){responses[self]=Response{accepted};progress();}
        else send(to,Promise,[&](Bytes &b){ballot(b,n);put(b,accepted.has_value(),1);
            if(accepted){ballot(b,accepted->ballot);value(b,accepted->value);}});
    }
    bool propose(unsigned candidate){
        if(!voter(self)||!silent(clock)||decision||!voter(candidate)||!ready[candidate]||highest_round==UINT64_MAX)return false;
        proposal={++highest_round,self};preferred=PeerMigrationValue{candidate,*ready[candidate]};selected.reset();responses.fill({});stage=Preparing;
        progress_seen();promise(self,proposal);broadcast(Prepare,[&](Bytes &b){ballot(b,proposal);});return true;
    }
};
PeerHostAgreement::PeerHostAgreement(unsigned s,AuthorityTransferContext c,std::uint64_t now):impl_(std::make_unique<Impl>(s,c,now)){}
PeerHostAgreement::~PeerHostAgreement()=default;
void PeerHostAgreement::observe_host(std::uint64_t now){auto &s=*impl_;std::unique_lock lock(s.mutex);s.tick(now);s.host_at=s.clock;
    if(!s.accepted&&!s.decision)s.automatic_after.reset();}
bool PeerHostAgreement::publish_ready(const ValidatedAuthorityCut &cut){auto &s=*impl_;std::unique_lock lock(s.mutex);
    if(!s.voter(s.self)||cut.peer!=s.self||cut.context!=s.context)return false;
    const PeerMigrationValue v{s.self,cut.seal};s.valid(v);
    if(s.ready[s.self])return *s.ready[s.self]==cut.seal;
    s.ready[s.self]=cut.seal;s.broadcast(Ready,[&](Bytes &b){value(b,v);});s.progress();return true;
}
bool PeerHostAgreement::propose(unsigned candidate,std::uint64_t now){auto &s=*impl_;std::unique_lock lock(s.mutex);s.tick(now);
    return s.propose(candidate);
}
bool PeerHostAgreement::drive(unsigned candidate,std::uint64_t now){auto &s=*impl_;std::unique_lock lock(s.mutex);s.tick(now);
    if(!s.voter(s.self)||!s.silent(s.clock)||s.decision||!s.voter(candidate)||!s.ready[candidate])return false;
    if(!s.automatic_after)s.automatic_after=s.later(s.proposer_order()*proposer_spacing_ms);
    if(s.clock<*s.automatic_after)return false;
    return s.propose(candidate);
}
std::vector<AuthorityTransferPacket> PeerHostAgreement::poll(std::uint64_t now,unsigned budget){auto &s=*impl_;std::unique_lock lock(s.mutex);s.tick(now);
    if(!s.emitted||s.clock-s.retry_at>=retry_ms){s.emitted=true;s.retry_at=s.clock;
        if(s.ready[s.self])s.broadcast(Ready,[&](Bytes &b){value(b,PeerMigrationValue{s.self,*s.ready[s.self]});});
        if(s.voter(s.self)){
            const bool silent=s.silent(s.clock);s.broadcast(Silence,[&](Bytes &b){put(b,silent,1);});
            if(silent)s.broadcast(Probe,[](Bytes &){});
        }
        if(s.stage==Impl::Preparing)s.broadcast(Prepare,[&](Bytes &b){ballot(b,s.proposal);});
        if(s.stage==Impl::Accepting)s.broadcast(Accept,[&](Bytes &b){ballot(b,s.proposal);value(b,*s.selected);});
        if(s.accepted)s.broadcast(Accepted,[&](Bytes &b){ballot(b,s.accepted->ballot);value(b,s.accepted->value);});
    }
    std::vector<AuthorityTransferPacket> out;budget=std::min(budget,128u);
    for(unsigned scanned=0;scanned<128&&out.size()<budget;++scanned){const auto index=s.cursor++%128;auto &p=s.pending[index/types][index%types];
        if(p){out.push_back({index/types,std::move(*p)});p.reset();}}
    return out;
}
bool PeerHostAgreement::is_packet(std::span<const std::byte> p)noexcept{return p.size()>=7&&p[0]==std::byte{0x48}&&p[1]==std::byte{0x32}&&p[2]==std::byte{0x4e}&&p[3]==std::byte{0x32}&&p[6]==std::byte{kind};}
bool PeerHostAgreement::receive(unsigned peer,std::span<const std::byte> packet,std::uint64_t now){
    if(!is_packet(packet))return false;
    auto &s=*impl_;std::unique_lock lock(s.mutex);s.tick(now);
    try{const auto m=decode(packet);need(m.context==s.context&&m.sender==peer&&m.target==s.self&&peer!=s.self&&s.voter(peer));
        if(m.type>=Prepare)s.valid(m.ballot);
        if(m.value)s.valid(*m.value);
        if(m.prior){s.valid(m.prior->ballot);s.valid(m.prior->value);need(m.prior->ballot<=m.ballot);}
        if(m.type==Prepare||m.type==Accept)need(m.ballot.proposer==peer);
        if(m.type==Probe){if(s.voter(s.self))s.send(peer,Silence,[&](Bytes &b){put(b,s.silent(s.clock),1);});}
        else if(m.type==Silence){s.silent_seen|=std::uint16_t(1u<<peer);s.silent_at[peer]=s.clock;
            if(!m.silent)s.silent_seen&=std::uint16_t(~(1u<<peer));}
        else if(m.type==Ready){need(m.value->host==peer&&(!s.ready[peer]||*s.ready[peer]==m.value->cut));s.ready[peer]=m.value->cut;s.progress();}
        else if(m.type==Prepare){if(s.voter(s.self)&&s.silent(s.clock))s.promise(peer,m.ballot);}
        else if(m.type==Promise){need(s.voter(s.self));if(s.stage==Impl::Preparing&&m.ballot==s.proposal){
            if(!s.responses[peer]){s.responses[peer]=Impl::Response{m.prior};s.progress_seen();}
            s.progress();}}
        else if(m.type==Accept){if(s.voter(s.self)&&s.silent(s.clock)){
            if(m.ballot<s.promised)s.send(peer,Nack,[&](Bytes &b){ballot(b,s.promised);});
            else if(s.known(*m.value))s.accept(m.ballot,*m.value);}}
        else if(m.type==Accepted){s.learn(peer,Vote{m.ballot,*m.value});}
        else if(m.type==Nack){if(m.ballot.round>s.highest_round)s.progress_seen();s.highest_round=std::max(s.highest_round,m.ballot.round);}
    }catch(const std::exception &){if(s.rejected<UINT64_MAX)++s.rejected;}
    return true;
}
PeerHostAgreementState PeerHostAgreement::state(std::uint64_t now)const{const auto &s=*impl_;std::shared_lock lock(s.mutex);
    const auto clock=std::max(s.clock,now);std::uint16_t silent{};
    for(unsigned p=0;p<16;++p)if(s.voter(p)&&(s.silent_seen&(1u<<p))&&clock-s.silent_at[p]<=retry_ms*3)silent|=std::uint16_t(1u<<p);
    if(s.voter(s.self)&&s.silent(clock))silent|=std::uint16_t(1u<<s.self);
    return {s.silent(clock),bool(s.accepted||s.decision),s.electorate,silent,s.majority,s.ready,s.decision,s.rejected};
}
}

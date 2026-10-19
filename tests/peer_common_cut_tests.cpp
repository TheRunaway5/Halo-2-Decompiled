#include "halo2/peer_common_cut.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <tuple>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F &&f){bool rejected{};try{f();}catch(const std::exception &){rejected=true;}check(rejected,"Invalid common-cut constructor accepted");}
auto context(std::uint16_t peers=0x111,unsigned host=0){AuthorityTransferContext c;c.session[0]=std::byte{0x15};c.epoch=2;c.membership_revision=5;c.peers=peers;c.host=host;return c;}
auto cut(const AuthorityTransferContext &c,std::uint64_t id=1,std::uint64_t tick=123){return PeerCommonCut{c,{id,tick,4096,{std::byte(id)}},std::uint16_t(c.peers&~(1u<<c.host))};}
// Real encoder positions, including the dispatcher prefix. Tests mutate the
// actual emitted packets rather than manufacturing valid phase certificates.
enum Type:unsigned {Offer,Associated,Commit,Applied,Release,Released,Reject,Abort,Aborted,types};
unsigned type(std::span<const std::byte> bytes){check(bytes.size()==116,"Unexpected common-cut wire extent");return std::to_integer<unsigned>(bytes[8]);}
bool same_safety_state(const PeerCommonCutState &a,const PeerCommonCutState &b){return
    a.context==b.context&&a.recipients==b.recipients&&a.committed==b.committed&&a.pending==b.pending&&
    a.associated==b.associated&&a.applied==b.applied&&a.released==b.released&&a.aborted==b.aborted&&
    a.native_all_validated==b.native_all_validated&&a.commit==b.commit&&a.release==b.release&&a.abort==b.abort;}

struct Mesh {
    struct Packet {unsigned from,to;std::vector<std::byte> bytes;};
    AuthorityTransferContext ctx;
    std::array<std::unique_ptr<PeerCommonCutAgreement>,16> peers;
    std::deque<Packet> wire;
    std::map<std::tuple<unsigned,unsigned,unsigned>,Packet> samples;
    std::set<std::tuple<unsigned,unsigned,std::vector<std::byte>>> dropped_once;
    std::function<bool(const Packet &)> allowed=[](const auto &){return true;};
    std::uint64_t now{};
    unsigned serial{},losses{},reorders{},duplicates{};
    explicit Mesh(std::uint16_t mask=0x111,unsigned host=0):ctx(context(mask,host)){
        for(unsigned p=0;p<16;++p)if(mask&(1u<<p))peers[p]=std::make_unique<PeerCommonCutAgreement>(p,ctx);
    }
    template<class F>void recipients(F &&f){for(unsigned p=0;p<16;++p)if(peers[p]&&p!=ctx.host)f(p,*peers[p]);}
    auto &host(){return *peers[ctx.host];}
    const Packet &sample(unsigned kind,unsigned from,unsigned to)const{
        const auto found=samples.find({kind,from,to});check(found!=samples.end(),"Required real control packet was not observed");return found->second;
    }
    void step(bool lossy=false,unsigned budget=4){now+=101;
        for(unsigned p=0;p<16;++p)if(peers[p])for(auto &out:peers[p]->poll(now,budget)){
            check(PeerCommonCutAgreement::is_packet(out.bytes)&&out.bytes.size()==116,"Unbounded or unrecognized emitted common-cut control");
            check(out.peer<16&&out.peer!=p&&(ctx.peers&(1u<<out.peer)),"Control escaped physical membership");
            Packet packet{p,out.peer,std::move(out.bytes)};
            samples.try_emplace(std::tuple{type(packet.bytes),p,packet.to},packet);
            if(!allowed(packet))continue;
            // Every distinct datagram loses its first attempt, so loss is
            // bounded per route/message and cannot permanently starve a route.
            if(lossy&&dropped_once.emplace(p,packet.to,packet.bytes).second){++losses;continue;}
            ++serial;
            if(lossy&&serial%2==0){wire.push_front(packet);++reorders;}else wire.push_back(packet);
            if(lossy&&serial%3==0){wire.push_back(packet);++duplicates;}
        }
        while(!wire.empty()){auto packet=std::move(wire.front());wire.pop_front();
            if(peers[packet.to]&&allowed(packet))check(peers[packet.to]->receive(packet.from,packet.bytes,now),"Real control was not dispatched");}
    }
    void pump(unsigned steps=64,bool lossy=true,unsigned budget=4){for(unsigned i=0;i<steps;++i)step(lossy,budget);}
    void offer(const PeerCommonCut &value){check(host().offer(value),"Host refused a fresh exact full-membership cut");pump();
        recipients([&](unsigned,auto &p){check(p.state().pending==value,"Offer did not reach every physical recipient");});}
    void complete(const PeerCommonCut &value){offer(value);
        recipients([&](unsigned,auto &p){check(p.associate(value),"Local exact association refused");});pump();
        check(!host().state().commit,"Association alone manufactured host native validation");
        check(host().confirm_native(value),"Host native confirmation refused");pump();
        recipients([&](unsigned,auto &p){check(p.state().commit&&p.apply(value),"Exact committed cut could not be locally applied");});pump();
        check(host().state().release,"All explicit local applies did not release old pins");
        recipients([&](unsigned,auto &p){check(p.state().release&&p.release(value),"Released cut could not be locally published");});pump();
        check(host().state().committed==value&&host().state().released==value.recipients,"Host promoted before all distinct Released acknowledgements");
        recipients([&](unsigned,auto &p){check(p.state().committed==value,"Recipients did not retain the exact common cut");});
    }
};

void constructor_and_local_barriers(){
    auto c=context();rejects([&]{PeerCommonCutAgreement p(16,c);});rejects([&]{PeerCommonCutAgreement p(1,c);});
    auto bad=c;bad.host=16;rejects([&]{PeerCommonCutAgreement p(4,bad);});bad=c;bad.host=1;rejects([&]{PeerCommonCutAgreement p(4,bad);});
    bad=c;bad.session={};rejects([&]{PeerCommonCutAgreement p(4,bad);});rejects([&]{PeerCommonCutAgreement p(0,context(1));});
    Mesh m;const auto first=cut(m.ctx),next=cut(m.ctx,2),third=cut(m.ctx,3);
    for(auto change:{0u,1u,2u,3u}){auto invalid=first;
        if(change==0)invalid.seal.id=0;if(change==1)invalid.seal.bytes=0;
        if(change==2)invalid.seal.bytes=64ull*1024*1024+1;if(change==3)invalid.seal.digest={};
        check(!m.host().offer(invalid),"Invalid native seal was offered");}
    for(auto mask:{std::uint16_t(0x10),std::uint16_t(0x111),std::uint16_t(0x112)}){auto invalid=first;invalid.recipients=mask;
        check(!m.host().offer(invalid),"Host shrank membership or counted a host/couch seat as recipient");}
    check(!m.peers[4]->offer(first)&&!m.peers[4]->confirm_native(first)&&!m.peers[4]->abort(first),"Recipient performed a host-only action");
    check(!m.host().associate(first)&&!m.host().apply(first)&&!m.host().release(first)&&!m.host().acknowledge_abort(first),"Host performed a recipient-only action");
    check(!m.peers[4]->associate(first)&&!m.peers[4]->apply(first)&&!m.peers[4]->release(first)&&!m.peers[4]->reject(first),"A local action created a missing offer");
    m.complete(first);m.offer(next);
    check(!m.peers[4]->apply(next)&&!m.peers[4]->release(next)&&!m.peers[4]->acknowledge_abort(next),"Offer bypassed Commit/Release/Abort barriers");
    auto foreign=next;foreign.seal.digest[1]=std::byte{1};
    check(!m.peers[4]->associate(foreign)&&!m.host().confirm_native(foreign),"A different exact cut reused pending eligibility");
    m.allowed=[](const auto &packet){return packet.from!=4||packet.to!=8||type(packet.bytes)!=Associated;};
    check(m.peers[4]->associate(next)&&m.peers[4]->associate(next),"Local association was not idempotent");m.pump();
    check(!m.peers[4]->reject(next),"Associated recipient revoked its retained-pair promise");
    check(m.host().confirm_native(next),"Native confirmation refused before final association");m.pump();
    check(m.host().state().associated==0x10&&!m.host().state().commit,"Duplicate association counted as the missing physical peer");
    check(!m.host().offer(third),"Host replaced a partially associated round");
    // A genuine host control reordered ahead of the recipient's own local
    // phase must not stand in for that recipient's owner action.
    const auto controls=m.host().poll(m.now+101,128);
    const auto offer_to_last=std::ranges::find_if(controls,[](const auto &p){return p.peer==8&&type(p.bytes)==Offer;});
    check(offer_to_last!=controls.end(),"Current exact offer for phase-order control missing");
    auto early=offer_to_last->bytes;early[8]=std::byte{Commit};
    m.peers[8]->receive(0,early,m.now);check(!m.peers[8]->state().commit,"Commit bypassed local association");
    check(m.peers[8]->associate(next),"Final distinct recipient could not associate");m.pump();
    check(m.host().state().commit,"All physical associations plus native validation did not Commit");
    check(m.peers[8]->state().commit&&m.peers[8]->state().associated==0x100&&!m.peers[8]->apply(next),"Host Commit invented a missing peer association or bypassed full local visibility");
    m.allowed=[](const auto &){return true;};m.pump();
    m.recipients([&](unsigned,auto &p){check(p.state().commit&&!p.release(next)&&!p.reject(next),"Commit bypassed local apply or allowed rejection");});
    check(!m.host().abort(next),"Committed cut was aborted");
    m.allowed=[](const auto &packet){return packet.from!=4||packet.to!=8||type(packet.bytes)!=Applied;};
    check(m.peers[4]->apply(next),"First local apply refused");m.pump();
    check(m.host().state().applied==0x10&&!m.host().state().release,"One local apply released every old pin");
    check(m.peers[8]->apply(next),"Second local apply refused");m.pump();
    check(m.peers[8]->state().release&&m.peers[8]->state().applied==0x100&&!m.peers[8]->release(next),"Host Release invented a missing peer apply or bypassed full local visibility");
    m.allowed=[](const auto &){return true;};m.pump();
    m.recipients([&](unsigned,auto &p){check(p.state().release&&p.state().committed==first,"Release control implicitly published a local owner");});
    m.allowed=[](const auto &packet){return packet.from!=8||type(packet.bytes)!=Released;};
    m.recipients([&](unsigned,auto &p){check(p.release(next)&&p.release(next),"Explicit local release was not idempotent");});m.pump();
    check(m.host().state().released==0x10&&m.host().state().committed==first&&!m.host().offer(third),"Missing Released ACK was replaced by a timeout or duplicate");
    m.allowed=[](const auto &){return true;};m.pump();
    check(m.host().state().committed==next,"Delayed final release did not finish the round");
    auto regressed=third;--regressed.seal.tick;check(!m.host().offer(regressed),"Host offered a backwards native tick");
    check(!m.host().offer(first),"Host replayed a lower seal ID");
    m.complete(third); // A new exact seal at the same tick remains admissible.
    std::cout<<"PASS common-cut exact full-membership barriers: native confirmation, distinct associations, local apply, all releases and monotone same-tick seals\n";
}

void abort_and_repair(){Mesh m;const auto old=cut(m.ctx),pending=cut(m.ctx,2),repair=cut(m.ctx,3);m.complete(old);m.offer(pending);
    check(m.peers[4]->associate(pending)&&m.host().confirm_native(pending),"Partial association fixture refused");m.pump();
    check(!m.host().state().commit&&m.peers[8]->reject(pending)&&!m.peers[8]->associate(pending),"Missing receipt could not request safe pre-Commit Abort");m.pump();
    check(m.host().state().abort,"Recipient rejection did not reach host");
    m.recipients([&](unsigned,auto &p){check(p.state().abort&&p.state().committed==old&&!p.apply(pending)&&!p.release(pending),"Abort lost old common metadata or published pending state");});
    check(!m.host().confirm_native(pending)&&!m.host().offer(repair),"Host replaced an unretired aborted round");
    check(m.peers[4]->acknowledge_abort(pending)&&m.peers[4]->acknowledge_abort(pending),"Pending-pin retirement ACK was not idempotent");m.pump();
    check(m.host().state().aborted==0x10&&!m.host().offer(repair),"Duplicate abort ACK replaced missing retirement");
    check(m.peers[8]->acknowledge_abort(pending),"Last pending-pin retirement refused");m.pump();
    check(m.host().state().aborted==pending.recipients&&m.host().state().committed==old,"Abort acknowledgements changed the prior common cut");
    m.complete(repair);
    const auto fourth=cut(m.ctx,4);m.offer(fourth);
    m.recipients([&](unsigned,auto &p){check(p.associate(fourth),"Explicit host Abort fixture refused");});m.pump();
    check(m.host().abort(fourth)&&m.host().abort(fourth),"Host could not abort all-associated but unconfirmed pending cut");m.pump();
    m.recipients([&](unsigned,auto &p){check(p.acknowledge_abort(fourth),"Explicit host Abort did not permit pending-pin retirement");});m.pump();
    m.complete(cut(m.ctx,5));
    for(unsigned kind=0;kind<types;++kind){const bool observed=std::ranges::any_of(m.samples,[&](const auto &entry){return std::get<0>(entry.first)==kind;});
        check(observed,"Abort/repair corpus omitted a real control type");}
    std::cout<<"PASS common-cut pre-Commit rejection and explicit Abort repair: old cut retained, every pending pin explicitly retired, no membership shrink\n";
}

void packet_attacks(){Mesh m;const auto old=cut(m.ctx),pending=cut(m.ctx,2);m.complete(old);m.offer(pending);
    check(m.peers[4]->associate(pending),"Authenticated claim fixture refused");m.pump();
    const auto associated=m.sample(Associated,4,0).bytes; // A delayed claim from the earlier real cut.
    const auto before=m.host().state();
    for(unsigned i=0;i<32;++i){m.host().receive(4,associated,m.now);m.host().receive(8,associated,m.now);}
    check(same_safety_state(before,m.host().state()),"Delayed/relabeled ACK changed a new round");
    // Obtain the current exact claim through the actual encoder.
    const auto out=m.peers[4]->poll(m.now+101,128);
    const auto found=std::ranges::find_if(out,[](const auto &p){return p.peer==0&&type(p.bytes)==Associated;});
    check(found!=out.end(),"Current exact association packet missing");const auto exact=found->bytes;
    for(unsigned i=0;i<32;++i){m.host().receive(4,exact,m.now);m.host().receive(8,exact,m.now);}
    check(m.host().state().associated==0x10&&!m.host().state().commit,"One authenticated physical peer impersonated a second association");
    // Full context, sender, target, fixed recipient mask and exact full seal
    // must all match the pending round before any ACK is counted.
    for(auto offset:{4u,5u,7u,8u,9u,41u,45u,53u,55u,56u,57u,58u,60u,68u,76u,84u,115u}){
        auto bad=exact;bad[offset]^=std::byte{0x80};const auto saved=m.host().state();m.host().receive(4,bad,m.now);
        check(same_safety_state(saved,m.host().state()),"Malformed/foreign context, mask or seal changed eligibility");}
    const auto offer=m.sample(Offer,0,4).bytes;
    PeerCommonCutAgreement no_offer(4,m.ctx);no_offer.receive(8,offer,m.now);
    check(!no_offer.state().pending&&!no_offer.state().committed,"Recipient impersonated an authenticated host offer");
    auto foreign_context=m.ctx;++foreign_context.membership_revision;PeerCommonCutAgreement foreign(4,foreign_context);
    foreign.receive(0,offer,m.now);check(!foreign.state().pending,"Foreign room generation admitted an offer");
    // A phase certificate alone cannot establish its missing Offer or local
    // association, and a peer's metadata cannot manufacture host Commit.
    for(auto kind:{Associated,Commit,Applied,Release,Released,Reject,Abort,Aborted}){
        auto early=offer;early[8]=std::byte(kind);PeerCommonCutAgreement fresh(4,m.ctx);fresh.receive(0,early,m.now);
        check(!fresh.state().pending&&!fresh.state().committed&&!fresh.state().commit&&!fresh.state().release,"Reordered control created an unoffered cut");}
    check(m.host().state().rejected>=80,"Foreign/duplicate identity attack paths were not exercised");
    std::cout<<"PASS common-cut authenticated sender/target, complete context/mask/seal and delayed/duplicate/foreign ACK rejection\n";
}

void parser_corpus(){Mesh m;const auto first=cut(m.ctx),second=cut(m.ctx,2);m.complete(first);m.offer(second);
    check(m.peers[4]->reject(second),"Parser reject corpus fixture refused");m.pump();
    m.recipients([&](unsigned,auto &p){check(p.acknowledge_abort(second),"Parser abort corpus fixture refused");});m.pump();
    for(unsigned kind=0;kind<types;++kind){const auto found=std::ranges::find_if(m.samples,[&](const auto &entry){return std::get<0>(entry.first)==kind;});
        check(found!=m.samples.end(),"Parser corpus lacks a real control type");const auto &sample=found->second;
        PeerCommonCutAgreement recipient(sample.to,m.ctx);
        for(std::size_t n=0;n<sample.bytes.size();++n)recipient.receive(sample.from,std::span(sample.bytes).first(n),m.now);
        auto extra=sample.bytes;extra.push_back(std::byte{});recipient.receive(sample.from,extra,m.now);
        for(auto offset:{7u,8u}){auto future=sample.bytes;future[offset]=std::byte{255};recipient.receive(sample.from,future,m.now);}
        auto oversized=sample.bytes;oversized.resize(10000);recipient.receive(sample.from,oversized,m.now);
        check(!recipient.state().pending&&!recipient.state().committed&&!recipient.state().commit&&!recipient.state().release,"Truncated/extended/future packet changed common state");
        check(recipient.state().rejected==113,"Recognized decoder corpus did not exercise every truncation and invalid extent");
        check(!PeerCommonCutAgreement::is_packet(std::span(sample.bytes).first(6))&&PeerCommonCutAgreement::is_packet(sample.bytes),"Control dispatcher extent changed");
    }
    std::cout<<"PASS all nine common-cut controls reject every truncation, extension, oversize and future version/type\n";
}

void host_loss_every_phase(){
    for(unsigned phase=0;phase<9;++phase){Mesh m;const auto old=cut(m.ctx),pending=cut(m.ctx,2);m.complete(old);
        if(phase){m.offer(pending);
            if(phase>=2){check(m.peers[4]->associate(pending),"Host-loss first association refused");m.pump();}
            if(phase>=3){check(m.peers[8]->associate(pending),"Host-loss final association refused");m.pump();}
            if(phase>=4&&phase!=8){check(m.host().confirm_native(pending),"Host-loss native confirmation refused");m.pump();}
            if(phase>=5&&phase!=8){check(m.peers[4]->apply(pending),"Host-loss first apply refused");m.pump();}
            if(phase>=6&&phase!=8){check(m.peers[8]->apply(pending),"Host-loss last apply refused");m.pump();}
            if(phase==7){check(m.peers[4]->release(pending),"Host-loss partial release refused");m.pump();}
            if(phase==8){check(m.host().abort(pending),"Host-loss Abort refused");m.pump();check(m.peers[4]->acknowledge_abort(pending),"Host-loss partial Abort ACK refused");m.pump();}
        }
        std::array<PeerCommonCutState,16> saved;m.recipients([&](unsigned p,auto &node){saved[p]=node.state();});
        m.peers[0].reset();m.wire.clear();m.now+=600'000;m.pump(1000);
        m.recipients([&](unsigned p,auto &node){const auto after=node.state();
            check(same_safety_state(saved[p],after),"Host loss/timeout implicitly committed, released, aborted or shrank a common cut");
            check(after.context==m.ctx&&after.recipients==pending.recipients,"Host loss changed fixed physical generation");
            check(after.committed==(phase==7&&p==4?std::optional(pending):std::optional(old)),"Host loss discarded the last explicitly released local common cut");});
    }
    std::cout<<"PASS common-cut host loss at idle, Offer, partial/all association, Commit, partial/all apply, partial Release and Abort preserves exact retained metadata\n";
}

void lossy_schedules(){
    for(auto host:{0u,4u,8u,12u}){Mesh m(0x1111,host);m.complete(cut(m.ctx));m.complete(cut(m.ctx,2));
        check(m.losses&&m.reorders&&m.duplicates,"Sparse-room fixture did not actually lose/reorder/duplicate controls");}
    Mesh full(0xffff,15);full.complete(cut(full.ctx));full.complete(cut(full.ctx,2));
    check(full.host().state().recipients==0x7fff&&std::popcount(full.host().state().released)==15,"Sixteen-PC room counted couch seats or omitted a physical recipient");
    for(unsigned seed=0;seed<32;++seed){Mesh m;std::mt19937 random(seed);
        for(unsigned round=1;round<=2;++round){const auto value=cut(m.ctx,round);check(m.host().offer(value),"Scheduled fresh offer refused");
            std::array<bool,16> locally_released{};bool finished{};
            for(unsigned step=0;step<600&&!finished;++step){
                if(random()%5==0)m.host().confirm_native(value);
                m.recipients([&](unsigned p,auto &node){auto state=node.state();if(state.pending!=value)return;
                    if(!(state.associated&(1u<<p))&&random()%4==0)check(node.associate(value),"Scheduled local association refused");
                    state=node.state();if(state.commit&&state.associated==value.recipients&&!(state.applied&(1u<<p))&&random()%4==0)check(node.apply(value),"Scheduled local apply refused");
                    state=node.state();if(state.release&&state.applied==value.recipients&&!(state.released&(1u<<p))&&random()%4==0){check(node.release(value),"Scheduled local release refused");locally_released[p]=true;}});
                m.step(true);m.recipients([&](unsigned p,auto &node){if(node.state().committed==value)check(locally_released[p],"Network traffic replaced an explicit local release");});
                finished=m.host().state().committed==value;
                if(!finished)check(!m.host().offer(cut(m.ctx,round+1)),"Scheduled next offer bypassed incomplete retirement");
            }
            check(finished&&m.host().state().released==value.recipients,"Stable lossy schedule failed to finish all physical recipients");
            m.recipients([&](unsigned,auto &node){check(node.state().committed==value,"Completed lossy schedule retained conflicting exact seals");});
        }
    }
    std::cout<<"PASS common-cut bounded four-packet retries with first-datagram loss, reordering/duplicates, four sparse hosts, sixteen physical peers and 32 delayed-owner schedules\n";
}
}
int main()try{constructor_and_local_barriers();abort_and_repair();packet_attacks();parser_corpus();host_loss_every_phase();lossy_schedules();return 0;}
catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

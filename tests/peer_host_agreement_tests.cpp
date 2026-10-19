#include "halo2/peer_host_agreement.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool caught{};try{f();}catch(const std::exception &){caught=true;}check(caught,"Invalid agreement input accepted");}
auto context(std::uint16_t peers=0x1111){AuthorityTransferContext c;c.session[0]=std::byte{0x15};c.host=0;c.peers=peers;c.epoch=2;c.membership_revision=5;return c;}
auto cut(unsigned p,const AuthorityTransferContext &c){return ValidatedAuthorityCut{p,c,{p+1,123,4096,{std::byte(p+1)}}};}
struct Mesh {
    struct Packet {unsigned from,to;std::vector<std::byte> bytes;};
    AuthorityTransferContext ctx;std::array<std::unique_ptr<PeerHostAgreement>,16> peers;
    std::deque<Packet> wire;std::uint64_t now=501;unsigned serial{};
    std::array<std::array<std::array<bool,8>,16>,16> dropped{};
    std::function<bool(const Packet &)> allowed=[](auto &){return true;};
    explicit Mesh(std::uint16_t mask=0x1111,unsigned host=0):ctx(context(mask)){ctx.host=host;for(unsigned p=0;p<16;++p)if(mask&(1u<<p))peers[p]=std::make_unique<PeerHostAgreement>(p,ctx,0);}
    void ready(unsigned p){check(peers[p]->publish_ready(cut(p,ctx)),"Complete native-ready fixture admission failed");}
    void step(bool lossy=false,unsigned budget=128){++now;
        for(unsigned p=0;p<16;++p)if(peers[p])for(auto &out:peers[p]->poll(now,budget)){
            check(out.bytes.size()<=134,"Agreement packet exceeds its bounded datagram extent");
            Packet packet{p,out.peer,std::move(out.bytes)};if(!allowed(packet))continue;
            // Bound loss per route/type. A global every-fifth pattern alone
            // can permanently starve one retransmitted packet at this cadence.
            auto &lost=dropped[p][out.peer][std::to_integer<unsigned>(packet.bytes[8])];
            const bool lose=lossy&&++serial%5==0&&!lost;lost=lose;if(lose)continue;
            if(lossy&&serial%7==0)wire.push_front(packet);else wire.push_back(packet);
            if(lossy&&serial%11==0)wire.push_back(packet);
        }
        while(!wire.empty()){auto p=std::move(wire.front());wire.pop_front();if(peers[p.to]&&allowed(p))peers[p.to]->receive(p.from,p.bytes,now);}
    }
    void pump(unsigned steps=300){for(unsigned i=0;i<steps;++i)step(true);}
    void same(unsigned expected){for(unsigned id=0;id<16;++id)if(const auto &p=peers[id]){const auto state=p->state(now);
        if(!state.decision||state.decision->value.host!=expected||state.decision->value.cut!=cut(expected,ctx).seal)
            std::cerr<<"peer="<<id<<" expected="<<expected<<" chosen="<<(state.decision?state.decision->value.host:99)<<" rejected="<<state.rejected<<" fenced="<<state.fenced<<'\n';
        check(state.decision&&state.decision->value.host==expected&&state.decision->value.cut==cut(expected,ctx).seal,"Peers failed to learn the same exact host/cut");
        check(state.decision->next.session==ctx.session&&state.decision->next.epoch==ctx.epoch+1&&state.decision->next.membership_revision==ctx.membership_revision+1&&
            state.decision->next.peers==std::uint16_t(ctx.peers&~(1u<<ctx.host))&&state.decision->next.host==expected,"Decision changed room or survivor generation");
        check(unsigned(std::popcount(state.decision->voters))>=state.majority&&!(state.decision->voters&(1u<<ctx.host)),"Certificate counted host, couch seats or missing votes");}}
};
void threshold(){
    PeerHostAgreement p(4,context(),1000);check(!p.state(1500).suspected&&p.state(1501).suspected,"Silence threshold must be strictly greater than 500ms");
    check(!p.propose(4,1501),"A peer without a complete world became a candidate");
    check(p.publish_ready(cut(4,context())),"Ready cut refused");
    auto wrong=cut(4,context());++wrong.seal.tick;check(!p.publish_ready(wrong),"A frozen retained cut was replaced");
    wrong=cut(4,context());wrong.context.epoch++;check(!p.publish_ready(wrong),"Wrong native-ready generation admitted");
    check(p.state(1501).majority==2&&p.state(1501).electorate==0x1110,"Physical nonhost majority is incorrect");
    p.observe_host(1501);check(!p.state(2001).suspected&&p.state(2002).suspected,"Real host traffic did not reset suspicion");
    check(!p.propose(4,2001)&&p.propose(4,2002),"Host silence did not gate a new proposal");
    check(!p.state(999999).decision&&p.state(999999).majority==2,"Local peer timeouts shrank the agreed quorum");
    PeerHostAgreement solo(4,context(0x11),0);check(solo.publish_ready(cut(4,context(0x11)))&&solo.propose(4,501),"Two-PC host loss failed");
    check(solo.state(501).decision&&solo.state(501).decision->voters==0x10,"Two-PC survivor must be its one-voter majority");
    solo.observe_host(2000);check(solo.state(2000).fenced&&solo.state(2000).decision,"Late host traffic undid an accepted authority change");
    std::cout<<"PASS strict 500ms suspicion, independently retained cut, fixed physical quorum, two-PC survivor and irreversible generation fence\n";
}
void ordinary(){for(auto host:{0u,4u,8u,12u}){Mesh m(0x1111,host);const auto chosen=(host+4)%16;m.ready(chosen);m.pump();
    check(m.peers[chosen]->propose(chosen,m.now),"Ready candidate could not start");m.pump();m.same(chosen);
    for(auto &p:m.peers)if(p)p->observe_host(m.now);
    m.same(chosen);check(!m.peers[host]->state(m.now).suspected,"Old host suspected its own connection");}
    std::cout<<"PASS complete host/cut agreement and old-host learner under bounded loss, reordering and duplicate votes\n";
}
void partitions(){Mesh m;m.ready(4);m.ready(12);m.pump();m.allowed=[](const auto &p){return (p.from==4)==(p.to==4);};
    check(m.peers[4]->propose(4,m.now)&&m.peers[12]->propose(12,m.now),"Partition proposals refused");m.pump();
    check(!m.peers[4]->state(m.now).decision&&m.peers[4]->state(m.now).majority==2,"A one-voter partition promoted itself");
    check(m.peers[8]->state(m.now).decision&&m.peers[12]->state(m.now).decision,"Survivor majority failed to agree");
    m.allowed=[](const auto &){return true;};m.pump();m.same(12);
    std::cout<<"PASS partition-majority intersection and reconciliation without locally deleting stale voters\n";
}
void healthy_majority(){Mesh m;m.ready(4);m.pump();
    m.peers[8]->observe_host(m.now);m.peers[12]->observe_host(m.now);
    check(m.peers[4]->propose(4,m.now),"Locally silent peer could not verify silence");
    for(unsigned i=0;i<1000;++i){m.peers[8]->observe_host(m.now);m.peers[12]->observe_host(m.now);m.step(true);}
    for(auto &p:m.peers)if(p)check(!p->state(m.now).fenced&&!p->state(m.now).decision,"One peer's silence stripped a healthy host");
    m.pump(1500);m.same(4);
    Mesh even(0x1f);for(auto p:{1u,2u,3u,4u})even.ready(p);even.pump();
    even.allowed=[](const auto &p){return (p.from<=2)==(p.to<=2);};
    check(even.peers[1]->propose(1,even.now)&&even.peers[3]->propose(3,even.now),"Even split fixture refused");even.pump(1000);
    for(auto &p:even.peers)if(p)check(p->state(even.now).majority==3&&!p->state(even.now).decision,"Exactly 50 percent manufactured a strict majority");
    even.allowed=[](const auto &){return true;};check(even.peers[4]->propose(4,even.now),"Healed proposer refused");even.pump(1500);even.same(4);
    Mesh full(0xffff);full.ready(15);full.pump(1000);check(full.peers[15]->propose(15,full.now),"Full room proposer refused");full.pump(2000);full.same(15);
    std::cout<<"PASS healthy-host majority veto, exact 50 percent split, healed membership and sixteen physical peers\n";
}
void prior_value(){Mesh m;m.ready(8);m.ready(12);m.pump();
    // Proposer4 can choose another PC's retained world. Suppress all acceptance
    // announcements: the world may be chosen before any learner knows it.
    m.allowed=[](const auto &p){return p.bytes[8]!=std::byte{6};};
    check(m.peers[4]->propose(8,m.now),"Initial proposal refused");m.pump();
    for(auto &p:m.peers)if(p)check(!p->state(m.now).decision,"Hidden votes unexpectedly learned");
    // The first proposer disappears. Its chosen world remains on PC8. PC12
    // wants its own world, but phase1 must carry PC8's accepted value forward.
    m.peers[4].reset();m.allowed=[](const auto &p){return p.from!=4&&p.to!=4;};
    check(m.peers[12]->propose(12,m.now),"Replacement proposer refused");m.pump(1000);
    m.same(8);
    std::cout<<"PASS interrupted proposer recovery carries highest accepted host/cut instead of conflicting fresh preference\n";
}
void malformed(){Mesh m;m.ready(8);auto out=m.peers[8]->poll(m.now,128);
    auto found=std::ranges::find_if(out,[](const auto &p){return p.peer==4&&p.bytes[8]==std::byte{2};});check(found!=out.end(),"Ready packet missing");
    auto &recipient=*m.peers[4];const auto bytes=found->bytes;
    for(unsigned n=0;n<bytes.size();++n)recipient.receive(8,std::span(bytes).first(n),m.now);
    for(auto offset:{4u,9u,41u,45u,53u,55u,56u,57u}){auto bad=bytes;bad[offset]^=std::byte{1};recipient.receive(8,bad,m.now);}
    recipient.receive(12,bytes,m.now);auto extra=bytes;extra.push_back(std::byte{});recipient.receive(8,extra,m.now);
    check(!recipient.state(m.now).ready[8]&&!recipient.state(m.now).fenced&&!recipient.state(m.now).decision,"Malformed or misattributed bytes changed agreement");
    check(recipient.state(m.now).rejected>30,"Malformed paths were not exercised");
    recipient.receive(8,bytes,m.now);check(recipient.state(m.now).ready[8].has_value(),"Valid packet failed after malformed input");
    auto bad=bytes;bad.resize(10000);recipient.receive(8,bad,m.now);
    check(!recipient.state(m.now).decision,"Oversized bytes manufactured a quorum");
    std::cout<<"PASS strict extents, context/epoch/membership/protocol and authenticated sender/target rejection\n";
}
void parser_corpus(){Mesh m;m.ready(8);m.ready(12);std::array<std::optional<Mesh::Packet>,8> corpus;
    m.allowed=[&](const auto &p){const auto type=std::to_integer<unsigned>(p.bytes[8]);
        if(!corpus[type]||p.bytes.size()>corpus[type]->bytes.size())corpus[type]=p;
        return type!=6;};
    m.pump();check(m.peers[4]->propose(8,m.now),"Parser fixture first proposer refused");m.pump();
    check(m.peers[12]->propose(12,m.now),"Parser fixture retry refused");m.pump(1000);
    check(corpus[4]&&corpus[4]->bytes.size()==134,"Parser corpus lacks a full prior-vote promise");
    for(const auto &sample:corpus){check(sample.has_value(),"Parser corpus is missing a packet type");
        PeerHostAgreement recipient(sample->to,m.ctx,0);
        for(std::size_t n=0;n<sample->bytes.size();++n)recipient.receive(sample->from,std::span(sample->bytes).first(n),m.now);
        auto extra=sample->bytes;extra.push_back(std::byte{});recipient.receive(sample->from,extra,m.now);
        for(auto offset:{7u,8u}){auto future=sample->bytes;future[offset]=std::byte{255};recipient.receive(sample->from,future,m.now);}
        const auto s=recipient.state(m.now);check(!s.fenced&&!s.decision&&std::ranges::none_of(s.ready,[](auto &r){return bool(r);}),"Truncated/extended/future control bytes changed authority state");
        // Prefixes shorter than the seven-byte dispatcher key are unclaimed.
        check(s.rejected==sample->bytes.size()-4,"Control decoder did not exercise every recognized truncation");
    }
    std::cout<<"PASS all eight control types and full prior-vote promise reject every truncation, extension and future version/type\n";
}
void duplicates(){Mesh m;m.ready(8);m.pump();std::array<std::vector<std::byte>,16> receipts;
    m.allowed=[&](const auto &p){if(p.to==0&&p.bytes[8]==std::byte{6}){receipts[p.from]=p.bytes;return false;}return true;};
    check(m.peers[4]->propose(8,m.now),"Receipt fixture refused");m.pump();
    check(!m.peers[0]->state(m.now).decision&&!receipts[4].empty()&&!receipts[8].empty(),"Receipt capture did not isolate the old host learner");
    for(unsigned n=0;n<100;++n){m.peers[0]->receive(4,receipts[4],m.now);m.peers[0]->receive(8,receipts[4],m.now);}
    check(!m.peers[0]->state(m.now).decision,"Duplicate/relabeled votes manufactured a majority");
    m.peers[0]->receive(8,receipts[8],m.now);check(m.peers[0]->state(m.now).decision.has_value(),"Two independently authenticated votes failed to form a majority");
    std::cout<<"PASS duplicate receipts count once and a claimed peer cannot impersonate a second authenticated voter\n";
}
void schedules(){
    for(unsigned seed=0;seed<64;++seed){std::mt19937 random(seed);Mesh m;for(auto p:{4u,8u,12u})m.ready(p);m.pump();
        std::optional<PeerMigrationValue> chosen;
        for(unsigned step=0;step<500;++step){if(step%17==0){const auto proposer=4*(1+random()%3),candidate=4*(1+random()%3);m.peers[proposer]->propose(candidate,m.now);}
            m.step(true);for(auto &p:m.peers)if(p)if(auto d=p->state(m.now).decision){if(chosen)check(*chosen==d->value,"Competing proposers chose conflicting worlds");chosen=d->value;}}
        if(!m.peers[4]->state(m.now).decision)m.peers[4]->propose(4,m.now);
        m.pump(1000);auto d=m.peers[4]->state(m.now).decision;check(d.has_value(),"Stable proposer did not recover after contention");m.same(d->value.host);
    }
    std::cout<<"PASS 64 competing-proposer schedules with lost, duplicated/reordered traffic and stable-proposer recovery\n";
}
void automatic_schedule(){
    PeerHostAgreement threshold_peer(4,context(),1000);
    check(threshold_peer.publish_ready(cut(4,context())),"Automatic threshold fixture refused");
    check(!threshold_peer.drive(4,1500)&&threshold_peer.drive(4,1501),"Automatic driver changed the strict 500ms threshold");
    for(unsigned now=1502;now<2000;++now)check(!threshold_peer.drive(4,now),"Frequent service restarted an in-flight proposal");
    threshold_peer.observe_host(2000);check(!threshold_peer.drive(4,2500),"Automatic driver ignored renewed host traffic");
    check(threshold_peer.drive(4,2501),"Automatic driver did not recover after a temporary suspicion");
    Mesh paced;paced.ready(8);paced.pump();const auto began=paced.now;std::array<unsigned,16> starts{};
    paced.allowed=[&](const auto &p){return p.bytes[8]!=std::byte{4}||p.to!=4||paced.now-began>=400;};
    for(unsigned i=0;i<800;++i){for(auto p:{4u,8u,12u})starts[p]+=paced.peers[p]->drive(8,paced.now);paced.step(true);}
    paced.same(8);check(starts[4]==1&&starts[8]==0&&starts[12]==0,"Duplicate traffic or frequent polling restarted delayed phase1");
    // A vanished coordinator's repeated old prepare must not postpone a
    // successor forever. Its accepted value still has to win phase1.
    Mesh interrupted;interrupted.ready(8);interrupted.ready(12);interrupted.pump();std::vector<Mesh::Packet> prepares;
    interrupted.allowed=[&](const auto &p){if(p.from==4&&p.bytes[8]==std::byte{3})prepares.push_back(p);return p.bytes[8]!=std::byte{6};};
    check(interrupted.peers[4]->drive(8,interrupted.now),"Automatic initial coordinator refused");interrupted.pump(300);
    check(!prepares.empty(),"Automatic prepare fixture missing");
    interrupted.peers[4].reset();interrupted.allowed=[](const auto &p){return p.from!=4&&p.to!=4;};
    for(unsigned i=0;i<2500;++i){
        for(const auto &p:prepares)if(interrupted.peers[p.to])interrupted.peers[p.to]->receive(p.from,p.bytes,interrupted.now);
        for(auto p:{8u,12u})interrupted.peers[p]->drive(12,interrupted.now);
        interrupted.step(true);
    }
    interrupted.same(8);
    Mesh healthy;healthy.ready(8);healthy.pump();
    for(unsigned i=0;i<2500;++i){for(auto p:{8u,12u})healthy.peers[p]->observe_host(healthy.now);
        for(auto p:{4u,8u,12u})healthy.peers[p]->drive(8,healthy.now);healthy.step(true);}
    for(auto &p:healthy.peers)if(p)check(!p->state(healthy.now).fenced&&!p->state(healthy.now).decision,"Automatic minority suspicion replaced a healthy host");
    for(unsigned i=0;i<2500;++i){for(auto p:{4u,8u,12u})healthy.peers[p]->drive(8,healthy.now);healthy.step(true);}
    healthy.same(8);
    Mesh full(0xffff);full.ready(15);full.pump(1000);
    for(unsigned i=0;i<4000;++i){for(unsigned p=1;p<16;++p)full.peers[p]->drive(15,full.now);full.step(true,4);}
    full.same(15);
    for(unsigned seed=0;seed<64;++seed){Mesh competing;std::mt19937 random(seed);for(auto p:{4u,8u,12u})competing.ready(p);competing.pump();
        std::optional<PeerMigrationValue> chosen;
        for(unsigned i=0;i<2500;++i){for(auto p:{4u,8u,12u})competing.peers[p]->drive(4*(1+random()%3),competing.now);
            competing.step(true,4);for(auto &p:competing.peers)if(p)if(auto d=p->state(competing.now).decision){
                if(chosen)check(*chosen==d->value,"Automatic competing drivers chose conflicting worlds");chosen=d->value;}}
        check(chosen.has_value(),"Automatic drivers needed a manual stable proposer");competing.same(chosen->host);
    }
    std::cout<<"PASS automatic paced proposals, delayed promises, failed-proposer retry with duplicate prepares, prior accepted cut preservation and healthy-majority veto\n";
    std::cout<<"PASS sixteen physical peers and 64 varying-preference schedules converge with bounded four-packet polls and no manual proposer\n";
}
void concurrent(){Mesh m;m.ready(4);m.pump();std::exception_ptr error;std::mutex mutex;
    const auto run=[&](auto work){try{for(unsigned i=0;i<10000;++i)work(i);}catch(...){std::lock_guard lock(mutex);error=std::current_exception();}};
    {std::jthread a([&]{run([&](unsigned i){m.peers[4]->observe_host(i);});});
     std::jthread b([&]{run([&](unsigned i){(void)m.peers[4]->state(i);});});
     std::jthread c([&]{run([&](unsigned i){(void)m.peers[4]->poll(i,4);});});
     std::jthread driver([&]{run([&](unsigned i){(void)m.peers[4]->drive(4,i);});});
     std::jthread d([&]{run([&](unsigned i){m.peers[4]->receive(15,std::array<std::byte,7>{std::byte{0x48},std::byte{0x32},std::byte{0x4e},std::byte{0x32},std::byte{},std::byte{},std::byte{10}},i);});});}
    if(error)std::rethrow_exception(error);
    std::cout<<"PASS concurrent host observation, snapshots, bounded polling and untrusted receive\n";
}
}
int main()try{threshold();ordinary();partitions();healthy_majority();prior_value();malformed();parser_corpus();duplicates();schedules();automatic_schedule();concurrent();return 0;}
catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

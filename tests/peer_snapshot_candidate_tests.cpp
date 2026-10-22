#include "halo2/peer_snapshot_candidate.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>
#include <tuple>

using namespace halo2;
namespace {
using Bytes=std::vector<std::byte>;
using Delivery=PeerSnapshotCandidate::Delivery;
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool failed{};try{f();}catch(const std::exception &){failed=true;}check(failed,"Invalid candidate constructor accepted");}
auto context(std::uint16_t mask=0x111,unsigned host=0){AuthorityTransferContext c;c.session[0]=std::byte{0x15};c.epoch=3;c.membership_revision=7;c.peers=mask;c.host=host;return c;}
auto limits(std::uint64_t cap=8192){PeerSnapshotLimits l;l.maximum_archive_bytes=cap;l.retry_ms=10;l.idle_ms=1000;l.window_chunks=3;return l;}
// Actual emitted wire positions, never a replacement certificate encoder.
enum Type:unsigned {Arm,Armed,Offer,Chunk,Ack,Captured,Release,Abort,Released,Aborted,Reject};
unsigned type(const Bytes &p){check(p.size()>=67&&p.size()<=1400,"Candidate packet escaped wire bound");return std::to_integer<unsigned>(p[8]);}
void overwrite(Bytes &p,unsigned at,std::uint64_t value,unsigned n){check(at+n<=p.size(),"Mutation exceeded actual packet");for(unsigned i=0;i<n;++i)p[at+i]=std::byte(value>>(8*i));}
bool same(const PeerSnapshotState &a,const PeerSnapshotState &b){return
    a.context==b.context&&a.recipients==b.recipients&&a.pending==b.pending&&a.armed==b.armed&&a.delivered==b.delivered&&
    a.captured==b.captured&&a.retired==b.retired&&a.rejected==b.rejected&&a.closing==b.closing&&a.published==b.published&&
    a.delivery_outstanding==b.delivery_outstanding&&a.buffered_bytes==b.buffered_bytes&&a.maximum_buffered_bytes==b.maximum_buffered_bytes;}
struct Mesh {
    struct Packet {unsigned from{},to{};Bytes bytes;};
    AuthorityTransferContext ctx;
    std::array<std::unique_ptr<PeerSnapshotCandidate>,16> nodes;
    std::array<Bytes,16> expected;
    std::map<std::tuple<unsigned,unsigned,unsigned>,Packet> samples;
    std::set<std::tuple<unsigned,unsigned,Bytes>> lost_once;
    std::deque<Packet> wire;
    std::function<bool(const Packet &)> allowed=[](const auto &){return true;};
    std::uint64_t now{};unsigned serial{},losses{},reorders{},duplicates{};
    explicit Mesh(std::uint16_t mask=0x111,unsigned host=0,std::uint64_t cap=8192):ctx(context(mask,host)){
        for(unsigned p=0;p<16;++p)if(mask&(1u<<p)){
            nodes[p]=std::make_unique<PeerSnapshotCandidate>(p,ctx,limits(cap));
            check(nodes[p]->self()==p,"Same-context module lost its actual physical endpoint owner");
        }
    }
    auto &host(){return *nodes[ctx.host];}
    template<class F>void recipients(F f){for(unsigned p=0;p<16;++p)if(nodes[p]&&p!=ctx.host)f(p,*nodes[p]);}
    void step(bool loss=true,unsigned budget=4){now+=5;
        for(unsigned p=0;p<16;++p)if(nodes[p]){
            const auto out=nodes[p]->poll(now,budget);check(out.size()<=std::min(budget,64u),"Poll exceeded packet budget");
            for(auto packet:out){check(PeerSnapshotCandidate::is_packet(packet.bytes),"Candidate dispatcher failed its actual packet");
                check(packet.peer<16&&packet.peer!=p&&(ctx.peers&(1u<<packet.peer)),"Packet escaped physical membership");
                Packet value{p,packet.peer,std::move(packet.bytes)};samples.try_emplace(std::tuple{type(value.bytes),p,value.to},value);
                if(!allowed(value))continue;
                if(loss&&lost_once.emplace(value.from,value.to,value.bytes).second){++losses;continue;}
                if(++serial%2){wire.push_front(value);++reorders;}else wire.push_back(value);
                if(serial%3==0){wire.push_back(value);++duplicates;}
            }
        }
        while(!wire.empty()){auto p=std::move(wire.front());wire.pop_front();if(allowed(p))check(nodes[p.to]->receive(p.from,p.bytes,now),"Actual candidate packet not consumed");}
        for(const auto &node:nodes)if(node){const auto s=node->state();check(s.buffered_bytes<=s.maximum_buffered_bytes,"Candidate storage exceeded advertised hard archive bound");}
    }
    template<class F>void wait(F ready,bool loss=true,unsigned budget=4){for(unsigned i=0;i<6000&&!ready();++i)step(loss,budget);check(ready(),"Stable bounded peer schedule did not complete");}
    PeerSnapshotToken arm(){auto t=host().begin(now);check(bool(t),"Host refused fresh token");
        wait([&]{bool all=true;recipients([&](unsigned,auto &n){all&=n.state().pending==t;});return all;});
        check(host().state().armed==0,"Arm requests forged foreground acknowledgments");
        recipients([&](unsigned,auto &n){check(n.acknowledge_arm(*t)&&n.acknowledge_arm(*t),"Explicit Arm refused or not idempotent");});
        wait([&]{return host().state().armed==t->recipients;});return *t;
    }
    std::vector<PeerSnapshotArchive> archives(std::size_t size){std::vector<PeerSnapshotArchive> out;
        recipients([&](unsigned p,auto &){Bytes bytes(size);for(std::size_t i=0;i<size;++i)bytes[i]=std::byte((p*43+i*17+i/13)&255);
            expected[p]=bytes;out.push_back({p,std::move(bytes)});});return out;
    }
    std::array<std::optional<Delivery>,16> deliver(const PeerSnapshotToken &t,std::size_t size=4099){
        check(host().publish(t,archives(size),now),"Exact atomic archive publication refused");
        std::array<std::optional<Delivery>,16> held;
        wait([&]{bool all=true;recipients([&](unsigned p,auto &n){if(!held[p])held[p]=n.take_delivery();all&=bool(held[p]);});return all;});
        recipients([&](unsigned p,auto &n){check(held[p]->token()==t&&held[p]->source()==ctx.host&&held[p]->recipient()==p&&
            std::ranges::equal(held[p]->bytes(),expected[p]),"Opaque native archive changed target or original bytes");
            check(!n.take_delivery()&&n.state().delivery_outstanding,"Delivery lease duplicated or untracked");});
        check(host().state().captured==0&&!host().close(t,PeerSnapshotClose::Release),"Byte delivery forged Captured or Release");return held;
    }
    void retire(const PeerSnapshotToken &t,PeerSnapshotClose mode){
        wait([&]{bool all=true;recipients([&](unsigned,auto &n){all&=n.state().closing==mode;});return all;});
        recipients([&](unsigned,auto &n){const auto before=n.state();
            check(n.can_acknowledge_retired(t,std::nullopt)&&same(before,n.state()),"Empty-lease retirement preflight changed state or refused actual closure");
            check(n.acknowledge_retired(t,std::nullopt)&&n.acknowledge_retired(t,std::nullopt),"Explicit retirement refused or not idempotent");});
        wait([&]{return host().state().retired==t.recipients;});
    }
    void complete(){const auto t=arm();auto held=deliver(t);
        recipients([&](unsigned p,auto &n){check(n.acknowledge_captured(std::move(*held[p]))&&held[p]->bytes().empty(),"Captured failed to consume exact lease");held[p].reset();});
        wait([&]{return host().state().captured==t.recipients;});check(host().state().buffered_bytes==0,"Captured retained host archive bytes");
        check(host().close(t,PeerSnapshotClose::Release),"All explicit Captured acknowledgments did not permit Release");retire(t,PeerSnapshotClose::Release);
    }
    const Packet &sample(unsigned kind,unsigned from,unsigned to)const{const auto i=samples.find({kind,from,to});check(i!=samples.end(),"Actual packet sample absent");return i->second;}
};

void construction_and_bounds(){
    auto c=context();rejects([&]{PeerSnapshotCandidate p(16,c);});rejects([&]{PeerSnapshotCandidate p(1,c);});
    auto bad=c;bad.session={};rejects([&]{PeerSnapshotCandidate p(0,bad);});bad=c;bad.host=16;rejects([&]{PeerSnapshotCandidate p(0,bad);});
    auto l=limits();l.maximum_archive_bytes=0;rejects([&]{PeerSnapshotCandidate p(0,c,l);});l=limits();l.maximum_archive_bytes=9ull*1024*1024+1;rejects([&]{PeerSnapshotCandidate p(0,c,l);});
    l=limits();l.window_chunks=0;rejects([&]{PeerSnapshotCandidate p(0,c,l);});
    Mesh m(0x11,0,2051);
    check(m.host().state().context==m.nodes[4]->state().context&&m.host().self()!=m.nodes[4]->self(),
        "Same-context foreign physical owner cannot be distinguished by adapter");
    check(!m.nodes[4]->begin(0),"Recipient minted host token");
    const auto t=m.host().begin(0);check(t&&!m.host().begin(0),"Second pending round was admitted");
    check(!m.host().publish(*t,m.archives(2051),0),"Publication bypassed foreground Arm");
    m.wait([&]{return m.nodes[4]->state().pending==t;});
    check(!m.host().acknowledge_arm(*t)&&!m.nodes[4]->close(*t,PeerSnapshotClose::Abort),"Wrong role performed foreground/host control");
    check(m.nodes[4]->acknowledge_arm(*t),"Foreground Arm refused");m.wait([&]{return m.host().state().armed==t->recipients;});
    check(!m.host().publish(*t,m.archives(2052),m.now),"Maximum+1 archive allocated");
    auto missing=m.archives(1);missing.clear();check(!m.host().publish(*t,std::move(missing),m.now),"Missing physical recipient accepted");
    auto duplicate=m.archives(1);duplicate.push_back({4,Bytes(1)});check(!m.host().publish(*t,std::move(duplicate),m.now),"Duplicate physical target accepted");
    auto foreign=*t;++foreign.nonce;check(!m.host().publish(foreign,m.archives(1),m.now),"Different nonce published");
    check(!m.host().close(*t,static_cast<PeerSnapshotClose>(9)),"Unknown closing mode accepted");
    auto held=m.deliver(*t,2051);const auto preclose=m.nodes[4]->state();
    check(!m.nodes[4]->can_acknowledge_retired(*t,held[4])&&same(preclose,m.nodes[4]->state())&&held[4]->bytes().size()==2051,
        "Pre-close retirement preflight changed state or consumed delivery");
    check(!m.nodes[4]->acknowledge_retired(*t,std::move(held[4]))&&held[4]&&held[4]->bytes().size()==2051,"False retirement consumed caller delivery");
    check(m.host().poll(m.now,0).empty(),"Zero-budget polling emitted packets");
    const auto before=m.host().state();m.host().poll(m.now-1,0);check(same(before,m.host().state()),"Clock rollback changed protocol safety state");
    check(m.nodes[4]->acknowledge_captured(std::move(*held[4]))&&!m.nodes[4]->acknowledge_captured(std::move(*held[4])),"Consumed Delivery was reusable");
    m.wait([&]{return m.host().state().captured==t->recipients;});check(m.host().close(*t,PeerSnapshotClose::Release),"Release refused");m.retire(*t,PeerSnapshotClose::Release);
    check(m.host().begin(m.now)->nonce>t->nonce,"Retired token nonce was reused");
    std::cout<<"PASS construction, role/foreground barriers, exact/+1 byte cap, atomic physical targets, move-only lease and injected clock budgets\n";
}
void abort_and_reject(){
    // Abort before the first Arm reaches a recipient must still deliver its
    // token; packet receipt alone cannot perform foreground retirement.
    Mesh early;const auto t=early.host().begin(0);check(t&&early.host().close(*t,PeerSnapshotClose::Abort),"Early Abort refused");
    early.wait([&]{bool all=true;early.recipients([&](unsigned,auto &n){all&=n.state().closing==PeerSnapshotClose::Abort;});return all;});
    check(early.host().state().armed==0&&early.host().state().retired==0&&!early.host().begin(100000),"Early Abort fabricated Arm/Retired");
    early.retire(*t,PeerSnapshotClose::Abort);check(early.host().begin(early.now).has_value(),"Retired early Abort blocked next token");
    Mesh m(0x11);const auto token=m.arm();auto held=m.deliver(token);
    Mesh foreign(0x11);const auto ft=foreign.arm();auto foreign_held=foreign.deliver(ft);
    check(m.nodes[4]->reject(token),"Native/capture failure could not request host Abort");
    m.wait([&]{return m.host().state().closing==PeerSnapshotClose::Abort&&m.nodes[4]->state().closing==PeerSnapshotClose::Abort;});
    check(!m.nodes[4]->acknowledge_captured(std::move(*held[4]))&&!held[4]->bytes().empty(),"Abort consumed or falsely captured checked-out delivery");
    const auto before_preflight=m.nodes[4]->state();auto wrong_token=token;++wrong_token.nonce;
    check(!m.host().can_acknowledge_retired(token,std::nullopt)&&
        !m.nodes[4]->can_acknowledge_retired(wrong_token,held[4])&&
        !m.nodes[4]->can_acknowledge_retired(token,std::nullopt)&&
        !m.nodes[4]->can_acknowledge_retired(token,foreign_held[4])&&
        m.nodes[4]->can_acknowledge_retired(token,held[4])&&same(before_preflight,m.nodes[4]->state())&&
        std::ranges::equal(held[4]->bytes(),m.expected[4])&&std::ranges::equal(foreign_held[4]->bytes(),foreign.expected[4]),
        "Retirement preflight accepted wrong owner/token/lease or changed actual bytes/state");
    check(!m.nodes[4]->acknowledge_retired(token,std::nullopt),"Abort retirement omitted checked-out bytes");
    check(!m.nodes[4]->acknowledge_retired(token,std::move(foreign_held[4]))&&foreign_held[4]&&!foreign_held[4]->bytes().empty(),"Foreign module lease retired current candidate");
    m.allowed=[](const auto &p){return type(p.bytes)!=Aborted;};
    check(m.nodes[4]->acknowledge_retired(token,std::move(held[4]))&&!held[4],"Actual Abort lease not consumed");
    for(unsigned n=0;n<100;++n)m.step();m.host().poll(m.now+1'000'000,0);
    check(!m.host().begin(m.now+1'000'000)&&m.host().state().retired==0&&m.host().state().stalled,"Timeout/dropped Retired freed original membership");
    m.allowed=[](const auto &){return true;};m.wait([&]{return m.host().state().retired==token.recipients;});
    check(m.host().begin(m.now).has_value(),"Recovered exact Retired ACK did not free slot");
    // An in-flight partial archive is retired explicitly, with no Delivery.
    Mesh partial(0x11);const auto pt=partial.arm();partial.allowed=[](const auto &p){return type(p.bytes)!=Chunk;};
    check(partial.host().publish(pt,partial.archives(8192),partial.now),"Partial archive publish refused");
    partial.wait([&]{return partial.nodes[4]->state().buffered_bytes==8192;});
    check(partial.host().close(pt,PeerSnapshotClose::Abort),"Partial archive Abort refused");partial.retire(pt,PeerSnapshotClose::Abort);
    check(partial.nodes[4]->state().buffered_bytes==0&&!partial.nodes[4]->take_delivery(),"Partial Abort retained or published bytes");
    std::cout<<"PASS pre-Arm/partial/held Delivery Abort, native reject, foreign lease, dropped retirement and timeout non-retirement\n";
}
void attacks_and_integrity(){
    Mesh m(0x11);const auto t=m.arm();
    const auto arm=m.sample(Arm,0,4).bytes;const auto baseline=m.nodes[4]->state();
    for(unsigned i=0;i<10000;++i){auto p=arm;switch(i%7){
        case 0:p[9]^=std::byte{1};break;case 1:p[41]^=std::byte{1};break;case 2:p[45]^=std::byte{1};break;
        case 3:p[53]^=std::byte{1};break;case 4:p[55]=std::byte{4};break;case 5:p[56]=std::byte{0};break;default:p[57]^=std::byte{1};break;}
        m.nodes[4]->receive(0,p,m.now);m.nodes[4]->receive(4,arm,m.now);m.nodes[4]->receive(1,arm,m.now);
    }
    check(same(baseline,m.nodes[4]->state())&&m.nodes[4]->state().rejected_packets>=30000,"Authentication/context flood grew storage or changed barrier");
    m.allowed=[](const auto &p){return type(p.bytes)!=Offer&&type(p.bytes)!=Chunk;};
    check(m.host().publish(t,m.archives(4099),m.now),"Attack fixture publish refused");m.step(false);
    auto offer=m.sample(Offer,0,4).bytes;auto oversized=offer;overwrite(oversized,67,8193,8);overwrite(oversized,107,9,4);
    m.nodes[4]->receive(0,oversized,m.now);check(m.nodes[4]->state().buffered_bytes==0,"Oversized offer allocated storage");
    m.nodes[4]->receive(0,offer,m.now);const auto admitted=m.nodes[4]->state();
    auto changed_offer=offer;changed_offer[75]^=std::byte{1};m.nodes[4]->receive(0,changed_offer,m.now);
    check(same(admitted,m.nodes[4]->state()),"Conflicting exact-token offer replaced archive");
    auto chunk=m.sample(Chunk,0,4).bytes;m.nodes[4]->receive(0,chunk,m.now);
    auto conflict=chunk;conflict.back()^=std::byte{1};const auto before=m.nodes[4]->state().rejected_packets;m.nodes[4]->receive(0,conflict,m.now);
    check(m.nodes[4]->state().rejected_packets==before+1,"Conflicting duplicate chunk accepted");
    auto out_of_range=chunk;overwrite(out_of_range,67,UINT32_MAX,4);m.nodes[4]->receive(0,out_of_range,m.now);
    for(unsigned n=0;n<offer.size();++n)m.nodes[4]->receive(0,std::span(offer).first(n),m.now);
    auto huge=offer;huge.resize(1401);m.nodes[4]->receive(0,huge,m.now);
    m.allowed=[](const auto &){return true;};std::optional<Delivery> held;
    m.wait([&]{if(!held)held=m.nodes[4]->take_delivery();return bool(held);});
    check(std::ranges::equal(held->bytes(),m.expected[4]),"Rejected conflicting packet corrupted original archive");
    check(m.nodes[4]->acknowledge_captured(std::move(*held))&&m.host().state().captured==0,"Local capture forged host receipt");
    m.wait([&]{return m.host().state().captured==t.recipients;});check(m.host().close(t,PeerSnapshotClose::Release),"Release refused after attack recovery");m.retire(t,PeerSnapshotClose::Release);
    const auto next=m.host().begin(m.now);check(next&&next->nonce>t.nonce,"Successor missing");
    m.wait([&]{return m.nodes[4]->state().pending==next;});const auto successor=m.nodes[4]->state();
    for(unsigned n=0;n<1000;++n){m.nodes[4]->receive(0,arm,m.now);m.nodes[4]->receive(0,offer,m.now);m.nodes[4]->receive(0,chunk,m.now);}
    check(same(successor,m.nodes[4]->state())&&!m.nodes[4]->take_delivery(),"Retired replay revived old archive");
    // A consistent corruption across retransmits must fail the whole digest.
    Mesh corrupt(0x11);const auto ct=corrupt.arm();check(corrupt.host().publish(ct,corrupt.archives(4099),corrupt.now),"Corruption publish refused");
    for(unsigned n=0;n<500&&!corrupt.host().state().closing;++n){corrupt.now+=5;
        for(auto p:corrupt.host().poll(corrupt.now,4)){if(type(p.bytes)==Chunk&&std::to_integer<unsigned>(p.bytes[67])==0)p.bytes.back()^=std::byte{1};corrupt.nodes[4]->receive(0,p.bytes,corrupt.now);}
        for(auto p:corrupt.nodes[4]->poll(corrupt.now,4))corrupt.host().receive(4,p.bytes,corrupt.now);
    }
    check(!corrupt.nodes[4]->take_delivery()&&corrupt.host().state().closing==PeerSnapshotClose::Abort&&corrupt.host().state().captured==0,"Digest corruption reached foreground or forged Captured");
    std::cout<<"PASS actual-packet authenticated role/context flood, bounds/truncation, duplicate conflict, SHA corruption and retired-token replay\n";
}
void physical_rooms(){
    for(unsigned host:{0u,4u,8u,12u}){Mesh m(0x1111,host);m.complete();m.complete();check(m.losses&&m.reorders&&m.duplicates,"Sparse schedule did not actually perturb transport");}
    Mesh full(0xffff,15);full.complete();check(full.host().state().retired==0x7fff,"Sixteen-PC room omitted an original physical recipient");
    std::vector<OnlineAdmission> couch(4);for(unsigned p=0;p<4;++p){couch[p].slot=p;couch[p].peer=0;couch[p].guest=p!=0;}
    OnlineAdmission remote;remote.slot=4;remote.peer=4;couch.push_back(remote);
    std::uint16_t physical{};for(const auto &p:couch)physical|=std::uint16_t(1u<<online_admission_peer(p));
    check(physical==0x11,"Couch fixture did not collapse guest connection owners");Mesh guests(physical);guests.complete();
    check(guests.host().state().recipients==0x10&&guests.host().state().retired==0x10,"Guest seats counted as independent transport ACKs");
    std::cout<<"PASS two successive lossy rounds across sparse host positions, sixteen physical peers and four-player couch collapse\n";
}
void concurrent_abort(){
    const auto c=context(0x11);PeerSnapshotCandidate host(0,c,limits()),client(4,c,limits());
    std::atomic<std::uint64_t> clock{};std::atomic<bool> failed{};std::mutex error_mutex;std::exception_ptr error;
    auto pump=[&](std::stop_token stop,auto &from,auto &to,unsigned source){try{unsigned serial{};while(!stop.stop_requested()){
        const auto now=clock.fetch_add(1);for(auto p:from.poll(now,4)){if(++serial%7==0)continue;to.receive(source,p.bytes,now);if(serial%11==0)to.receive(source,p.bytes,now);}std::this_thread::yield();
    }}catch(...){std::lock_guard lock(error_mutex);error=std::current_exception();failed=true;}};
    std::jthread a([&](std::stop_token stop){pump(stop,host,client,0);});
    std::jthread b([&](std::stop_token stop){pump(stop,client,host,4);});
    auto wait=[&](auto ready){for(unsigned n=0;n<2'000'000&&!ready();++n){if(failed){std::lock_guard lock(error_mutex);std::rethrow_exception(error);}std::this_thread::yield();}check(ready(),"Concurrent bounded schedule did not complete");};
    const auto t=host.begin(clock.load());check(t.has_value(),"Concurrent begin refused");wait([&]{return client.state().pending==t;});
    check(client.acknowledge_arm(*t),"Concurrent Arm refused");wait([&]{return host.state().armed==t->recipients;});
    Bytes payload(8192,std::byte{0x5a});std::vector<PeerSnapshotArchive> batch;batch.push_back({4,payload});check(host.publish(*t,std::move(batch),clock.load()),"Concurrent publish refused");
    std::optional<Delivery> held;wait([&]{if(!held)held=client.take_delivery();return bool(held);});
    check(std::ranges::equal(held->bytes(),payload)&&host.close(*t,PeerSnapshotClose::Abort),"Concurrent bytes/Abort incorrect");
    wait([&]{return client.state().closing==PeerSnapshotClose::Abort;});
    check(!client.acknowledge_captured(std::move(*held))&&!client.acknowledge_retired(*t,std::nullopt)&&held->bytes().size()==payload.size(),"Concurrent Abort lost checked-out lease");
    check(client.acknowledge_retired(*t,std::move(held)),"Concurrent exact retirement refused");wait([&]{return host.state().retired==t->recipients;});
    check(host.begin(clock.load()).has_value(),"Concurrent completed retirement blocked successor");
    std::cout<<"PASS concurrent direct pumping/observation, held-delivery Abort and explicit retirement under loss/reorder duplicates\n";
}
}
int main()try{construction_and_bounds();abort_and_reject();attacks_and_integrity();physical_rooms();concurrent_abort();return 0;}
catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

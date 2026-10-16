#include "halo2/peer_authority_transfer.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}check(rejected,"Invalid authority publication accepted");}
AuthorityTransferContext context(){AuthorityTransferContext c;c.session[0]=std::byte{0x42};c.peers=(1u<<0)|(1u<<4);c.host=0;return c;}
AuthorityTransferLimits limits(){AuthorityTransferLimits l;l.bytes_per_second=16*1024*1024;l.retry_ms=10;l.offer_ms=20;l.idle_ms=5000;l.lifetime_ms=30000;l.window_chunks=32;return l;}
struct Link {
    PeerAuthorityTransfer a{0,context(),limits()},b{4,context(),limits()};
    std::uint64_t now{};unsigned serial{};std::vector<std::byte> old_offer,old_chunk;
    void step(bool loss=true,bool corrupt=false){
        ++now;
        auto deliver=[&](auto &from,auto &to,unsigned source){auto packets=from.poll(now,16);std::reverse(packets.begin(),packets.end());
            for(auto &p:packets){check(p.bytes.size()<=1100,"Authority packet exceeds direct UDP budget");++serial;
                if(source==0&&p.bytes[8]==std::byte{2})old_offer=p.bytes;
                if(source==0&&p.bytes[8]==std::byte{3}){old_chunk=p.bytes;if(corrupt)p.bytes[69]^=std::byte{1};}
                if(loss&&serial%7==0)continue;
                check(to.receive(source,p.bytes,now),"Authority packet was not consumed");
                if(loss&&serial%11==0)to.receive(source,p.bytes,now);
            }
        };
        deliver(a,b,0);deliver(b,a,4);
    }
    void handshake(){for(unsigned i=0;i<3000&&(!a.supported(4)||!b.supported(0));++i)step();check(a.supported(4)&&b.supported(0),"Capabilities did not survive loss");}
};
void concurrent_delivery(){
    PeerAuthorityTransfer a(0,context(),limits()),b(4,context(),limits());
    const auto started=std::chrono::steady_clock::now();
    auto now=[&]{return std::uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-started).count());};
    std::atomic<bool> failed=false;std::mutex error_mutex;std::exception_ptr error;
    auto pump=[&](std::stop_token stop,PeerAuthorityTransfer &from,PeerAuthorityTransfer &to,unsigned source){
        try {unsigned serial=0;while(!stop.stop_requested()&&!failed.load()){
            auto packets=from.poll(now(),16);std::reverse(packets.begin(),packets.end());
            for(auto &p:packets){if(++serial%7==0)continue;to.receive(source,p.bytes,now());
                if(serial%17==0)to.receive(source,p.bytes,now());}
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }}catch(...){std::lock_guard lock(error_mutex);error=std::current_exception();failed.store(true);}
    };
    std::jthread outbound_a([&](std::stop_token stop){pump(stop,a,b,0);});
    std::jthread outbound_b([&](std::stop_token stop){pump(stop,b,a,4);});
    std::jthread observer([&](std::stop_token stop){while(!stop.stop_requested()){
        (void)a.context();(void)b.context();(void)a.supported(4);(void)b.supported(0);
        std::this_thread::yield();
    }});
    auto wait=[&](auto ready){const auto until=now()+10'000;while(!ready()){
        if(failed.load()){std::lock_guard lock(error_mutex);std::rethrow_exception(error);}
        check(now()<until,"Concurrent authority delivery timed out");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }};
    auto transfer=[&](auto &host,auto &client,unsigned target,AuthorityTransferContext binding){
        wait([&]{return host.supported(target);});
        std::vector<std::byte> bytes(1'000'117);for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte(i*29);
        const std::array<unsigned,1> targets{target};auto seal=host.publish(binding,123,bytes,targets,now());
        std::optional<ReceivedAuthorityCut> cut;
        wait([&]{cut=client.take_completed();return bool(cut);});
        check(cut->context==binding&&cut->seal==seal&&*cut->bytes==bytes,"Concurrent transfer changed the complete cut");
        check(host.validated_cuts().empty(),"Concurrent byte delivery acknowledged a world");
        check(client.acknowledge_validated(binding,seal),"Concurrent executor acknowledgement was rejected");
        std::vector<ValidatedAuthorityCut> ready;
        wait([&]{ready=host.validated_cuts();return !ready.empty();});
        check(ready.size()==1&&ready[0].context==binding&&ready[0].seal==seal,"Concurrent acknowledgement changed context");
    };
    transfer(a,b,4,context());
    auto next=context();next.epoch=1;next.host=4;
    check(a.replace_context(context(),next)&&b.replace_context(context(),next),"Concurrent committed context reset failed");
    transfer(b,a,0,next);
    std::cout<<"PASS concurrent bidirectional pumping, publication, executor acknowledgement, observation and authority-context replacement under loss/reorder/duplicates\n";
}
void run(){
    {
        auto enabled=limits();enabled.features=authority_feature_source_replay;
        PeerAuthorityTransfer modern(0,context(),enabled),legacy(4,context(),limits());
        for(unsigned now=0;now<200;++now){for(auto p:modern.poll(now,8))legacy.receive(0,p.bytes,now);
            for(auto p:legacy.poll(now,8))modern.receive(4,p.bytes,now);}
        check(modern.supported(4)&&legacy.supported(0),"Optional replay capability prevented transport admission");
        check(!modern.feature_supported(4,authority_feature_source_replay)&&legacy.feature_supported(0,authority_feature_source_replay),
            "Maximum-only legacy hello advertised replay support");
        check(!legacy.feature_supported(0,authority_feature_source_replay|authority_feature_source_state),
            "Older replay decoder advertised native state journal support");
        check(!legacy.feature_supported(0,authority_feature_source_replay|authority_feature_source_state|authority_feature_source_camera),
            "Older replay decoder advertised native camera journal support");
        check(!legacy.feature_supported(0,authority_feature_source_replay|authority_feature_source_state|authority_feature_source_camera|authority_feature_source_lens_query),
            "Older replay decoder advertised lens query device operands");
        check(!legacy.feature_supported(0,authority_feature_source_replay|authority_feature_source_state|authority_feature_source_camera|authority_feature_source_lens_query|authority_feature_source_lens_draw),
            "Older replay decoder advertised lens drawing and world-pass support");
        check(!legacy.feature_supported(0,authority_feature_source_replay|authority_feature_source_state|authority_feature_source_camera|authority_feature_source_lens_query|authority_feature_source_lens_draw|authority_feature_source_particle_collision),
            "Older replay decoder advertised standalone particle collision queries");
        check(!legacy.feature_supported(0,authority_feature_source_replay|authority_feature_source_state|authority_feature_source_camera|authority_feature_source_lens_query|authority_feature_source_lens_draw|authority_feature_source_particle_collision|authority_feature_source_projectile_contacts),
            "Older replay decoder advertised the shared projectile journal owner");
        auto next=context();++next.epoch;check(legacy.replace_context(context(),next),"Capability generation reset rejected");
        check(!legacy.feature_supported(0,authority_feature_source_replay),"Prior generation replay capability survived reset");
    }
    Link l;l.handshake();std::vector<std::byte> bytes(180'123);for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte((i*37+i/13)&255);
    const std::array<unsigned,1> targets{4};auto cut=l.a.publish(context(),42,bytes,targets,l.now);
    check(!l.b.acknowledge_validated(context(),cut),"Unreceived world was acknowledged");
    std::optional<ReceivedAuthorityCut> completed;
    for(unsigned i=0;i<10000&&!completed;++i){l.step();completed=l.b.take_completed();}
    check(completed&&*completed->bytes==bytes&&completed->seal==cut,"Reliable chunks did not restore complete bytes under loss/reorder/duplicates");
    for(unsigned i=0;i<200;++i)l.step();check(l.a.validated_cuts().empty(),"Byte receipt acknowledged an unvalidated world");
    auto wrong=cut;++wrong.tick;check(!l.b.acknowledge_validated(context(),wrong),"Different world validation was accepted");
    check(l.b.acknowledge_validated(context(),cut),"Executor validation was rejected");
    std::vector<ValidatedAuthorityCut> ready;
    for(unsigned i=0;i<1000&&ready.empty();++i){l.step();ready=l.a.validated_cuts();}
    check(ready.size()==1&&ready[0].peer==4&&ready[0].seal==cut,"Validated cut acknowledgement did not survive loss");
    for(unsigned i=0;i<200;++i)l.step();check(l.a.validated_cuts().empty(),"Duplicate validation produced another receipt");
    // Untrusted packet fields cannot choose the transport's authenticated
    // endpoint, room, target or authority generation.
    l.b.receive(8,l.old_offer,l.now);l.b.receive(4,l.old_offer,l.now);
    auto stale=l.old_offer;stale[9]^=std::byte{1};l.b.receive(0,stale,l.now);
    check(!l.b.take_completed(),"Forged room/source republished a world");
    auto next=context();next.epoch=1;next.host=4;
    check(l.a.replace_context(context(),next)&&l.b.replace_context(context(),next),"Committed authority replacement failed");
    l.b.receive(0,l.old_offer,l.now);l.b.receive(0,l.old_chunk,l.now);check(!l.b.take_completed(),"Old generation revived after authority replacement");
    check(!l.a.replace_context(context(),next),"Stale context replacement accepted");
    rejects([&]{l.a.publish(context(),43,bytes,targets,l.now);});
    Link corrupt;corrupt.handshake();corrupt.a.publish(context(),50,bytes,targets,corrupt.now);
    for(unsigned i=0;i<1500;++i)corrupt.step(false,true);
    check(!corrupt.b.take_completed()&&corrupt.a.validated_cuts().empty(),"Corrupt complete data reached the executor");
    Link expired;expired.handshake();expired.a.publish(context(),60,bytes,targets,expired.now);
    for(unsigned i=0;i<5;++i)expired.step(false);
    const auto offer=expired.old_offer;expired.b.poll(expired.now+6000,0);expired.b.receive(0,offer,expired.now+6001);
    for(unsigned i=0;i<100;++i)expired.b.receive(0,expired.old_chunk,expired.now+6002+i);
    check(!expired.b.take_completed(),"Timed-out transfer identifier was revived");
    PeerAuthorityTransfer alone(0,AuthorityTransferContext{context().session,0,0,1,0});check(alone.poll(0).empty(),"Couch-only room generated a self route");
    concurrent_delivery();
    std::cout<<"PASS bounded peer authority delivery under deterministic loss/reorder/duplicates, explicit validated-world acknowledgements, context/source/timeout/corruption guards and couch-only rooms\n";
}
}
int main()try{run();return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

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
    auto configured=limits();configured.features=authority_feature_source_pipeline;
    PeerAuthorityTransfer a(0,context(),configured),b(4,context(),configured);
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
        wait([&]{return host.delivered_to(binding,seal,target);});
        const auto successor=host.publish_ahead(binding,seal,124,bytes,target,now());
        std::optional<ReceivedAuthorityCut> next;
        wait([&]{next=client.take_completed();return bool(next);});
        check(next->seal==successor&&*next->bytes==bytes,"Concurrent successor changed complete bytes");
        check(client.acknowledge_validated(binding,seal),"Concurrent executor acknowledgement was rejected");
        check(client.acknowledge_validated(binding,successor),"Concurrent successor validation was rejected");
        std::vector<ValidatedAuthorityCut> ready;
        wait([&]{auto r=host.validated_cuts();ready.insert(ready.end(),r.begin(),r.end());return ready.size()==2;});
        check(ready[0].context==binding&&ready[0].seal==seal&&ready[1].context==binding&&ready[1].seal==successor,
            "Concurrent acknowledgement changed context or order");
    };
    transfer(a,b,4,context());
    auto next=context();next.epoch=1;next.host=4;
    check(a.replace_context(context(),next)&&b.replace_context(context(),next),"Concurrent committed context reset failed");
    transfer(b,a,0,next);
    std::cout<<"PASS concurrent bidirectional pumping, publication, executor acknowledgement, observation and authority-context replacement under loss/reorder/duplicates\n";
}
void pipeline_delivery(){
    auto configured=limits();configured.features=authority_feature_source_pipeline;
    PeerAuthorityTransfer host(0,context(),configured),client(4,context(),configured);
    std::uint64_t now{};unsigned serial{};
    auto step=[&](bool drop_ready=false){++now;
        auto send=[&](auto &from,auto &to,unsigned source){auto packets=from.poll(now,16);std::reverse(packets.begin(),packets.end());
            for(auto &p:packets){if(drop_ready&&source==4&&p.bytes[8]==std::byte{5})continue;
                if(++serial%7==0)continue;to.receive(source,p.bytes,now);if(serial%11==0)to.receive(source,p.bytes,now);}};
        send(host,client,0);send(client,host,4);
    };
    auto wait=[&](auto ready,bool drop=false){for(unsigned n=0;n<10'000&&!ready();++n)step(drop);check(ready(),"Two-cut pipeline did not progress under loss");};
    wait([&]{return host.supported(4)&&client.supported(0);});
    check(host.feature_supported(4,authority_feature_source_pipeline),"Pipeline feature was not negotiated");
    const std::array<unsigned,1> targets{4};std::vector<std::byte> first(90'011,std::byte{0x41}),second(80'013,std::byte{0x42});
    const auto one=host.publish(context(),10,first,targets,now);
    rejects([&]{host.publish_ahead(context(),one,11,second,4,now);});
    wait([&]{return host.delivered_to(context(),one,4);});
    const auto two=host.publish_ahead(context(),one,11,second,4,now);
    for(unsigned n=0;n<100;++n)step();
    auto received_one=client.take_completed();
    check(received_one&&received_one->seal==one&&*received_one->bytes==first&&host.validated_cuts().empty(),
        "A successor overwrote unconsumed bytes or manufactured native readiness");
    std::optional<ReceivedAuthorityCut> received_two;
    wait([&]{if(!received_two)received_two=client.take_completed();return received_two&&host.delivered_to(context(),two,4);});
    check(received_two->seal==two&&*received_two->bytes==second&&host.validated_cuts().empty(),"Prefetch changed or validated a world");
    rejects([&]{host.publish_ahead(context(),two,12,first,4,now);});
    auto wrong=one;++wrong.tick;check(!client.acknowledge_validated(context(),wrong),"Pipeline accepted a different completed world");
    check(client.acknowledge_validated(context(),one),"Earlier executor validation was lost to prefetch");
    for(unsigned n=0;n<100;++n)step(true);check(host.validated_cuts().empty(),"Dropped validation still produced readiness");
    check(client.acknowledge_validated(context(),two),"Successor native validation was rejected");
    std::vector<ValidatedAuthorityCut> receipts;
    wait([&]{auto r=host.validated_cuts();receipts.insert(receipts.end(),r.begin(),r.end());return receipts.size()==2;});
    check(receipts[0].seal==one&&receipts[1].seal==two,"Lost Ready recovery reordered validated cuts");
    for(unsigned n=0;n<100;++n)step();check(host.validated_cuts().empty(),"Duplicate pipeline Ready created new receipts");
    const auto three=host.publish_ahead(context(),two,12,first,4,now);
    check(three.id>two.id,"Pipeline reused a transfer identity");
    std::optional<ReceivedAuthorityCut> rejected;
    wait([&]{if(!rejected)rejected=client.take_completed();return bool(rejected);});
    check(!client.reject_delivered(context(),two)&&client.reject_delivered(context(),three)&&
        !client.acknowledge_validated(context(),three),"Native rejection retained a valid-world receipt");
    const auto repaired=host.publish(context(),13,second,targets,now);std::optional<ReceivedAuthorityCut> repair;
    wait([&]{if(!repair)repair=client.take_completed();return bool(repair);});
    check(repair->seal==repaired&&*repair->bytes==second&&client.acknowledge_validated(context(),repaired),
        "Rejected delivered cut blocked complete world repair");
    auto changed=context();++changed.epoch;check(host.replace_context(context(),changed)&&client.replace_context(context(),changed),"Pipeline context replacement failed");
    check(!host.delivered_to(context(),two,4)&&!client.acknowledge_validated(context(),two),"Old pipeline receipt survived context replacement");
    auto old=limits();PeerAuthorityTransfer legacy(4,context(),old),modern(0,context(),configured);
    for(unsigned t=0;t<100;++t){for(auto p:modern.poll(t,16))legacy.receive(0,p.bytes,t);for(auto p:legacy.poll(t,16))modern.receive(4,p.bytes,t);}
    const auto single=modern.publish(context(),1,first,targets,100);
    rejects([&]{modern.publish_ahead(context(),single,2,second,4,100);});
    std::cout<<"PASS negotiated two-cut delivery, withheld executor readiness, bounded third-cut rejection, earlier/native validation under lost Ready, context reset and legacy refusal\n";
}
bool default_credit_fanout(unsigned packet_budget){
    auto binding=context();binding.peers|=std::uint16_t(1u<<8);
    const AuthorityTransferLimits defaults;
    std::array<std::unique_ptr<PeerAuthorityTransfer>,16> owners;
    const std::array<unsigned,3> physical{0,4,8};
    const std::array<unsigned,2> recipients{4,8};
    for(auto peer:physical)owners[peer]=std::make_unique<PeerAuthorityTransfer>(peer,binding);
    std::uint64_t now{};
    struct Receipt {
        std::optional<ReceivedAuthorityCut> cut;
        std::vector<std::uint8_t> chunks;
        std::uint64_t distinct{},first_chunk{},last_chunk{},largest_gap{},completed_at{},delivered_at{};
    };
    std::array<Receipt,16> receipts;
    const auto pump=[&] {
        now+=2;
        for(auto source:physical)for(auto &packet:owners[source]->poll(now,packet_budget)){
            check(packet.peer<owners.size()&&owners[packet.peer],"Fanout packet escaped the fixed physical membership");
            if(source==0&&packet.bytes.size()>=69&&packet.bytes[8]==std::byte{3}){
                auto &receipt=receipts[packet.peer];std::uint32_t index{};
                for(unsigned byte=0;byte<4;++byte)
                    index|=std::to_integer<std::uint32_t>(packet.bytes[65+byte])<<(byte*8);
                check(index<receipt.chunks.size(),"Fanout fragment escaped the actual published cut");
                if(!receipt.chunks[index]){
                    receipt.chunks[index]=1;++receipt.distinct;
                    if(!receipt.first_chunk)receipt.first_chunk=now;
                    else receipt.largest_gap=std::max(receipt.largest_gap,now-receipt.last_chunk);
                    receipt.last_chunk=now;
                }
            }
            // The sender is the established owner used to deliver this real
            // encoded packet. No packet field supplies authentication or ACKs.
            check(owners[packet.peer]->receive(source,packet.bytes,now),"Actual authority packet was not consumed");
        }
    };
    for(unsigned step=0;step<1000;++step){
        bool supported=true;
        for(auto peer:recipients)supported&=owners[0]->supported(peer)&&owners[peer]->supported(0);
        if(supported)break;pump();
    }
    for(auto peer:recipients)check(owners[0]->supported(peer)&&owners[peer]->supported(0),
        "Default-budget fanout did not independently admit both physical recipients");
    std::vector<std::byte> bytes(8u*1024u*1024u+137u);
    for(std::size_t index=0;index<bytes.size();++index)bytes[index]=std::byte((index*37+index/13)&255u);
    const auto seal=owners[0]->publish(binding,123,bytes,recipients,now);
    const auto started=now,total_chunks=(seal.bytes+1023u)/1024u;
    for(auto peer:recipients)receipts[peer].chunks.resize(total_chunks);
    const auto finished=[&]{return std::ranges::all_of(recipients,[&](unsigned peer){
        return receipts[peer].cut&&receipts[peer].delivered_at;});};
    // More than the lossless wire/credit lower bound, still less than one
    // fake minute. Normal default expiry, retry, burst and rate stay intact.
    for(unsigned step=0;step<29'999&&!finished();++step){
        pump();
        for(auto peer:recipients){auto &receipt=receipts[peer];
            if(!receipt.cut)if(auto cut=owners[peer]->take_completed()){
                check(cut->peer==0&&cut->context==binding&&cut->seal==seal&&*cut->bytes==bytes,
                    "Default-budget fanout changed the source, complete cut or exact bytes");
                receipt.cut=std::move(cut);receipt.completed_at=now;
            }
            if(!receipt.delivered_at&&owners[0]->delivered_to(binding,seal,peer))receipt.delivered_at=now;
        }
    }
    check(owners[0]->validated_cuts().empty(),"Fanout byte delivery invented native-world validation");
    const auto charged_bytes=std::uint64_t(recipients.size())*(seal.bytes+69u*total_chunks);
    const auto minimum_ms=((charged_bytes-defaults.burst_bytes)*1000u+defaults.bytes_per_second-1u)/defaults.bytes_per_second;
    check(minimum_ms<59'998,"Fanout fixture observation is shorter than its actual wire/credit lower bound");
    const bool passed=finished();
    std::cout<<(passed?"PASS":"DIAGNOSTIC")<<" default-credit physical fanout: packet_budget="<<packet_budget
        <<" pump_ms=2 cut_bytes="<<seal.bytes<<" recipients="<<recipients.size()
        <<" bytes_per_second="<<defaults.bytes_per_second<<" burst_bytes="<<defaults.burst_bytes
        <<" idle_ms="<<defaults.idle_ms<<" minimum_wire_ms="<<minimum_ms<<" elapsed_ms="<<now-started;
    for(auto peer:recipients){const auto &receipt=receipts[peer];
        const auto gap=receipt.cut?receipt.largest_gap:
            receipt.last_chunk?std::max(receipt.largest_gap,now-receipt.last_chunk):now-started;
        std::cout<<" peer="<<peer<<" distinct_chunks="<<receipt.distinct<<'/'<<total_chunks
            <<" first_ms="<<(receipt.first_chunk?receipt.first_chunk-started:0)
            <<" last_ms="<<(receipt.last_chunk?receipt.last_chunk-started:0)
            <<" largest_gap_ms="<<gap<<" complete="<<bool(receipt.cut)
            <<" complete_ms="<<(receipt.completed_at?receipt.completed_at-started:0)
            <<" byte_ack_ms="<<(receipt.delivered_at?receipt.delivered_at-started:0)
            <<" byte_ack="<<bool(receipt.delivered_at)
            <<" byte_ack_current="<<owners[0]->delivered_to(binding,seal,peer);
    }
    std::cout<<'\n';return passed;
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
    pipeline_delivery();concurrent_delivery();
    const bool default_fanout=default_credit_fanout(4),session_fanout=default_credit_fanout(2);
    check(default_fanout&&session_fanout,"Default-credit multi-recipient delivery starved an independently admitted physical peer");
    std::cout<<"PASS bounded peer authority delivery under deterministic loss/reorder/duplicates, explicit validated-world acknowledgements, context/source/timeout/corruption guards and couch-only rooms\n";
}
}
int main()try{run();return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

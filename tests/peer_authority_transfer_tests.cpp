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
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::invalid_argument &){rejected=true;}check(rejected,"Invalid authority publication accepted or changed exception category");}
AuthorityTransferContext context(){AuthorityTransferContext c;c.session[0]=std::byte{0x42};c.peers=(1u<<0)|(1u<<4);c.host=0;return c;}
AuthorityTransferLimits limits(){AuthorityTransferLimits l;l.bytes_per_second=16*1024*1024;l.retry_ms=10;l.offer_ms=20;l.idle_ms=5000;l.lifetime_ms=30000;l.window_chunks=32;return l;}
struct Link {
    PeerAuthorityTransfer a{0,context(),limits()},b{4,context(),limits()};
    explicit Link(AuthorityTransferLimits configured=limits()):a(0,context(),configured),b(4,context(),configured){}
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
void observed_failures_and_fresh_retry(){
    const std::array<unsigned,1> target{4};const std::vector<std::byte> old_bytes(4097,std::byte{0x31}),fresh_bytes(5003,std::byte{0x72});
    const auto deliver=[&](Link& link,std::uint64_t tick,const auto& bytes){
        link.handshake();const auto seal=link.a.publish(context(),tick,bytes,target,link.now);
        std::optional<ReceivedAuthorityCut> value;
        for(unsigned n=0;n<10000&&(!value||!link.a.delivered_to(context(),seal,4));++n){
            link.step(false);if(!value)value=link.b.take_completed();
        }
        check(value&&value->seal==seal&&*value->bytes==bytes&&link.a.delivered_to(context(),seal,4),
            "Failure/retry control did not deliver its exact immutable byte bank");return seal;
    };
    const auto ready_packets=[&](Link& link,const auto& seal){
        check(link.b.acknowledge_validated(context(),seal),"Test executor could not validate its exact delivered bank");
        std::vector<std::vector<std::byte>> packets;
        for(auto& packet:link.b.poll(link.now,32))if(packet.bytes[8]==std::byte{5})packets.push_back(packet.bytes);
        check(!packets.empty(),"Actual executor validation emitted no real Ready packet");return packets;
    };
    const auto finish=[&](Link& link,const auto& seal){
        check(link.b.acknowledge_validated(context(),seal),"Fresh complete bank did not admit its executor validation");
        std::vector<ValidatedAuthorityCut> received;
        for(unsigned n=0;n<1000&&received.empty();++n){link.step(false);received=link.a.validated_cuts();}
        check(received.size()==1&&received.front().seal==seal&&!link.a.failed_to(context(),seal,4,link.now),
            "Fresh retry was not independently acknowledged or was reported failed after validation");
    };
    Link idle;const auto one=deliver(idle,10,old_bytes);const auto before=idle.now;
    auto foreign=context();++foreign.membership_revision;auto partial=one;++partial.bytes;
    check(!idle.a.failed_to(foreign,one,4,before+6000)&&!idle.a.failed_to(context(),partial,4,before+6000)&&
        !idle.a.failed_to(context(),one,8,before+6000)&&!idle.b.failed_to(context(),one,0,before+6000)&&
        !idle.a.failed_to(context(),one,4,before+4999),"Unknown/foreign failure observation expired or revived an unrelated cut");
    idle.now=before+6000;
    check(idle.a.failed_to(context(),one,4,idle.now)&&!idle.a.delivered_to(context(),one,4)&&idle.a.validated_cuts().empty(),
        "Exact expired send did not report failure without inventing native readiness");
    const auto late=ready_packets(idle,one);
    for(const auto& packet:late)idle.a.receive(4,packet,idle.now);
    check(idle.a.validated_cuts().empty(),"Late Ready revived an expired native send");
    const auto two=deliver(idle,11,fresh_bytes);
    check(two.id>one.id&&idle.a.context()==context(),"Retry changed context or reused a failed transfer identifier");
    for(const auto& packet:late)idle.a.receive(4,packet,idle.now);
    check(idle.a.validated_cuts().empty()&&!idle.a.failed_to(context(),one,4,idle.now),
        "Retired old Ready matched a fresh pending bank");finish(idle,two);

    Link lifetime;const auto full=deliver(lifetime,20,old_bytes);const auto began=lifetime.now;
    for(unsigned n=0;n<7;++n){lifetime.now+=4000;check(lifetime.a.note_native_preparation(context(),full,4,lifetime.now),
        "Lifetime failure control lost its genuine bounded preparation lease");}
    lifetime.now=began+30001;
    check(lifetime.a.failed_to(context(),full,4,lifetime.now),"Repeated progress concealed the original total lifetime expiry");
    const auto expired_ready=ready_packets(lifetime,full);const auto renewed=deliver(lifetime,21,fresh_bytes);
    for(const auto& packet:expired_ready)lifetime.a.receive(4,packet,lifetime.now);
    check(lifetime.a.validated_cuts().empty(),"Overall-expired Ready validated the fresh retry");finish(lifetime,renewed);

    Link rejected;const auto denied=deliver(rejected,30,old_bytes);
    check(rejected.b.reject_delivered(context(),denied),"Exact delivered bank did not accept native preflight rejection");
    rejected.step(false);
    check(rejected.a.failed_to(context(),denied,4,rejected.now)&&rejected.a.validated_cuts().empty(),
        "Authenticated native Nack was not observed as failure");
    finish(rejected,deliver(rejected,31,fresh_bytes));

    auto configured=limits();configured.features=authority_feature_source_pipeline;
    Link pipeline(configured);const auto first=deliver(pipeline,40,old_bytes);
    const auto second=pipeline.a.publish_ahead(context(),first,41,fresh_bytes,4,pipeline.now);
    std::optional<ReceivedAuthorityCut> successor;
    for(unsigned n=0;n<10000&&(!successor||!pipeline.a.delivered_to(context(),second,4));++n){
        pipeline.step(false);if(!successor)successor=pipeline.b.take_completed();}
    check(successor&&successor->seal==second&&*successor->bytes==fresh_bytes,
        "Pipeline failure control lost its independent successor bytes");
    check(pipeline.b.reject_delivered(context(),first),"Pipeline predecessor rejection was refused");pipeline.step(false);
    check(pipeline.a.failed_to(context(),first,4,pipeline.now)&&!pipeline.a.failed_to(context(),second,4,pipeline.now),
        "Retiring predecessor failure falsely acknowledged or failed the independent successor");
    const auto old_successor_ready=ready_packets(pipeline,second);
    const auto third=deliver(pipeline,42,old_bytes);
    check(third.id>second.id&&pipeline.a.context()==context(),"Pipeline retry reset context or serial allocation");
    for(const auto& packet:old_successor_ready)pipeline.a.receive(4,packet,pipeline.now);
    check(pipeline.a.validated_cuts().empty(),"Abandoned pipeline successor Ready promoted a fresh retry");finish(pipeline,third);

    Link incomplete;incomplete.handshake();const std::vector<std::byte> large(180123,std::byte{0x41});
    const auto unreceived=incomplete.a.publish(context(),50,large,target,incomplete.now);
    check(!incomplete.a.failed_to(context(),unreceived,4,incomplete.now),"Live undelivered bank was prematurely failed");
    incomplete.now+=6000;check(incomplete.a.failed_to(context(),unreceived,4,incomplete.now)&&
        incomplete.a.validated_cuts().empty(),"Undelivered idle failure manufactured native readiness");
    std::cout<<"PASS exact failure observation, idle/lifetime/Nack/partial/foreign controls, same-context increasing-serial fresh retry, abandoned pipeline and late Ready isolation\n";
}
void native_preparation_lease(){
    const std::array<unsigned,1> targets{4};const std::vector<std::byte> bytes(4097,std::byte{0x5a});
    auto deliver=[&](Link& link){
        link.handshake();const auto seal=link.a.publish(context(),90,bytes,targets,link.now);
        check(!link.a.note_native_preparation(context(),seal,4,link.now),"Undelivered bytes admitted native construction progress");
        std::optional<ReceivedAuthorityCut> complete;
        for(unsigned i=0;i<10000&&(!complete||!link.a.delivered_to(context(),seal,4));++i){
            link.step(false);if(!complete)complete=link.b.take_completed();
        }
        check(complete&&complete->seal==seal&&*complete->bytes==bytes&&link.a.delivered_to(context(),seal,4),
            "Native construction lease fixture did not deliver its exact complete bytes");return seal;
    };
    Link live;const auto seal=deliver(live);const auto began=live.now;
    auto wrong=seal;++wrong.tick;
    check(!live.a.note_native_preparation(context(),wrong,4,live.now)&&
        !live.a.note_native_preparation(context(),seal,8,live.now)&&
        !live.b.note_native_preparation(context(),seal,0,live.now),"Foreign seal/peer/nonhost renewed native construction");
    auto changed=context();++changed.membership_revision;
    check(!live.a.note_native_preparation(changed,seal,4,live.now),"Foreign admission generation renewed construction");
    for(unsigned i=0;i<6;++i){live.now+=1000;check(live.a.note_native_preparation(context(),seal,4,live.now),
            "Actual delivered native preparation lost its idle lease");live.step(false);}
    check(live.now-began>5000&&live.a.validated_cuts().empty(),"Native preparation pulse manufactured a Ready receipt");
    check(live.b.acknowledge_validated(context(),seal),"Genuine native Ready failed after longer-than-idle preparation");
    std::vector<ValidatedAuthorityCut> ready;
    for(unsigned i=0;i<1000&&ready.empty();++i){live.step(false);ready=live.a.validated_cuts();}
    check(ready.size()==1&&ready.front().seal==seal,"Actual native Ready did not survive construction lease renewal");
    check(!live.a.note_native_preparation(context(),seal,4,live.now),"Already validated world renewed construction");

    Link idle;const auto expired=deliver(idle);auto foreign=expired;foreign.digest.front()^=std::byte{1};
    for(unsigned i=0;i<5;++i){idle.now+=1000;
        check(!idle.a.note_native_preparation(context(),foreign,4,idle.now),"Foreign native seal renewed the real cut's idle lease");}
    ++idle.now;
    check(!idle.a.note_native_preparation(context(),expired,4,idle.now),"Expired native idle lease was revived");
    check(idle.b.acknowledge_validated(context(),expired),"Delivered negative-control bytes disappeared");
    for(unsigned i=0;i<50;++i)idle.step(false);
    check(idle.a.validated_cuts().empty(),"Late native Ready revived expired construction");

    Link bounded;const auto lifetime=deliver(bounded);const auto start=bounded.now;
    for(unsigned i=0;i<7;++i){bounded.now+=4000;check(bounded.a.note_native_preparation(context(),lifetime,4,bounded.now),
        "Live preparation expired before the existing lifetime bound");}
    bounded.now=start+30001;
    check(!bounded.a.note_native_preparation(context(),lifetime,4,bounded.now),"Construction progress extended the total native lifetime");
    check(bounded.a.validated_cuts().empty(),"Unvalidated lifetime control produced native readiness");
    std::cout<<"PASS exact delivered native construction lease beyond idle, no early Ready, foreign/partial/expired rejection and unchanged total lifetime\n";
}
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
    const auto observed_features=host.feature_snapshot();
    check(observed_features.context==host.context()&&
        (observed_features.features[4]&authority_feature_source_pipeline)&&
        observed_features.features[15]==0,"Capability snapshot lost its actual context or unknown-peer barrier");
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
    for(unsigned i=0;i<6;++i){now+=1000;
        check(host.note_native_preparation(context(),one,4,now)&&host.note_native_preparation(context(),two,4,now),
            "Exact pipelined predecessor/successor construction leases were lost");step();}
    check(host.validated_cuts().empty(),"Pipelined construction progress produced native readiness");
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
        check(legacy.feature_snapshot().features[0]==0,"Prior generation capability snapshot survived reset");
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
    native_preparation_lease();observed_failures_and_fresh_retry();pipeline_delivery();concurrent_delivery();
    const bool default_fanout=default_credit_fanout(4),session_fanout=default_credit_fanout(2);
    check(default_fanout&&session_fanout,"Default-credit multi-recipient delivery starved an independently admitted physical peer");
    std::cout<<"PASS bounded peer authority delivery under deterministic loss/reorder/duplicates, explicit validated-world acknowledgements, context/source/timeout/corruption guards and couch-only rooms\n";
}
}
int main()try{run();return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

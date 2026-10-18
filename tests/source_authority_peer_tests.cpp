#include "source_match_rebind_fixture.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_authority_delta.hpp"
#include "halo2/peer_authority_transfer.hpp"
#include "halo2/online_peer_transport.hpp"
#include <chrono>
#include <thread>

namespace {
using ClockTime=std::chrono::steady_clock;
std::uint64_t milliseconds(){return std::chrono::duration_cast<std::chrono::milliseconds>(ClockTime::now().time_since_epoch()).count();}
void run_peer(const char *xbe,const char *map){
    const std::array<unsigned,4> al{0,1,2,3},bl{4,5,6,7};Owner a(xbe,map,al,1000),b(xbe,map,bl,900000),workspace(xbe,map,al,500000);
    for(unsigned tick=0;tick<20;++tick){std::array<Controls,16> input{};for(unsigned id=0;id<player_count;++id)input[id].yaw=a.match->unit(id).yaw;
        input[0].fire=tick<12;input[0].reload=tick==12;input[0].grenade=tick>=17;a.step(input);}
    const auto admitted=admissions(a,bl); // Trusted fixture admission, separate from received bytes.
    const auto encoded=encode_source_authority(a.authority->checkpoint(binding));
    AuthorityTransferContext context;context.session[0]=std::byte{0x15};context.session[1]=std::byte{0x44};context.peers=(1u<<0)|(1u<<4);context.host=0;
    AuthorityTransferLimits limits;limits.bytes_per_second=16*1024*1024;limits.burst_bytes=32*1024;limits.retry_ms=50;limits.offer_ms=100;limits.lifetime_ms=120000;
    PeerAuthorityTransfer sender(0,context,limits),receiver(4,context,limits);
    OnlinePeerTransport pa(0,{0,4},{}),pb(4,{0,4},{});
    unsigned serial{},media{},control{};auto probe_at=ClockTime::now(),step_at=probe_at;
    std::vector<std::array<Controls,16>> inputs;std::optional<ReceivedAuthorityCut> completed;std::vector<ValidatedAuthorityCut> acknowledged;
    auto pump=[&](bool simulate){
        for(auto signal:pa.signals()){check(signal.peer==4,"Wrong DTLS signal destination");signal.peer=0;pb.signal(signal);}
        for(auto signal:pb.signals()){check(signal.peer==0,"Wrong reverse DTLS signal destination");signal.peer=4;pa.signal(signal);}
        const auto now=milliseconds();
        // Synthetic voice/control-sized native datagrams coexist on the same
        // actual authenticated UDP channel. This is not a headset/Opus proof.
        if(ClockTime::now()-probe_at>=std::chrono::milliseconds(10)){
            std::vector<std::byte> voice(256),command(96);
            for(auto *p:{&voice,&command}){(*p)[0]=std::byte{0x48};(*p)[1]=std::byte{0x32};(*p)[2]=std::byte{0x4e};(*p)[3]=std::byte{0x32};
                (*p)[4]=std::byte(native_multiplayer_protocol);}
            voice[6]=std::byte{6};command[6]=std::byte{2};pa.send(voice,4);pb.send(command,0);probe_at=ClockTime::now();
        }
        auto send=[&](auto &state,auto &transport){auto packets=state.poll(now,16);std::reverse(packets.begin(),packets.end());
            for(auto &packet:packets){++serial;if(serial%17!=0)transport.send(packet.bytes,packet.peer);}};
        send(sender,pa);send(receiver,pb);
        std::vector<std::byte> packet;unsigned peer;
        while(pa.receive(packet,peer)){check(peer==4,"Unverified sender endpoint");if(!sender.receive(peer,packet,now)){check(packet.size()==96,"Unexpected control probe");++control;}}
        while(pb.receive(packet,peer)){check(peer==0,"Unverified host endpoint");if(!receiver.receive(peer,packet,now)){check(packet.size()==256,"Unexpected media probe");++media;}}
        if(!completed)completed=receiver.take_completed();
        auto ready=sender.validated_cuts();acknowledged.insert(acknowledged.end(),ready.begin(),ready.end());
        if(simulate&&inputs.size()<30&&ClockTime::now()-step_at>=std::chrono::milliseconds(33)){
            std::array<Controls,16> input{};for(unsigned id=0;id<player_count;++id){input[id].yaw=a.match->unit(id).yaw;input[id].forward=id&1?0:.1f;}
            a.step(input);inputs.push_back(input);step_at=ClockTime::now();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    };
    const auto connected_deadline=ClockTime::now()+std::chrono::seconds(15);
    while(!sender.supported(4)||!receiver.supported(0)){check(ClockTime::now()<connected_deadline,"Encrypted authority capability exchange timed out");pump(false);}
    const auto started=ClockTime::now();const std::array<unsigned,1> targets{4};const auto seal=sender.publish(context,20,encoded,targets,milliseconds());
    while(!completed){check(ClockTime::now()-started<std::chrono::seconds(90),"Encrypted authority transfer timed out");pump(true);}
    check(completed->seal==seal&&*completed->bytes==encoded&&acknowledged.empty(),"Received data differs or claimed world validation early");
    check(!inputs.empty()&&media>5&&control>5,"Authority bulk delivery stalled source ticks or concurrent datagrams");
    const auto decoded=decode_source_authority(*completed->bytes);
    auto prepared=b.authority->prepare_with_local_views(decoded,binding,admitted,*workspace.authority);
    check(b.authority->commit(std::move(prepared)),"Encrypted source cut did not restore to four new local views");
    check(receiver.acknowledge_validated(context,seal),"Prepared native world did not acknowledge its exact cut");
    const auto ack_deadline=ClockTime::now()+std::chrono::seconds(10);
    while(acknowledged.empty()){check(ClockTime::now()<ack_deadline,"Validated world acknowledgement timed out");pump(false);}
    check(acknowledged.size()==1&&acknowledged[0].peer==4&&acknowledged[0].seal==seal,"Different world was acknowledged");
    // Input catch-up here is an explicit test log, not a live network replay
    // protocol. It proves which owner that later protocol must preserve.
    for(const auto &input:inputs)b.step(input);same_gameplay(a,b);local_views(b,bl);
    for(unsigned tick=0;tick<40;++tick){std::array<Controls,16> input{};
        for(unsigned id=0;id<player_count;++id){input[id].yaw=a.match->unit(id).yaw;input[id].forward=id&1?0:.1f;}
        a.step(input);b.step(input);same_gameplay(a,b);}
    std::cout<<"PASS actual DTLS encrypted direct UDP complete TU1.5 source archive: "<<encoded.size()<<" bytes, deliberate packet loss, "
        <<inputs.size()<<" source ticks during delivery, "<<media<<" media-sized and "<<control<<" control-sized datagrams, exact executor validation, four local views 0/1/2/3 -> 4/5/6/7, declared input catch-up and forty future matching complete live object bodies\n";
    // Retain the sender's canonical image as the base. Recapturing the locally
    // rebound world would change its view bytes and cannot supply this base.
    auto retained_image=AuthorityArchiveImage::read(encoded);
    for(unsigned round=0;round<2;++round){
        for(unsigned tick=0;tick<3;++tick){std::array<Controls,16> input{};
            for(unsigned id=0;id<player_count;++id)input[id].yaw=a.match->unit(id).yaw;
            input[round].fire=tick<2;a.step(input);}
        // This actual source per-frame owner runs outside Match::step. A
        // complete canonical delta retains its bytes without inventing a
        // control replay that omitted that phase.
        a.world->advance_effect_frame(1.f/60.f);
        const auto capture_started=ClockTime::now();
        const auto next_encoded=encode_source_authority(a.authority->checkpoint(binding));
        const auto next_image=AuthorityArchiveImage::read(next_encoded);
        const auto delta=encode_authority_delta(retained_image,next_image);
        const auto capture_ms=std::chrono::duration_cast<std::chrono::milliseconds>(ClockTime::now()-capture_started).count();
        check(delta.size()<next_encoded.size()/4,"Complete source delta did not reuse immutable world regions");
        completed.reset();acknowledged.clear();inputs.clear();
        const auto next_seal=sender.publish(context,a.match->simulation().ticks(),delta,targets,milliseconds());
        const auto delta_started=ClockTime::now();
        while(!completed){check(ClockTime::now()-delta_started<std::chrono::seconds(30),"Encrypted complete delta timed out");pump(true);}
        const auto delivery_ms=std::chrono::duration_cast<std::chrono::milliseconds>(ClockTime::now()-delta_started).count();
        check(completed->seal==next_seal&&acknowledged.empty(),"Delta byte receipt claimed a prepared source world");
        const auto prepare_started=ClockTime::now();
        const auto reconstructed=apply_authority_delta(retained_image,*completed->bytes);
        check(reconstructed.digest()==next_image.digest()&&reconstructed.encode()==next_encoded,"Complete source delta changed canonical state");
        const auto next_decoded=decode_source_authority(reconstructed.encode());
        workspace.local_views(al);
        auto next_prepared=b.authority->prepare_with_local_views(next_decoded,binding,admitted,*workspace.authority);
        check(b.authority->commit(std::move(next_prepared)),"Complete source delta did not restore the rebound world");
        const auto prepare_ms=std::chrono::duration_cast<std::chrono::milliseconds>(ClockTime::now()-prepare_started).count();
        check(receiver.acknowledge_validated(context,next_seal),"Prepared delta world acknowledgement failed");
        const auto ready_deadline=ClockTime::now()+std::chrono::seconds(10);
        while(acknowledged.empty()){check(ClockTime::now()<ready_deadline,"Prepared delta acknowledgement timed out");pump(false);}
        check(acknowledged.size()==1&&acknowledged[0].seal==next_seal,"Different complete delta was acknowledged");
        for(const auto &input:inputs)b.step(input);same_gameplay(a,b);local_views(b,bl);
        for(unsigned tick=0;tick<10;++tick){std::array<Controls,16> input{};
            for(unsigned id=0;id<player_count;++id)input[id].yaw=a.match->unit(id).yaw;
            a.step(input);b.step(input);same_gameplay(a,b);}
        std::cout<<"PASS complete TU1.5 delta round "<<round+1<<": "<<next_encoded.size()<<" full bytes -> "<<delta.size()
            <<" encrypted delta bytes; capture/encode="<<capture_ms<<"ms, UDP delivery="<<delivery_ms<<"ms, resolve/prepare/commit="<<prepare_ms
            <<"ms; actual source frame outside Match::step, retained sender base, four rebound local views and ten future matching complete object-body ticks\n";
        retained_image=next_image;
    }
}
}
int main(int argc,char **argv)try{
    std::cout<<std::unitbuf;if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    run_peer(argv[1],argv[2]);return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

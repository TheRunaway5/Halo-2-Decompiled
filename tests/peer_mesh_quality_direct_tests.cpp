#include "halo2/peer_mesh_quality.hpp"
#include "halo2/online_peer_transport.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace halo2;
using namespace std::chrono_literals;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
constexpr std::array<unsigned,4> peers{0,4,8,12};
std::uint64_t now(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
}
int main()try{
    AuthorityTransferContext context;context.session[0]=std::byte{17};context.host=16;
    std::array<std::unique_ptr<OnlinePeerTransport>,16> transports;
    std::array<std::unique_ptr<PeerMeshQuality>,16> quality;
    for(auto p:peers)context.peers|=1u<<p;
    for(auto p:peers){transports[p]=std::make_unique<OnlinePeerTransport>(p,std::vector<unsigned>(peers.begin(),peers.end()),std::vector<std::string>{});
        quality[p]=std::make_unique<PeerMeshQuality>(p,context);}
    const auto deadline=now()+20'000;std::uint64_t last_media{};unsigned media_received{},probe_packets{};
    bool done=false;
    while(!done){
        check(now()<deadline,"Actual four-PC direct quality test timed out");done=true;
        for(auto p:peers){
            for(auto signal:transports[p]->signals()){const auto to=signal.peer;signal.peer=p;transports[to]->signal(signal);}
            std::uint16_t ready=0;for(auto other:peers)if(other!=p&&transports[p]->ready(other))ready|=1u<<other;
            std::vector<std::byte> packet;unsigned from{};
            while(transports[p]->receive(packet,from)){
                if(!quality[p]->receive(from,packet,now())){check(packet==std::vector<std::byte>(200,std::byte{0x5a}),"QoS disturbed parallel media bytes");++media_received;}
            }
            for(auto packet:quality[p]->poll(ready,now(),16)){
                if(std::to_integer<unsigned>(packet.bytes[6])==3)++probe_packets;
                transports[p]->send(packet.bytes,packet.peer);
            }
            if(now()-last_media>=20)for(auto other:peers)if(ready&(1u<<other))transports[p]->send(std::vector<std::byte>(200,std::byte{0x5a}),other);
            if(quality[p]->candidates(now()).size()!=peers.size())done=false;
        }
        if(now()-last_media>=20)last_media=now();
        std::this_thread::sleep_for(2ms);
    }
    check(media_received>=12&&probe_packets==4*3*64,"Actual probes or simultaneous media missing");
    for(auto p:peers){const auto local=quality[p]->local(now());
        for(auto other:peers)if(other!=p)check(local->links[other].measured&&local->links[other].upload_bytes_per_second>0,"Direct measured upload missing");}
    // A local timeout cannot shrink the agreed generation's membership and
    // elect a host. Require a new externally agreed context before measuring
    // the three survivors; no gameplay role is promoted by this fixture.
    transports[12].reset();
    quality[0]->poll(context.peers&~(1u<<0)&~(1u<<12),now(),0);
    const auto remaining=quality[0]->candidates(now());
    check(std::none_of(remaining.begin(),remaining.end(),[](const auto &p){return p.peer==0||p.peer==12;}),
        "A broken reciprocal physical path remained eligible");
    check(quality[0]->candidates(now()+5001).empty(),"Departed peer reports did not expire");
    std::cout<<"PASS actual four-peer ICE/DTLS/UDP: 768 upload probes, reciprocal quality, parallel media ("<<media_received<<" frames), physical departure invalidation\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

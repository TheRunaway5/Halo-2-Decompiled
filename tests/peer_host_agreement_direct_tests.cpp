#include "halo2/peer_host_agreement.hpp"
#include "halo2/online_peer_transport.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace halo2;
using namespace std::chrono_literals;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
auto now(){return std::uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
}
int main()try{
    constexpr std::array<unsigned,4> slots{0,4,8,12};const std::vector<unsigned> peers(slots.begin(),slots.end());
    AuthorityTransferContext c;c.session.front()=std::byte{0x71};c.epoch=3;c.membership_revision=7;c.host=0;c.peers=0x1111;
    std::array<std::unique_ptr<OnlinePeerTransport>,16> transport;
    std::array<std::unique_ptr<PeerHostAgreement>,16> agreement;
    for(auto p:peers){transport[p]=std::make_unique<OnlinePeerTransport>(p,peers,std::vector<std::string>{});agreement[p]=std::make_unique<PeerHostAgreement>(p,c,now());}
    const auto deadline=now()+20'000;
    bool all=false;
    while(!all){check(now()<deadline,"Direct agreement handshake timeout");all=true;
        for(auto p:peers){for(auto signal:transport[p]->signals()){const auto to=signal.peer;signal.peer=p;transport[to]->signal(signal);}
            for(auto q:peers)if(p!=q&&!transport[p]->ready(q))all=false;}
        std::this_thread::sleep_for(2ms);
    }
    const auto host_seen=now();for(auto p:peers)agreement[p]->observe_host(host_seen);
    const auto retained=ValidatedAuthorityCut{4,c,{11,1234,8192,{std::byte{0x41}}}};
    check(agreement[4]->publish_ready(retained),"Retained native-ready metadata refused");
    transport[0].reset(); // Actual original-host socket/DTLS shutdown.
    bool proposed=false;unsigned packets{},media{};const auto expires=now()+15'000;std::uint64_t last_media{};
    for(;;){bool done=true;check(now()<expires,"Direct survivor agreement timeout");
        for(auto p:{4u,8u,12u}){
            std::vector<std::byte> bytes;unsigned source{};
            while(transport[p]->receive(bytes,source)){
                if(!agreement[p]->receive(source,bytes,now())){check(bytes==std::vector<std::byte>(200,std::byte{0x5a}),"Agreement disturbed parallel player bytes");++media;}}
            for(auto &packet:agreement[p]->poll(now(),16))if(packet.peer!=0){check(transport[p]->send(packet.bytes,packet.peer),"Surviving direct path unavailable");++packets;}
            if(now()-last_media>=20)for(auto q:{4u,8u,12u})if(q!=p)transport[p]->send(std::vector<std::byte>(200,std::byte{0x5a}),q);
            if(!agreement[p]->state(now()).decision)done=false;
        }
        if(now()-last_media>=20)last_media=now();
        if(!proposed&&now()-host_seen>500)proposed=agreement[4]->propose(4,now());
        if(done)break;
        std::this_thread::sleep_for(2ms);
    }
    check(proposed&&packets>0&&media>6,"Direct election or parallel payload path missing");
    for(auto p:{4u,8u,12u}){const auto s=agreement[p]->state(now());check(s.decision&&s.fenced&&s.decision->value.host==4&&s.decision->value.cut==retained.seal&&
        s.decision->next.peers==0x1110&&s.decision->next.epoch==4&&s.majority==2,"Survivor chose a different generation, cut or physical quorum");}
    std::cout<<"PASS four real ICE/DTLS/UDP endpoints: actual old-host shutdown, 500ms silence, direct survivor majority on exact retained cut, and "<<media<<" parallel payload frames\n";
    std::cout<<"LIMIT ready world is declared fixture metadata; no native simulation promotion, runtime devices or WAN acceptance\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

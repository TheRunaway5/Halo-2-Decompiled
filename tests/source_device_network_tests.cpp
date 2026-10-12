#include "halo2/multiplayer_network.hpp"
#include "source_device_snapshot_check.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#ifndef _WIN32
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
using namespace halo2;
namespace {
void require(bool value,const char*message){if(!value)throw FormatError(message);}
SlayerRules rules(const Cache&cache){auto r=slayer_rules({"Slayer",0,{}});r.weapon=cache.slayer_starting_weapon(5);return r;}
void roundtrip(const LevelContent&content,const Cache&cache){
    const auto selected=rules(cache);MultiplayerMatch host_match(content,0,cache.spawns(),selected);
    host_match.join("Authority");LanHost host(host_match,0,0x11ba50);
    const auto address="127.0.0.1:"+std::to_string(host.port());LanClient client(address,0x11ba50,1);
    std::array<Controls,multiplayer_capacity> controls{};
    const auto join=[&](LanClient&peer,unsigned count){
        std::array<Controls,4> neutral{};
        for(unsigned i=0;i<64&&!peer.ready();++i){
            peer.send(std::span(neutral).first(count));host.receive(controls);host.broadcast();peer.receive();
            if(!peer.ready())std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(peer.ready(),"actual UDP device peer failed to join");
        test::same_device_snapshot(host_match.state().devices,peer.state().devices);
        test::same_device_events(host_match.state().device_events,peer.state().device_events);
    };
    join(client,1);MultiplayerMatch mirror(content,0,cache.spawns(),selected);
    NativeDeviceEventCursor first_audio(false);
    require(first_audio.observe(client.state().device_events).empty(),"Initial join replayed device sounds");
    mirror.replicate_devices(client.state().devices);
    std::uint32_t control=~0u,gate=~0u;
    for(const auto&[id,o]:host_match.simulation().devices().owners()){
        if(content.devices[o.placement]->control)control=id;
        if(content.devices[o.placement]->machine && content.devices[o.placement]->placement.position_group==0)gate=id;
    }
    require(control!=~0u && gate!=~0u,"actual Zanzibar control/gate network owner missing");
    require(host_match.activate_admitted_control(control).changed,"admitted host control did not change shared group");
    host.broadcast();client.receive();
    test::same_device_snapshot(host_match.state().devices,client.state().devices);
    require(client.state().tick==0 && client.state().devices.groups[0].value==1 &&
        client.state().devices.groups[0].flags==3,"same-tick host group update was not published");
    for(unsigned i=0;i<45;++i)host_match.step(controls);
    host.broadcast();client.receive();test::same_device_snapshot(host_match.state().devices,client.state().devices);
    test::same_device_events(host_match.state().device_events,client.state().device_events);
    const auto opening_audio=first_audio.observe(client.state().device_events);
    require(opening_audio.size()==2,"Actual UDP lost button/opening device events");
    const auto has=[&](std::span<const SourceDeviceWorldEvent> events,std::uint32_t owner,unsigned index){
        const auto *o=host_match.simulation().devices().find(owner);
        const auto tag=content.devices[o->placement]->motion.effects[index];
        return std::count_if(events.begin(),events.end(),[&](const auto&e){return e.handle==owner && e.tag==tag;})==1;
    };
    require(has(opening_audio,control,0) && has(opening_audio,gate,0),"UDP changed authored device tag/owner");
    require(first_audio.observe(client.state().device_events).empty(),"Duplicate UDP bank replayed device sounds");
    mirror.replicate_devices(client.state().devices);
    test::same_device_snapshot(client.state().devices,mirror.simulation().devices().snapshot(0));
    const auto*opening=mirror.simulation().devices().find(gate);
    require(opening && opening->motion.position>0 && opening->motion.position<1 && opening->motion.position_velocity>0,
        "actual UDP lost moving gate channels/velocity");
    require(mirror.simulation().devices().groups()[0].value==1 && mirror.simulation().devices().groups()[0].flags==3,
        "actual UDP lost shared one-use state");
    // Join during motion, with all four local slots sharing this one bank.
    LanClient late(address,0x11ba50,4);join(late,4);
    NativeDeviceEventCursor late_audio(false);
    require(late_audio.observe(late.state().device_events).empty(),"Four-view late join replayed earlier button/gate sounds");
    require(late.slots().size()==4,"late device peer did not retain four local slots");
    require(late.state().devices.owners.size()==client.state().devices.owners.size(),"late device peer lost owners");
    for(unsigned i=0;i<150;++i)host_match.step(controls);
    host.broadcast();client.receive();late.receive();
    const auto expected=host_match.state().devices;
    test::same_device_snapshot(expected,client.state().devices);test::same_device_snapshot(expected,late.state().devices);
    test::same_device_events(host_match.state().device_events,client.state().device_events);
    test::same_device_events(host_match.state().device_events,late.state().device_events);
    const auto settled_audio=first_audio.observe(client.state().device_events);
    const auto late_settled_audio=late_audio.observe(late.state().device_events);
    require(settled_audio.size()==1 && has(settled_audio,gate,2) && late_settled_audio.size()==1 && has(late_settled_audio,gate,2),
        "Moving late join did not hear the later authored gate endpoint exactly once");
    host.broadcast();client.receive();late.receive();
    require(first_audio.observe(client.state().device_events).empty() && late_audio.observe(late.state().device_events).empty(),
        "Repeated settled snapshot replayed device endpoint sounds");
    mirror.replicate_devices(late.state().devices);
    require(mirror.simulation().devices().find(gate)->motion.position==1,"joined client gate did not reach authority endpoint");
    require(!host_match.activate_admitted_control(control).changed,"host one-use gate changed on repeated admitted action");
    bool denied=false;try{mirror.activate_admitted_control(control);}catch(const FormatError&){denied=true;}
    require(denied,"client replica accepted a group write");
    const auto before=mirror.simulation().ticks();denied=false;
    try{mirror.step(controls);}catch(const FormatError&){denied=true;}
    require(denied && mirror.simulation().ticks()==before,"client replica advanced a second device clock");
    // A fresh peer sees the open gate immediately, without ticking/replaying
    // its closed state or requiring the original control event.
    LanClient after(address,0x11ba50,1);join(after,1);
    NativeDeviceEventCursor after_audio(false);
    require(after_audio.observe(after.state().device_events).empty(),"Settled late join replayed historical device sounds");
    test::same_device_snapshot(host_match.state().devices,after.state().devices);
    std::cout<<"Zanzibar actual UDP: moving/settled gate, three exact authored device events, duplicate suppression, one-use authority and one/four-player late joins passed\n";
}
void fragment_recovery(const LevelContent&content,const Cache&cache){
#ifndef _WIN32
    MultiplayerMatch match(content,0,cache.spawns(),rules(cache));
    for(unsigned i=0;i<15;++i)match.join("Device roster "+std::to_string(i)+std::string(80,'x'));
    LanHost host(match,0,0x106790);
    struct Socket {int fd;~Socket(){if(fd>=0)close(fd);}} proxy{::socket(AF_INET,SOCK_DGRAM,0)};
    require(proxy.fd>=0,"device UDP proxy socket failed");sockaddr_in at{};at.sin_family=AF_INET;
    at.sin_addr.s_addr=htonl(INADDR_LOOPBACK);at.sin_port=0;
    require(bind(proxy.fd,reinterpret_cast<sockaddr*>(&at),sizeof(at))==0,"device UDP proxy bind failed");
    socklen_t size=sizeof(at);require(getsockname(proxy.fd,reinterpret_cast<sockaddr*>(&at),&size)==0,"device UDP proxy address failed");
    LanClient client("127.0.0.1:"+std::to_string(ntohs(at.sin_port)),0x106790,1);
    client.send(std::array<Controls,1>{});std::array<char,1500> bytes{};sockaddr_in client_address{};
    size=sizeof(client_address);
    const auto hello=recvfrom(proxy.fd,bytes.data(),bytes.size(),0,reinterpret_cast<sockaddr*>(&client_address),&size);
    require(hello>0,"device UDP proxy did not receive client hello");
    sockaddr_in host_address{};host_address.sin_family=AF_INET;host_address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    host_address.sin_port=htons(host.port());
    require(sendto(proxy.fd,bytes.data(),hello,0,reinterpret_cast<sockaddr*>(&host_address),sizeof(host_address))==hello,
        "device UDP proxy did not relay hello");
    std::array<Controls,16> controls{};host.receive(controls);
    const auto receive=[&](){
        std::vector<std::vector<char>> packets;
        for(;;){sockaddr_in from{};socklen_t from_size=sizeof(from);
            const auto n=recvfrom(proxy.fd,bytes.data(),bytes.size(),MSG_DONTWAIT,reinterpret_cast<sockaddr*>(&from),&from_size);
            if(n<0)break;
            require(n<=1400 && from.sin_port==host_address.sin_port,"device snapshot exceeded datagram bound");
            packets.emplace_back(bytes.begin(),bytes.begin()+n);
        }
        require(packets.size()>=2,"device full roster did not fragment");return packets;
    };
    const auto deliver=[&](const std::vector<char>&packet){
        require(sendto(proxy.fd,packet.data(),packet.size(),0,reinterpret_cast<sockaddr*>(&client_address),sizeof(client_address))==int(packet.size()),
            "device UDP proxy relay failed");client.receive();
    };
    auto initial=receive();for(auto i=initial.rbegin();i!=initial.rend();++i)deliver(*i);
    require(client.ready(),"out-of-order full device snapshot did not join");
    test::same_device_snapshot(match.state().devices,client.state().devices);
    std::uint32_t control=~0u;
    for(const auto&[h,o]:match.simulation().devices().owners())if(content.devices[o.placement]->control)control=h;
    match.activate_admitted_control(control);match.step(controls);host.broadcast();auto missing=receive();
    deliver(missing.front());deliver(missing.front());
    require(client.state().tick==0,"partial/duplicate snapshot changed device bank");
    match.step(controls);host.broadcast();auto next=receive();for(auto i=next.rbegin();i!=next.rend();++i)deliver(*i);
    require(client.state().tick==2,"device snapshot did not recover from packet loss");
    test::same_device_events(match.state().device_events,client.state().device_events);
    require(client.state().device_events.size()==2,"Lost datagram lost retained button/opening events");
    const auto recovered=client.state().devices;test::same_device_snapshot(match.state().devices,recovered);
    for(const auto&p:missing)deliver(p);for(const auto&p:initial)deliver(p);
    test::same_device_snapshot(recovered,client.state().devices);
    match.step(controls);host.broadcast();auto conflicting=receive();
    deliver(conflicting.front());auto corrupt=conflicting.front();corrupt.back()^=1;deliver(corrupt);
    for(unsigned i=1;i<conflicting.size();++i)deliver(conflicting[i]);
    test::same_device_snapshot(recovered,client.state().devices);
    match.step(controls);host.broadcast();auto final=receive();for(const auto&p:final)deliver(p);
    test::same_device_snapshot(match.state().devices,client.state().devices);
    require(client.state().tick==4,"device bank did not recover after conflicting fragments");
    const auto before_bad=client.state().devices;
    match.step(controls);host.broadcast();auto malformed=receive();
    std::sort(malformed.begin(),malformed.end(),[](const auto&a,const auto&b){return std::uint8_t(a.at(27))<std::uint8_t(b.at(27));});
    std::vector<char> body;for(const auto&p:malformed)body.insert(body.end(),p.begin()+31,p.end());
    // Locate the literal native bank prefix by tick/groups and full owner
    // identity, then corrupt one power float. This exercises decode rejection
    // before publication, separately from fragment-header/conflict handling.
    const auto source=match.state().devices;std::vector<char> prefix;
    const auto put=[&](std::uint64_t value,unsigned bytes){for(unsigned i=0;i<bytes;++i)prefix.push_back(char(value>>(i*8)));};
    put(source.tick,8);put(source.groups.size(),4);
    for(const auto&g:source.groups){put(std::bit_cast<std::uint32_t>(g.value),4);put(g.flags,2);}
    put(source.owners.size(),2);const auto&first=source.owners.front();
    put(first.definition,4);put(first.state.handle,4);put(first.state.placement,4);put(first.state.created_tick,8);
    const auto found=std::search(body.begin(),body.end(),prefix.begin(),prefix.end());
    require(found!=body.end(),"native device bank wire prefix not found");
    const auto offset=std::size_t(found-body.begin())+prefix.size()+8;
    require(offset+4<=body.size(),"native device bank power offset outside snapshot");
    for(unsigned i=0;i<4;++i){const auto at=offset+i;malformed[at/1369][31+at%1369]=char(0x7fc00000u>>(i*8));}
    for(const auto&p:malformed)deliver(p);
    require(client.state().tick==4,"nonfinite device bank published a partial snapshot");
    test::same_device_snapshot(before_bad,client.state().devices);
    match.step(controls);host.broadcast();auto valid=receive();for(const auto&p:valid)deliver(p);
    require(client.state().tick==6,"valid device bank did not recover after malformed values");
    test::same_device_snapshot(match.state().devices,client.state().devices);
    std::cout<<"Zanzibar actual UDP fragmentation: atomic publication, reordering, duplicates, loss/stale rejection and conflict recovery passed\n";
#else
    (void)content;(void)cache;
#endif
}
}
int main(){try{
    if(!std::filesystem::exists("assets/disc/maps/zanzibar.map"))return 77;
    Cache cache("assets/disc/maps/zanzibar.map");const auto content=cache.load_content(0,true);
    roundtrip(content,cache);fragment_recovery(content,cache);return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

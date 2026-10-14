#include "halo2/online_peer_transport.hpp"
#include <nlohmann/json.hpp>
#include <poll.h>
#include <chrono>
#include <iostream>
#include <set>
#include <thread>
using namespace halo2;
using Json=nlohmann::json;
using Clock=std::chrono::steady_clock;
int main()try {
  OnlinePeerTransport peer(0,{0,1},{"stun:master.halotoforever.com:3478"});
  const auto end=Clock::now()+std::chrono::seconds(35);auto next=Clock::now();
  unsigned sent{};std::set<unsigned> received;
  while(Clock::now()<end) {
    for(auto &s:peer.signals())std::cout<<Json{{"signal",{{"peer",s.peer},{"type",s.type},{"payload",s.payload},{"mid",s.mid}}}}.dump()<<std::endl;
    pollfd input{0,POLLIN,0};
    if(poll(&input,1,1)>0) {
      std::string line;if(!std::getline(std::cin,line))break;
      const auto command=Json::parse(line);
      if(command.contains("signal")) {const auto &s=command["signal"];peer.signal({1,s["type"],s["payload"],s["mid"]});}
      if(command.contains("stop"))break;
    }
    if(peer.ready(1) && sent<50 && Clock::now()>=next) {
      std::vector<std::byte> packet(1400);packet[0]=std::byte(sent);
      for(unsigned i=1;i<packet.size();++i)packet[i]=std::byte(i^sent);
      if(peer.send(packet,1))++sent;
      next=Clock::now()+std::chrono::milliseconds(20);
    }
    std::vector<std::byte> packet;unsigned source{};
    while(peer.receive(packet,source)) {
      if(source!=1 || packet.size()!=1400)throw std::runtime_error("WAN packet endpoint or size differs");
      const auto serial=std::to_integer<unsigned>(packet[0]);
      if(serial>=50)throw std::runtime_error("WAN packet serial differs");
      for(unsigned i=1;i<packet.size();++i)if(packet[i]!=std::byte(i^serial))throw std::runtime_error("WAN payload differs");
      received.insert(serial);
    }
    if(sent==50 && received.size()>=45) {
      std::cout<<Json{{"done",true},{"sent",sent},{"received",received.size()},{"rtt_ms",peer.round_trip_ms(1)}}.dump()<<std::endl;
      return 0;
    }
  }
  throw std::runtime_error("WAN direct UDP did not deliver enough authenticated datagrams");
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

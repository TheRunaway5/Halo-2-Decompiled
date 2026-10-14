#include "halo2/online_peer_transport.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace halo2;
using namespace std::chrono_literals;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void wait(F f){const auto end=std::chrono::steady_clock::now()+10s;while(!f()){check(std::chrono::steady_clock::now()<end,"Direct UDP test timed out");std::this_thread::sleep_for(2ms);}}
}
int main()try {
  OnlinePeerTransport a(0,{0,4},{}),b(4,{0,4},{});
  wait([&]{
    for(auto signal:a.signals()){check(signal.peer==4,"Wrong destination");signal.peer=0;b.signal(signal);}
    for(auto signal:b.signals()){check(signal.peer==0,"Wrong destination");signal.peer=4;a.signal(signal);}
    return a.ready(4)&&b.ready(0);
  });
  // Actual ICE/DTLS/SCTP sockets with the native maximum datagram size. The
  // transport's configured stream is unordered and has zero retransmissions.
  std::vector<std::byte> sent(1400);for(unsigned i=0;i<sent.size();++i)sent[i]=std::byte(i);
  check(a.send(sent,4),"Direct send failed");std::vector<std::byte> received;unsigned peer{};
  wait([&]{return b.receive(received,peer);});check(peer==0&&received==sent,"Direct authenticated bytes differ");
  check(b.send(sent,0),"Reverse direct send failed");wait([&]{return a.receive(received,peer);});check(peer==4&&received==sent,"Reverse bytes differ");
  check(!a.send(sent,1),"Unadmitted endpoint sent data");
  sent.resize(1405);check(!a.send(sent,4),"Oversized packet admitted");
  bool rejected=false;try{a.signal({1,"candidate","candidate:1 1 UDP 1 127.0.0.1 2000 typ host","0"});}catch(const std::exception &){rejected=true;}
  check(rejected,"Unadmitted signaling endpoint accepted");
  rejected=false;try{a.signal({4,"candidate","candidate:1 1 TCP 1 127.0.0.1 2000 typ host tcptype active","0"});}catch(const std::exception &){rejected=true;}
  check(rejected,"TCP candidate accepted");
  rejected=false;try{a.signal({4,"candidate","candidate:1 1 UDP 1 127.0.0.1 2000 typ relay","0"});}catch(const std::exception &){rejected=true;}
  check(rejected,"Relay candidate accepted");
  rejected=false;try{OnlinePeerTransport bad(0,{0,4},{"turn:127.0.0.1:3478"});}catch(const std::exception &){rejected=true;}
  check(rejected,"TURN fallback accepted");
  OnlinePeerTransport couch(0,{0},{});check(!couch.send(sent,0),"Self network route accepted");
  std::cout<<"PASS actual encrypted direct UDP in both directions; admitted peers, packet bounds, TCP/TURN denial\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

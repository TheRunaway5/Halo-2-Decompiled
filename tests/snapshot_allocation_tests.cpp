#include "halo2/multiplayer_network.hpp"
#include <algorithm>
#include <array>
#include <deque>
#include <fstream>
#include <iostream>
#include <stdexcept>
#if defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#endif
using namespace halo2;
namespace {
using Packet=std::vector<std::byte>;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void put(Packet &p,std::uint64_t value,unsigned bytes){for(unsigned i=0;i<bytes;++i)p.push_back(std::byte(value>>(8*i)));}
struct Endpoint final:MultiplayerDatagrams {
  Packet sent;
  std::deque<Packet> incoming;
  void send(std::span<const std::byte> bytes,unsigned peer)override {
    check(peer==1,"Foreign destination");sent.assign(bytes.begin(),bytes.end());
  }
  bool receive(Packet &bytes,unsigned &peer)override {
    if(incoming.empty())return false;
    bytes=std::move(incoming.front());incoming.pop_front();peer=1;return true;
  }
};
}
int main()try{
  auto endpoint=std::make_shared<Endpoint>();
  const std::array<std::int8_t,1> teams{-1};
  LanClient client(endpoint,1,123,teams);
  client.send(std::array<Controls,1>{});
  check(endpoint->sent.size()>=15,"Missing real client admission request");
#if defined(__linux__)
  // The exact receive path must also pass with less spare virtual memory than
  // three declared snapshots. The old eager resize fails this process limit.
  std::ifstream statm("/proc/self/statm");std::uint64_t pages{};statm>>pages;
  check(bool(statm)&&pages,"Cannot measure native fixture virtual memory");
  rlimit limit{};check(getrlimit(RLIMIT_AS,&limit)==0,"Cannot read fixture memory limit");
  const auto bound=pages*std::uint64_t(sysconf(_SC_PAGESIZE))+16u*1024u*1024u;
  limit.rlim_cur=std::min(limit.rlim_max,rlim_t(bound));
  check(setrlimit(RLIMIT_AS,&limit)==0,"Cannot bound fixture memory");
#endif
  constexpr unsigned chunk=1400-35,total=(8*1024*1024/chunk)*chunk+1,parts=(total+chunk-1)/chunk;
  for(unsigned sequence=1;sequence<=500;++sequence){
    Packet tail(endpoint->sent.begin(),endpoint->sent.begin()+15);tail[6]=std::byte{3};
    put(tail,42,8);put(tail,sequence,4);put(tail,parts-1,2);put(tail,parts,2);put(tail,total,4);put(tail,1,1);
    endpoint->incoming.push_back(std::move(tail));client.receive();
    check(client.snapshot_buffered_bytes()<=4,"Tiny tails allocated complete snapshot bodies");
    check(!client.ready(),"Incomplete hostile snapshot published a world");
  }
  std::cout<<"PASS actual LanClient receive: 500 rotating one-byte tails for 8 MiB declarations under a 16 MiB allocation allowance\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

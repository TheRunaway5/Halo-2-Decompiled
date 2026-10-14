#pragma once
#ifndef _WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <cstddef>
#include <stdexcept>
namespace halo2::sandbox_effect_udp_probe {
inline void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
struct ScheduledEffectUdpProxy {
  using Packet=std::vector<std::byte>;
  int fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
  sockaddr_in host{},client{};
  explicit ScheduledEffectUdpProxy(unsigned port) {
    check(fd>=0,"Create real UDP proxy");sockaddr_in bind_at{};
    bind_at.sin_family=AF_INET;bind_at.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    check(bind(fd,reinterpret_cast<sockaddr *>(&bind_at),sizeof(bind_at))==0,"Bind real UDP proxy");
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0)|O_NONBLOCK);
    host.sin_family=AF_INET;host.sin_addr.s_addr=htonl(INADDR_LOOPBACK);host.sin_port=htons(port);
  }
  ~ScheduledEffectUdpProxy(){if(fd>=0)close(fd);}
  unsigned port() const {
    sockaddr_in at{};socklen_t n=sizeof(at);
    check(getsockname(fd,reinterpret_cast<sockaddr *>(&at),&n)==0,"Read real UDP proxy port");return ntohs(at.sin_port);
  }
  std::vector<Packet> pump(bool drop_inputs=false) {
    std::vector<Packet> responses;
    for(unsigned n=0;n<256;++n) {
      Packet p(1401);sockaddr_in from{};socklen_t size=sizeof(from);
      const auto count=recvfrom(fd,p.data(),p.size(),0,reinterpret_cast<sockaddr *>(&from),&size);
      if(count<=0)break;p.resize(count);
      if(from.sin_port==host.sin_port && from.sin_addr.s_addr==host.sin_addr.s_addr)responses.push_back(std::move(p));
      else {
        client=from;
        if(!drop_inputs)check(sendto(fd,p.data(),p.size(),0,reinterpret_cast<const sockaddr *>(&host),sizeof(host))==count,"Forward actual client input");
      }
    }
    return responses;
  }
  void deliver(const std::vector<Packet> &packets,bool reverse,bool duplicate) {
    for(unsigned n=0;n<packets.size();++n) {
      const auto &p=packets[reverse?packets.size()-1-n:n];
      check(p.size()<=1400,"Clock replication exceeded UDP datagram bound");
      for(unsigned copy=0;copy<(duplicate && n==0?2u:1u);++copy)
        check(sendto(fd,p.data(),p.size(),0,reinterpret_cast<const sockaddr *>(&client),sizeof(client))==int(p.size()),"Forward actual snapshot fragment");
    }
  }
};
}
#endif

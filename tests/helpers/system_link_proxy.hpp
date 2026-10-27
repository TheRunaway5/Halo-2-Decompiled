#pragma once
#ifndef _WIN32
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <array>
#include <optional>
#include <vector>
namespace halo2::test {
// Actual UDP delivery control only. Never reads keys, decrypts, edits a native
// body, manufactures a packet or supplies a source/world/session result.
class SystemLinkProxy {
  int socket_=-1;
  sockaddr_in host_{},client_{};
  bool client_known_{};
  std::vector<std::vector<std::byte>> held_host_,held_client_;
public:
  bool hold_host{},hold_client{};
  explicit SystemLinkProxy(unsigned host) {
    socket_=::socket(AF_INET,SOCK_DGRAM,0);if(socket_<0)throw FormatError("Cannot allocate System Link delivery fixture");
    sockaddr_in at{};at.sin_family=AF_INET;at.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(bind(socket_,reinterpret_cast<sockaddr*>(&at),sizeof(at))!=0){close(socket_);throw FormatError("Cannot bind System Link delivery fixture");}
    fcntl(socket_,F_SETFL,fcntl(socket_,F_GETFL,0)|O_NONBLOCK);
    const int buffer=8*1024*1024;setsockopt(socket_,SOL_SOCKET,SO_RCVBUF,&buffer,sizeof(buffer));
    host_=at;host_.sin_port=htons(host);
  }
  ~SystemLinkProxy(){if(socket_>=0)close(socket_);}
  unsigned port()const {sockaddr_in at{};socklen_t size=sizeof(at);getsockname(socket_,reinterpret_cast<sockaddr*>(&at),&size);return ntohs(at.sin_port);}
  void pump() {
    for(unsigned n=0;n<512;++n) {
      std::array<std::byte,1500> bytes{};sockaddr_in from{};socklen_t length=sizeof(from);
      const auto size=recvfrom(socket_,bytes.data(),bytes.size(),0,reinterpret_cast<sockaddr*>(&from),&length);if(size<=0)break;
      const bool from_host=from.sin_addr.s_addr==host_.sin_addr.s_addr && from.sin_port==host_.sin_port;
      if(!from_host){client_=from;client_known_=true;}
      if(!client_known_)continue;
      if(from_host?hold_host:hold_client) {
        auto& bank=from_host?held_host_:held_client_;
        if(bank.size()>=256)throw FormatError("System Link delayed-delivery fixture exceeded its finite packet budget");
        bank.emplace_back(bytes.begin(),bytes.begin()+size);continue;
      }
      const auto& to=from_host?client_:host_;
      sendto(socket_,bytes.data(),size,0,reinterpret_cast<const sockaddr*>(&to),sizeof(to));
    }
  }
  std::array<unsigned,2> replay() {
    const std::array<unsigned,2> count{unsigned(held_host_.size()),unsigned(held_client_.size())};
    for(const auto& p:held_host_)sendto(socket_,p.data(),p.size(),0,reinterpret_cast<const sockaddr*>(&client_),sizeof(client_));
    for(const auto& p:held_client_)sendto(socket_,p.data(),p.size(),0,reinterpret_cast<const sockaddr*>(&host_),sizeof(host_));
    held_host_.clear();held_client_.clear();return count;
  }
};
}
#endif

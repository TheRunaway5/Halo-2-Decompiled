#include "halo2/native_tu15_network_service.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
#ifdef __APPLE__
#include "macos/native_datagram.hpp"
#endif

using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
template<class Exception,class F>void rejects(F &&f) {
    bool rejected=false;try{f();}catch(const Exception &){rejected=true;}
    check(rejected,"Invalid datagram operation was accepted");
}
template<class F>auto until_ready(F &&f) {
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    do {auto value=f();if(value)return value;std::this_thread::yield();}
    while(std::chrono::steady_clock::now()<deadline);
    throw std::runtime_error("Loopback datagram did not arrive before its deadline");
}
void datagram_edges() {
    NativeTu15NetworkService service(native_tu15_network_configuration());
    const NativeIpv4Endpoint loopback{{127,0,0,1},0};
    auto sender=service.open_datagram(loopback),receiver=service.open_datagram(loopback);
    const auto from=sender.local_endpoint(),to=receiver.local_endpoint();
    receiver.set_nonblocking(false);receiver.set_nonblocking(true);
    sender.set_broadcast(true);sender.set_broadcast(false);
    NativeIpv4Endpoint peer{{9,8,7,6},1234};
    const auto unchanged=peer;
    std::array<std::byte,2048> buffer{};
    check(!receiver.receive(buffer,peer) && !receiver.receive({},peer) && peer==unchanged,
          "Empty receive invented a datagram or changed its source endpoint");
    sender.send({},to);
    const auto empty_packet=until_ready([&]{return receiver.receive({},peer);});
    if(empty_packet!=0 || peer!=from)
        std::cerr<<"zero-capacity receive: count="<<*empty_packet<<" source="
                 <<unsigned(peer.address[0])<<'.'<<unsigned(peer.address[1])<<'.'
                 <<unsigned(peer.address[2])<<'.'<<unsigned(peer.address[3])<<':'<<peer.port
                 <<" expected_port="<<from.port<<'\n';
    check(empty_packet==0 && peer==from,
          "Valid zero-byte UDP packet was confused with an empty queue");
    const std::vector<std::byte> payload(2048,std::byte{0xa7});
    for(const auto capacity:{0u,32u}) {
        sender.send(payload,to);peer=unchanged;
        until_ready([&]()->std::optional<bool> {
            try {auto size=receiver.receive(std::span(buffer).first(capacity),peer);
                if(!size)return {};throw std::runtime_error("Truncated UDP packet was accepted");}
            catch(const std::length_error &){return true;}
        });
        check(peer==unchanged && !receiver.receive(buffer,peer),
              "Truncated packet changed the caller's endpoint or stayed queued");
    }
    sender.send(payload,to);
    check(until_ready([&]{return receiver.receive(buffer,peer);})==payload.size() &&
          peer==from && std::ranges::equal(payload,buffer),"Datagram after truncation lost bytes or source endpoint");
    {
        auto collision=service.create_datagram();
        rejects<std::system_error>([&]{collision.bind(to);});
    }
    check(service.live_sockets()==2,"Failed bind did not release its socket owner");
    auto moved=std::move(receiver);
    rejects<std::runtime_error>([&]{receiver.local_endpoint();});
    auto replaced=service.open_datagram(loopback);
    check(service.live_sockets()==3,"Datagram move setup lost a socket");
    replaced=std::move(moved);
    check(service.live_sockets()==2 && replaced.local_endpoint()==to,"Move assignment leaked the prior socket");
    service.close();
    rejects<std::runtime_error>([&]{replaced.receive(buffer,peer);});
    rejects<std::runtime_error>([&]{service.create_datagram();});
    std::cout<<"PASS UDP zero packets, truncation/zero-capacity discard, endpoint preservation, options, bind failure, move ownership and retirement\n";
}
}
int main()try {
#ifdef __APPLE__
    struct Socket {int fd;~Socket(){if(fd>=0)::close(fd);}} socket{native_network_detail::datagram_socket()};
    check((::fcntl(socket.fd,F_GETFD)&FD_CLOEXEC) && (::fcntl(socket.fd,F_GETFL)&O_NONBLOCK),
          "Darwin socket was published without close-on-exec/nonblocking flags");
    check(native_network_detail::address({{127,0,0,1},8123}).sin_len==sizeof(sockaddr_in),
          "Darwin endpoint lost its native sockaddr size");
    std::cout<<"PASS Darwin close-on-exec/nonblocking descriptor and native sockaddr layout\n";
#endif
    datagram_edges();return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

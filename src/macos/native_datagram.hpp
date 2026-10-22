#pragma once
#include "halo2/native_tu15_network_service.hpp"
#include <cerrno>
#include <cstring>
#include <system_error>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace halo2::native_network_detail {
inline int datagram_socket() {
    const int descriptor=::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(descriptor<0)throw std::system_error(errno,std::generic_category(),"Darwin network datagram socket");
    // Darwin does not accept Linux's SOCK_NONBLOCK/SOCK_CLOEXEC type flags.
    // Publish only after both descriptor properties have been installed.
    const int flags=::fcntl(descriptor,F_GETFL);
    if(flags<0 || ::fcntl(descriptor,F_SETFD,FD_CLOEXEC)<0 ||
        ::fcntl(descriptor,F_SETFL,flags|O_NONBLOCK)<0) {
        const int error=errno;::close(descriptor);
        throw std::system_error(error,std::generic_category(),"Darwin network descriptor flags");
    }
    return descriptor;
}
inline sockaddr_in address(NativeIpv4Endpoint value) {
    sockaddr_in out{};out.sin_len=sizeof(out);out.sin_family=AF_INET;out.sin_port=htons(value.port);
    std::memcpy(&out.sin_addr,value.address.data(),4);return out;
}
}

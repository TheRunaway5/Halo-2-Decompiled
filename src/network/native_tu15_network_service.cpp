#include "halo2/native_tu15_network_service.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>
#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>
#endif
#if defined(__APPLE__)
#include "macos/native_datagram.hpp"
#endif

namespace halo2 {
namespace {
// Immutable TU1.5 defaults/minimum/maximum tables at 3EA2D0/3EA320/3EA370.
// Authenticated and compared by oracle_native_tu15_network_configuration.py.
constexpr std::array<std::uint8_t,76> defaults{
0,0,12,8,4,8,64,16,16,4,32,64,10,4,1,10,1,60,0,64,3,9,4,4,4,2,10,60,5,120,2,14,5,3,8,9,5,8,2,20,13,10,3,1,5,5,63,63,30,20,60,5,3,1,8,5,9,1,4,2,4,2,5,20,13,8,4,32,2,64,2,3,2,3,2,3};
constexpr std::array<std::uint8_t,76> minimum{
0,0,4,4,1,4,1,1,1,1,1,1,4,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,4,1,1,1,1,1,1};
constexpr std::array<std::uint8_t,76> maximum{
0,255,255,255,255,255,232,63,63,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,63,63,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255};
#if defined(__linux__)
int datagram_socket() {
    const auto descriptor=::socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC|SOCK_NONBLOCK,IPPROTO_UDP);
    if(descriptor<0)throw std::system_error(errno,std::generic_category(),"Native network datagram socket");
    return descriptor;
}
sockaddr_in address(NativeIpv4Endpoint value) {
    sockaddr_in out{};out.sin_family=AF_INET;out.sin_port=htons(value.port);
    std::memcpy(&out.sin_addr,value.address.data(),4);return out;
}
#elif defined(__APPLE__)
using native_network_detail::datagram_socket;
using native_network_detail::address;
#endif
#if defined(__linux__) || defined(__APPLE__)
NativeIpv4Endpoint endpoint(const sockaddr_in& value) {
    NativeIpv4Endpoint out{};std::memcpy(out.address.data(),&value.sin_addr,4);out.port=ntohs(value.sin_port);return out;
}
#endif
}
NativeTu15NetworkConfiguration native_tu15_network_configuration(std::span<const std::byte> input) {
    NativeTu15NetworkConfiguration out{defaults};
    if(input.empty())return out;
    const auto size=std::to_integer<unsigned>(input.front());
    if(size!=12 && size!=76)return out;
    if(input.size()<size)throw std::invalid_argument("Truncated TU1.5 network startup parameters");
    for(unsigned i=0;i<size;++i) {
        const auto value=std::to_integer<std::uint8_t>(input[i]);
        // Original 3DFDB3..3DFDDC uses the default for zero. Its shared store
        // label receives either the minimum or the maximum in DL.
        out.bytes[i]=!value?defaults[i]:std::clamp(value,minimum[i],maximum[i]);
    }
    return out;
}
struct NativeTu15NetworkState {
    explicit NativeTu15NetworkState(NativeTu15NetworkConfiguration value)
        :configuration(value),private_pool(value.private_pool_bytes()){}
    NativeTu15NetworkConfiguration configuration;
    std::vector<std::byte> private_pool;
    mutable std::mutex mutex;
    std::map<std::uint64_t,int> sockets;
    std::uint64_t next_identity=1;
    bool active=true;
    int descriptor(std::uint64_t identity)const {
        const auto at=sockets.find(identity);
        if(!active || at==sockets.end())throw std::runtime_error("Native network socket owner has retired");
        return at->second;
    }
};
NativeTu15NetworkService::NativeTu15NetworkService(NativeTu15NetworkConfiguration configuration) {
    for(std::size_t i=0;i<configuration.bytes.size();++i)
        if(configuration.bytes[i]<minimum[i] || configuration.bytes[i]>maximum[i])
            throw std::invalid_argument("Native network configuration is outside TU1.5 limits");
    if(configuration.bytes[1])throw std::invalid_argument("Native network startup flags are outside the verified game request domain");
#if defined(__linux__) || defined(__APPLE__)
    // The desktop kernel owns IP/driver initialization. Verify the real socket
    // provider before publishing a context, then retain the native pool and
    // configured budgets. Game sockets are created on demand.
    const auto probe=datagram_socket();::close(probe);
    state_=std::make_shared<NativeTu15NetworkState>(configuration);
#else
    throw std::runtime_error("Native TU1.5 network service platform unavailable");
#endif
}
NativeTu15NetworkService::~NativeTu15NetworkService(){close();}
const NativeTu15NetworkConfiguration& NativeTu15NetworkService::configuration()const{return state_->configuration;}
NativeRuntimeAddress NativeTu15NetworkService::private_pool_address()const {
    std::lock_guard lock(state_->mutex);
    if(!state_->active)throw std::runtime_error("Native network pool owner has retired");
    return NativeRuntimeAddress{reinterpret_cast<std::uintptr_t>(state_->private_pool.data())};
}
NativeTu15Datagram NativeTu15NetworkService::open_datagram(NativeIpv4Endpoint local) {
    auto socket=create_datagram();socket.bind(local);return socket;
}
NativeTu15Datagram NativeTu15NetworkService::create_datagram() {
    std::lock_guard lock(state_->mutex);
    if(!state_->active)throw std::runtime_error("Native network service has retired");
    if(state_->sockets.size()>=state_->configuration.max_sockets())throw std::length_error("TU1.5 native socket budget exhausted");
    if(state_->next_identity==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("Native socket identities exhausted");
#if defined(__linux__) || defined(__APPLE__)
    const auto fd=datagram_socket();
    try {
        const auto receive=int(state_->configuration.receive_buffer_bytes()),send=int(state_->configuration.send_buffer_bytes());
        if(::setsockopt(fd,SOL_SOCKET,SO_RCVBUF,&receive,sizeof(receive)) || ::setsockopt(fd,SOL_SOCKET,SO_SNDBUF,&send,sizeof(send)))
            throw std::system_error(errno,std::generic_category(),"Native network socket buffers");
        const auto identity=state_->next_identity;state_->sockets.emplace(identity,fd);++state_->next_identity;
        return NativeTu15Datagram{state_,identity};
    }catch(...){::close(fd);throw;}
#else
    throw std::runtime_error("Native TU1.5 datagram platform unavailable");
#endif
}
std::size_t NativeTu15NetworkService::live_sockets()const {std::lock_guard lock(state_->mutex);return state_->sockets.size();}
void NativeTu15NetworkService::close()noexcept {
    if(!state_)return;
    std::lock_guard lock(state_->mutex);state_->active=false;
#if defined(__linux__) || defined(__APPLE__)
    for(const auto& [identity,descriptor]:state_->sockets)::close(descriptor);
#endif
    state_->sockets.clear();state_->private_pool.clear();
}
NativeTu15Datagram::NativeTu15Datagram(std::shared_ptr<NativeTu15NetworkState> state,std::uint64_t identity)
    :state_(std::move(state)),identity_(identity){}
NativeTu15Datagram::NativeTu15Datagram(NativeTu15Datagram&& other)noexcept
    :state_(std::move(other.state_)),identity_(std::exchange(other.identity_,0)){}
NativeTu15Datagram& NativeTu15Datagram::operator=(NativeTu15Datagram&& other)noexcept {
    if(this!=&other){close();state_=std::move(other.state_);identity_=std::exchange(other.identity_,0);}return *this;
}
NativeTu15Datagram::~NativeTu15Datagram(){close();}
void NativeTu15Datagram::close()noexcept {
    if(!state_)return;
    std::lock_guard lock(state_->mutex);const auto at=state_->sockets.find(identity_);
    if(at!=state_->sockets.end()) {
#if defined(__linux__) || defined(__APPLE__)
        ::close(at->second);
#endif
        state_->sockets.erase(at);
    }
    identity_=0;
}
NativeIpv4Endpoint NativeTu15Datagram::local_endpoint()const {
    if(!state_)throw std::runtime_error("Native network socket was moved");
    std::lock_guard lock(state_->mutex);const auto fd=state_->descriptor(identity_);
#if defined(__linux__) || defined(__APPLE__)
    sockaddr_in at{};socklen_t size=sizeof(at);
    if(::getsockname(fd,reinterpret_cast<sockaddr*>(&at),&size))throw std::system_error(errno,std::generic_category(),"Native network local endpoint");
    return endpoint(at);
#else
    throw std::runtime_error("Native TU1.5 datagram platform unavailable");
#endif
}
void NativeTu15Datagram::bind(NativeIpv4Endpoint local) {
    if(!state_)throw std::runtime_error("Native network socket was moved");
    std::lock_guard lock(state_->mutex);const auto fd=state_->descriptor(identity_);
#if defined(__linux__) || defined(__APPLE__)
    const auto at=address(local);
    if(::bind(fd,reinterpret_cast<const sockaddr*>(&at),sizeof(at)))throw std::system_error(errno,std::generic_category(),"Native network socket bind");
#else
    throw std::runtime_error("Native TU1.5 datagram platform unavailable");
#endif
}
void NativeTu15Datagram::set_nonblocking(bool enabled) {
    if(!state_)throw std::runtime_error("Native network socket was moved");
    std::lock_guard lock(state_->mutex);const auto fd=state_->descriptor(identity_);
#if defined(__linux__) || defined(__APPLE__)
    int value=enabled?1:0;if(::ioctl(fd,FIONBIO,&value))throw std::system_error(errno,std::generic_category(),"Native network nonblocking mode");
#else
    throw std::runtime_error("Native TU1.5 datagram platform unavailable");
#endif
}
void NativeTu15Datagram::set_broadcast(bool enabled) {
    if(!state_)throw std::runtime_error("Native network socket was moved");
    std::lock_guard lock(state_->mutex);const auto fd=state_->descriptor(identity_);
#if defined(__linux__) || defined(__APPLE__)
    int value=enabled?1:0;if(::setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&value,sizeof(value)))throw std::system_error(errno,std::generic_category(),"Native network broadcast option");
#else
    throw std::runtime_error("Native TU1.5 datagram platform unavailable");
#endif
}
void NativeTu15Datagram::send(std::span<const std::byte> bytes,NativeIpv4Endpoint destination) {
    if(!state_)throw std::runtime_error("Native network socket was moved");
    std::lock_guard lock(state_->mutex);const auto fd=state_->descriptor(identity_);
#if defined(__linux__) || defined(__APPLE__)
    const auto at=address(destination);
    const auto size=::sendto(fd,bytes.data(),bytes.size(),0,reinterpret_cast<const sockaddr*>(&at),sizeof(at));
    if(size<0)throw std::system_error(errno,std::generic_category(),"Native network datagram send");
    if(std::size_t(size)!=bytes.size())throw std::runtime_error("Native network datagram send was incomplete");
#else
    throw std::runtime_error("Native TU1.5 datagram platform unavailable");
#endif
}
std::optional<std::size_t> NativeTu15Datagram::receive(std::span<std::byte> bytes,NativeIpv4Endpoint& source) {
    if(!state_)throw std::runtime_error("Native network socket was moved");
    std::lock_guard lock(state_->mutex);const auto fd=state_->descriptor(identity_);
#if defined(__linux__) || defined(__APPLE__)
    sockaddr_in at{};std::byte scratch{};
    // A zero-capacity request must still consume a real packet and obtain its
    // endpoint. Some kernels short-circuit an empty iovec before doing that.
    // One private byte distinguishes an empty packet from truncated data;
    // the caller's logical capacity below remains exactly bytes.size().
    iovec buffer{bytes.empty()?&scratch:bytes.data(),bytes.empty()?1:bytes.size()};msghdr message{};
    message.msg_name=&at;message.msg_namelen=sizeof(at);message.msg_iov=&buffer;message.msg_iovlen=1;
    // MSG_TRUNC in recvmsg's output flags reports discarded data on both
    // Linux and Darwin, including when the supplied buffer has zero bytes.
    const auto size=::recvmsg(fd,&message,0);
    if(size<0) {
        if(errno==EAGAIN || errno==EWOULDBLOCK)return std::nullopt;
        throw std::system_error(errno,std::generic_category(),"Native network datagram receive");
    }
    if((message.msg_flags&MSG_TRUNC) || std::size_t(size)>bytes.size())
        throw std::length_error("Native network receive buffer truncated a datagram");
    source=endpoint(at);return std::size_t(size);
#else
    throw std::runtime_error("Native TU1.5 datagram platform unavailable");
#endif
}
std::array<unsigned,2> NativeTu15Datagram::observed_buffer_bytes()const {
    if(!state_)throw std::runtime_error("Native network socket was moved");
    std::lock_guard lock(state_->mutex);const auto fd=state_->descriptor(identity_);
#if defined(__linux__) || defined(__APPLE__)
    int receive=0,send=0;socklen_t bytes=sizeof(int);
    if(::getsockopt(fd,SOL_SOCKET,SO_RCVBUF,&receive,&bytes) || ::getsockopt(fd,SOL_SOCKET,SO_SNDBUF,&send,&bytes))
        throw std::system_error(errno,std::generic_category(),"Native network observed socket buffers");
    return {unsigned(receive),unsigned(send)};
#else
    throw std::runtime_error("Native TU1.5 datagram platform unavailable");
#endif
}
}

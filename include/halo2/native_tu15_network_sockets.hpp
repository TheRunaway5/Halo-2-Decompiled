#pragma once
#include "halo2/native_tu15_network_startup.hpp"
#include <map>
#include <thread>
#include <vector>
namespace halo2 {
struct NativeTu15SocketWrite {std::uint32_t address,value;};
struct NativeTu15SocketCall {
    std::uint32_t instruction,target,entry_esp,return_esp,result;
    std::vector<std::uint32_t> arguments;
    std::vector<NativeTu15SocketWrite> outputs;
    std::optional<NativeIpv4Endpoint> native_bound_endpoint;
};
// Native socket APIs used by the cold session constructor. Both source UDP
// channel categories use the PC port's actual IPv4 datagram provider. Xbox
// VDP encryption/framing, stream APIs and subsequent packet APIs are separate
// owners and are not claimed by this resource constructor.
class NativeTu15NetworkSockets final {
public:
    // 2000 is the existing PC session transport base. The source constructor
    // keeps logical1000..1006; native socket APIs own endpoint translation.
    NativeTu15NetworkSockets(SourceRuntimeMemory&,NativeTu15NetworkStartup&,std::uint16_t native_base_port=2000);
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    const std::vector<NativeTu15SocketCall>& calls()const noexcept{return calls_;}
    std::size_t live_sockets()const noexcept{return sockets_.size();}
    std::vector<NativeIpv4Endpoint> local_endpoints()const;
private:
    SourceRuntimeMemory& memory_;
    NativeTu15NetworkStartup& network_;
    std::thread::id thread_;
    std::map<std::uint32_t,NativeTu15Datagram> sockets_;
    std::uint64_t next_alias_=0xf5000001u;
    std::uint16_t native_base_port_;
    std::vector<NativeTu15SocketCall> calls_;
};
}

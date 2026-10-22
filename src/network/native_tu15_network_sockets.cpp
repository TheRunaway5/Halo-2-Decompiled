#include "halo2/native_tu15_network_sockets.hpp"
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p) {
    auto b=m.view(p,4);std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
void put(SourceRuntimeMemory& m,std::uint32_t p,std::uint32_t v) {
    std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);
}
}
NativeTu15NetworkSockets::NativeTu15NetworkSockets(SourceRuntimeMemory& m,NativeTu15NetworkStartup& network,std::uint16_t base)
    :memory_(m),network_(network),thread_(std::this_thread::get_id()),native_base_port_(base) {
    if(base<1024 || base>65529)throw std::invalid_argument("Native session transport base cannot own its complete unprivileged port range");
}
bool NativeTu15NetworkSockets::try_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    std::uint32_t site;unsigned count;
    switch(call.target) {
    case 0x3d7268:site=0xb6ddf;count=3;break;
    case 0x3d72b2:site=0xb68bd;count=3;break;
    case 0x3d72a3:site=0xb67ef;count=5;break;
    case 0x3d7289:site=0x91a05;count=3;break;
    case 0x3d7294:site=0xb6856;count=5;break;
    default:return false;
    }
    const auto sp=abi.gp[4];
    if(&bank!=&memory_ || std::this_thread::get_id()!=thread_ || call.tail || call.instruction!=site ||
        word(memory_,sp)!=site+5 || !network_.service())
        throw std::invalid_argument("Native cold socket API bank, executor, initialized context or return frame differs");
    NativeTu15SocketCall event{site,call.target,sp,sp+4+count*4,0,{},{},{}};
    for(unsigned i=0;i<count;++i)event.arguments.push_back(word(memory_,sp+4+4*i));
    const auto& a=event.arguments;
    if(call.target==0x3d7268) {
        if(a[0]!=2 || a[1]!=2 || (a[2]!=17 && a[2]!=254) || next_alias_>=0xffffffffull)
            throw std::invalid_argument("Native cold session socket domain or alias budget differs");
        auto socket=network_.service()->create_datagram();const auto alias=std::uint32_t(next_alias_);
        sockets_.emplace(alias,std::move(socket));++next_alias_;event.result=alias;
    } else {
        const auto found=sockets_.find(a[0]);if(found==sockets_.end())throw std::invalid_argument("Native cold socket API has no retained socket");
        auto& socket=found->second;
        if(call.target==0x3d72b2) {
            const auto bytes=memory_.view(a[1],16);
            if(a[2]!=16 || bytes[0]!=std::byte{2} || bytes[1]!=std::byte{})
                throw std::invalid_argument("Native cold socket bind requires its source IPv4 sockaddr");
            NativeIpv4Endpoint local;local.port=(std::to_integer<unsigned>(bytes[2])<<8)|std::to_integer<unsigned>(bytes[3]);
            for(unsigned i=0;i<4;++i)local.address[i]=std::to_integer<std::uint8_t>(bytes[4+i]);
            if(local.port!=1000 && local.port!=1001 && local.port!=1005 && local.port!=1006)
                throw std::invalid_argument("Native cold session bind is outside its recovered logical port set");
            local.port=std::uint16_t(native_base_port_+local.port-1000);socket.bind(local);event.native_bound_endpoint=socket.local_endpoint();
        } else if(call.target==0x3d72a3) {
            if(a[1]!=0xffff || (a[2]!=0x1001 && a[2]!=0x1002) || a[3]!=sp+24 || a[4]!=sp+32 || word(memory_,a[4])!=4)
                throw std::invalid_argument("Native cold socket buffer observation requires its complete caller-owned outputs");
            memory_.mutable_view(a[3],4);memory_.mutable_view(a[4],4);
            const auto buffers=socket.observed_buffer_bytes();const auto value=buffers[a[2]==0x1002?0:1];
            put(memory_,a[3],value);put(memory_,a[4],4);event.outputs={{a[3],value},{a[4],4}};
        } else if(call.target==0x3d7289) {
            if(a[1]!=0x8004667e || word(memory_,a[2])>1)throw std::invalid_argument("Native cold socket ioctl requires FIONBIO and a boolean value");
            socket.set_nonblocking(word(memory_,a[2])!=0);
        } else {
            if(a[1]!=0xffff || a[2]!=0x20 || a[4]!=4 || word(memory_,a[3])>1)
                throw std::invalid_argument("Native cold socket option requires its source broadcast boolean");
            socket.set_broadcast(word(memory_,a[3])!=0);
        }
    }
    calls_.push_back(std::move(event));abi.gp[0]=calls_.back().result;abi.gp[4]=calls_.back().return_esp;return true;
}
std::vector<NativeIpv4Endpoint> NativeTu15NetworkSockets::local_endpoints()const {
    std::vector<NativeIpv4Endpoint> result;for(const auto& [alias,socket]:sockets_)result.push_back(socket.local_endpoint());return result;
}
}

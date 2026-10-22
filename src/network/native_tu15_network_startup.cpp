#include "halo2/native_tu15_network_startup.hpp"
#include <limits>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t p) {
    const auto bytes=memory.view(p,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;
}
void put(SourceRuntimeMemory& memory,std::uint32_t site,std::uint32_t p,unsigned bytes,
    std::uint32_t value,const SourcePhysicsContactRecordObserver& observe) {
    const auto previous=memory.view(p,bytes);
    SourcePhysicsContactRecordStore store{site,p,{previous.begin(),previous.end()},std::vector<std::byte>(bytes)};
    for(unsigned i=0;i<bytes;++i)store.written[i]=std::byte(value>>(8*i));
    memory.write(p,store.written);if(observe)observe(store);
}
}
bool NativeTu15NetworkStartup::try_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi,
    const SourcePhysicsContactRecordObserver& observe) {
    unsigned arguments=0;std::uint32_t site=0;
    switch(call.target) {
    case 0x3d7656:site=0x6e097;arguments=1;break;
    case 0x3d766d:site=0x6e0ab;arguments=2;break;
    case 0x3d7768:site=0x6e127;break;
    case 0x3d775b:site=0x6e12c;break;
    default:return false;
    }
    if(&bank!=&memory_ || call.tail || call.instruction!=site || word(memory_,abi.gp[4])!=site+5)
        throw std::invalid_argument("Native network startup caller/bank differs");
    const auto sp=abi.gp[4];memory_.view(sp,4+arguments*4);
    if(call.target==0x3d7656) {
        if(xnet_references_==65535)throw std::overflow_error("Native XNet reference domain exhausted");
        if(!service_) {
            const auto pointer=word(memory_,sp+4);
            auto config=native_tu15_network_configuration();
            if(pointer) {
                const auto size=std::to_integer<unsigned>(memory_.view(pointer,1)[0]);
                config=native_tu15_network_configuration(memory_.view(pointer,(size==12 || size==76)?size:1));
            }
            auto created=std::make_unique<NativeTu15NetworkService>(config);service_=std::move(created);
        }
        ++xnet_references_;
    } else if(call.target==0x3d766d) {
        if(winsock_references_==65535)throw std::overflow_error("Native Winsock reference domain exhausted");
        const auto version=word(memory_,sp+4),output=word(memory_,sp+8);
        // Recovered game requests version2. Outputs follow original 3D75F1..
        // 3D7620, which advertises2.2, empty descriptions/vendor and no legacy
        // maximums. The desktop socket provider supplies the real resources.
        if(version!=2)throw std::invalid_argument("Native game Winsock request is outside its verified version domain");
        memory_.mutable_view(output,400);
        if(!service_)service_=std::make_unique<NativeTu15NetworkService>(native_tu15_network_configuration());
        put(memory_,site,output,2,version,observe);put(memory_,site,output+2,2,0x202,observe);
        put(memory_,site,output+0x18c,4,0,observe);put(memory_,site,output+0x186,2,0,observe);
        put(memory_,site,output+0x188,2,0,observe);put(memory_,site,output+4,1,0,observe);put(memory_,site,output+0x105,1,0,observe);
        ++winsock_references_;
    } else if(call.target==0x3d7768) {
        if(!winsock_references_)throw std::runtime_error("Native Winsock cleanup has no initialization owner");
        --winsock_references_;
    } else {
        if(!xnet_references_)throw std::runtime_error("Native XNet cleanup has no initialization owner");
        --xnet_references_;
    }
    if(!xnet_references_ && !winsock_references_)service_.reset();
    abi.gp[0]=0;abi.gp[4]=sp+4+arguments*4;return true;
}
}

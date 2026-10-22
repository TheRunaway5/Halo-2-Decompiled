#include "halo2/native_tu15_online_startup.hpp"
#include <limits>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t p) {
    const auto bytes=memory.view(p,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;
}
}
NativeTu15OnlineStartup::NativeTu15OnlineStartup(SourceRuntimeMemory& memory,std::shared_ptr<NativeTu15OnlineServices> services)
    :memory_(memory),services_(std::move(services)){}
bool NativeTu15OnlineStartup::try_call(SourcePhysicsContactMemory& memory,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x3aad19u)return false;
    if(&memory!=&memory_ || call.tail || call.instruction!=0x6e0bdu || word(memory_,abi.gp[4])!=0x6e0c2u)
        throw std::invalid_argument("Native online startup caller/bank differs");
    const auto parameters=word(memory_,abi.gp[4]+4);
    if(!parameters || word(memory_,parameters)!=0)
        throw std::invalid_argument("Native online startup is outside the verified zero-flag game request");
    if(!services_ || !services_->login || !services_->directory || !services_->account || !services_->matchmaking)
        throw std::runtime_error("Native online startup requires the actual desktop service owners");
    if(references_==std::numeric_limits<unsigned>::max())throw std::overflow_error("Native online reference domain exhausted");
    if(!context_)context_=std::make_unique<Context>(Context{services_});
    ++references_;abi.gp[0]=0;abi.gp[4]+=8;return true;
}
NativeRuntimeAddress NativeTu15OnlineStartup::native_address()const {
    if(!context_)throw std::runtime_error("Native online startup context is unavailable or retired");
    return NativeRuntimeAddress{reinterpret_cast<std::uintptr_t>(context_.get())};
}
void NativeTu15OnlineStartup::release() {
    if(!references_)throw std::runtime_error("Native online cleanup has no initialization owner");
    if(!--references_)context_.reset();
}
}

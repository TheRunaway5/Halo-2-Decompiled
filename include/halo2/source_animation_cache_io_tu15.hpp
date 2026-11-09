#pragma once
#include "halo2/cache.hpp"
#include "halo2/physics_world_runtime.hpp"
#include <thread>

namespace halo2 {
struct SourceAnimationCacheReadTu15 {
    std::uint32_t descriptor,destination,bytes,encoded_offset;
};

// The existing World animation read policy. Real cache/resource and retained
// request owners provide the descriptor, sector bytes and resulting job; this
// boundary only validates its caller and publishes completion in that order.
// Xbox asynchronous scheduling is outside this synchronous native policy.
template<class Inspect,class DescriptorBytes,class Read,class Retain>
bool source_try_animation_cache_io_tu15(SourcePhysicsContactMemory& source,
    SourcePhysicsContactMemory& backing,const SourcePhysicsContactRuntimeCall& call,
    SourcePhysicsWorldRuntimeAbi& abi,std::thread::id executor,Inspect&& inspect,
    DescriptorBytes&& descriptor_bytes,Read&& read,Retain&& retain) {
    if(call.instruction!=0x126139u||call.target!=0x1a3b70u||call.tail)return false;
    const auto word=[&](std::uint32_t address){
        std::array<std::byte,4> value;source.read(address,value);return Bytes(value).u32(0);
    };
    const auto sp=abi.gp[4],tracking=abi.gp[7];
    const auto descriptor=word(tracking+12u),size=word(sp+4u),destination=word(sp+8u);
    const auto resource=inspect(descriptor);
    if(std::this_thread::get_id()!=executor||word(sp)!=0x12613eu||
        word(sp+12u)!=0||word(sp+16u)!=tracking+4u||word(sp+20u)!=6u||
        !std::ranges::equal(descriptor_bytes(descriptor),std::span(resource.descriptor).first(12))||
        abi.gp[1]!=word(descriptor+8u)||size!=((resource.bytes.size()+511u)&~511u))
        throw FormatError("TU1.5 animation resource I/O differs from its actual cache descriptor");
    const SourceAnimationCacheReadTu15 request{descriptor,destination,size,abi.gp[1]};
    const auto payload=read(request);
    backing.write(destination,payload);
    const auto job=retain(request);
    constexpr std::array done{std::byte{1}};backing.write(tracking+4u,done);
    abi.gp[0]=job;abi.gp[4]+=28u;return true;
}
} // namespace halo2

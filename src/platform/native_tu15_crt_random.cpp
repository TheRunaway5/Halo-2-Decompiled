#include "halo2/native_tu15_crt_random.hpp"

namespace halo2 {
NativeTu15CrtRandomRequiredOwner::NativeTu15CrtRandomRequiredOwner(
    NativeTu15CrtRandomOwnerReason kind,std::uint32_t site,std::uint32_t owner,
    std::array<std::uint32_t,2> args,unsigned count)
    :std::runtime_error(kind==NativeTu15CrtRandomOwnerReason::elevated_level?
        "Native CRT thread context requires its actual elevated-level handler":
        "Native CRT thread context requires its actual allocator"),
     reason(kind),instruction(site),target(owner),arguments(args),argument_count(count){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4){
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
}
void write(SourcePhysicsContactMemory &memory,std::uint32_t at,std::uint32_t value){
    std::array<std::byte,4> bytes{};for(unsigned n=0;n<4u;++n)bytes[n]=std::byte(value>>(8u*n));memory.write(at,bytes);
}
}
std::uint32_t native_tu15_crt_thread_context(const SourcePhysicsContactMemory &memory,
    std::uint32_t fs){
    if(read(memory,fs+0x24u,1)>=2u){
        const auto owner=read(memory,0x41b758u);
        throw NativeTu15CrtRandomRequiredOwner(NativeTu15CrtRandomOwnerReason::elevated_level,
            0x330187u,owner,{10u,0u},1);
    }
    const auto thread=read(memory,fs+0x28u);std::uint32_t holder=0x573840u;
    if(read(memory,thread+0x28u)){
        const auto index=read(memory,0x48c920u),table=read(memory,fs+4u);
        holder=read(memory,table+index*4u)+0xcu;
    }
    const auto context=read(memory,holder);
    if(!context)throw NativeTu15CrtRandomRequiredOwner(NativeTu15CrtRandomOwnerReason::context_allocation,
        0x3301c4u,0x332383u,{1u,0x88u},2);
    return context;
}
std::uint32_t native_tu15_crt_random(SourcePhysicsContactMemory &memory,std::uint32_t fs){
    const auto context=native_tu15_crt_thread_context(memory,fs);
    const auto next=read(memory,context+0x14u)*std::uint32_t{0x343fdu}+std::uint32_t{0x269ec3u};
    write(memory,context+0x14u,next);
    return (next>>16u)&0x7fffu;
}
}

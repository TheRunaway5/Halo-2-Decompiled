#include "halo2/native_tu15_scoped_contact_query.hpp"
#include "halo2/source_collision_query_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include <bit>

namespace halo2 {
namespace {
std::uint32_t read_word(const SourcePhysicsContactMemory &memory, std::uint32_t at) {
    std::array<std::byte, 4> bytes;
    memory.read(at, bytes);
    std::uint32_t result=0;
    for(unsigned n=0;n<4;++n)result|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);
    return result;
}
// Compatibility callers retain flags across this integer stack adjustment.
std::uint32_t addition_flags(std::uint32_t retained, std::uint32_t left,
    std::uint32_t right) {
    const auto result=left+right;
    constexpr auto arithmetic=0x8d5u;
    return (retained&~arithmetic)|2u|
        (std::uint64_t(left)+right>0xffffffffu?1u:0u)|
        (std::popcount(result&255u)%2==0?4u:0u)|
        (((left^right^result)&16u)?16u:0u)|(result==0?64u:0u)|
        (result&0x80000000u?128u:0u)|
        ((~(left^right)&(left^result)&0x80000000u)?0x800u:0u);
}
}
void native_tu15_scoped_contact_query(SourcePhysicsContactMemory &memory,
    SourcePhysicsWorldRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    SourcePhysicsEntryObservation diagnostic(memory,0x2f5600u,[&]{return abi;});
    const auto entry_stack=abi.gp[4];
    const auto publish=[&](std::uint32_t site,std::uint32_t at,std::uint32_t value,unsigned size=4) {
        if(services.instruction)services.instruction(site);
        SourcePhysicsContactRecordStore store{site,at,{}, {}};
        store.before.resize(size);store.written.resize(size);
        memory.read(at,store.before);
        for(unsigned n=0;n<size;++n)store.written[n]=std::byte(value>>(8*n));
        memory.write(at,store.written);
        if(observe)observe(store);
    };
    // The collector is passed by address to the real query. Preserve its
    // source layout, scoped exception-chain link, and ordered scratch stores.
    const auto previous_exception=read_word(memory,abi.fs_base);
    publish(0x2f5600u,entry_stack-4u,0xffffffffu);
    publish(0x2f5602u,entry_stack-8u,0x37e728u);
    publish(0x2f560du,entry_stack-12u,previous_exception);
    publish(0x2f560eu,abi.fs_base,entry_stack-12u);
    const auto collector=entry_stack-24u;
    publish(0x2f561eu,collector+4u,0u,1);
    publish(0x2f5622u,collector,0x41ca40u);
    publish(0x2f5629u,collector+8u,read_word(memory,entry_stack+16u));
    const auto first_shape=read_word(memory,entry_stack+4u);
    publish(0x2f5634u,entry_stack-28u,collector);
    const auto second_shape=read_word(memory,entry_stack+8u);
    publish(0x2f5639u,entry_stack-4u,0u);
    const auto query_input=read_word(memory,entry_stack+12u);
    publish(0x2f5641u,entry_stack-32u,query_input);
    publish(0x2f5642u,entry_stack-36u,first_shape);
    publish(0x2f5643u,entry_stack-40u,second_shape);
    publish(0x2f5644u,entry_stack-44u,0x2f5649u);
    abi.gp[0]=query_input;abi.gp[1]=first_shape;abi.gp[2]=second_shape;
    abi.gp[4]=entry_stack-44u;
    abi.eflags=(abi.eflags&~0x8d5u)|0x46u; // Integer zero used for collector state.
    source_run_collision_query_tu15(memory,0x2f4de0u,abi,fp,services,observe);
    // Read the retained chain from the returned source frame, including the
    // nested owner's writes. The native kernel never replaces query results.
    abi.gp[1]=read_word(memory,abi.gp[4]+0x1cu);
    publish(0x2f564du,abi.fs_base,abi.gp[1]);
    abi.eflags=addition_flags(abi.eflags,abi.gp[4],0x28u);
    abi.gp[4]+=0x2cu;
    if(services.instruction)services.instruction(0x2f5657u);
}
}

#include "halo2/native_tu15_wrapped_contact_query.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& m_;
    const SourcePhysicsContactRecordObserver& observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o)
        :m_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p) const {
        std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t n{};
        for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);
        return n;
    }
    std::array<std::byte,16> vector(std::uint32_t p) const {
        if(p&15u)throw std::runtime_error("Native wrapped query requires aligned geometry");
        std::array<std::byte,16>b{};m_.read_vector(p,b);return b;
    }
    void store(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b) {
        SourcePhysicsContactRecordStore e{pc,p,{},{}};
        if(observer_){e.before.resize(b.size());m_.read(p,e.before);}
        m_.write(p,b);if(observer_){e.written.assign(b.begin(),b.end());observer_(e);}
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n) {
        std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));store(pc,p,b);
    }
    void byte(std::uint32_t pc,std::uint32_t p,std::uint8_t n){
        const std::array b{std::byte(n)};store(pc,p,b);
    }
    void vector(std::uint32_t pc,std::uint32_t p,const std::array<std::byte,16>&b) {
        if(p&15u)throw std::runtime_error("Native wrapped query requires aligned geometry");
        store(pc,p,b);
    }
};
}
void native_tu15_query_wrapped_contact_agent(SourcePhysicsContactMemory& memory,
    NativeTu15IslandCollisionAgent agent,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration configuration,
    NativeTu15ContactQueryCollector collector,NativeTu15WrappedContactProcessScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ConditionalContactServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15WrappedContactProcessStats*stats) {
    if(stats)++stats->calls;Memory m(memory,observe);
    const auto original_transform=m.word(first.identity+8u);
    const auto first_vector=m.vector(original_transform);
    const auto shape=m.word(first.identity);
    const auto current_transform=m.word(first.identity+8u);
    m.vector(0x2f6f5cu,scratch.geometry,first_vector);
    const auto second_vector=m.vector(original_transform+16u);
    m.vector(0x2f6f71u,scratch.geometry+16u,second_vector);
    NativeTu15GeometryMemory geometry(memory,scratch.geometry+32u,
        current_transform+32u,shape+16u,observe);
    native_tu15_compose_affine_geometry(geometry,fp);
    if(stats)++stats->geometry_calls;
    const auto child=m.word(agent.identity+12u);
    m.word(0x2f6f7eu,scratch.descriptor.identity+12u,first.identity);
    m.word(0x2f6f86u,scratch.descriptor.identity+8u,scratch.geometry);
    const auto inherited=m.word(first.identity+4u);
    const auto child_shape=m.word(shape+12u);
    m.word(0x2f6f90u,scratch.descriptor.identity+4u,inherited);
    m.word(0x2f6fa4u,scratch.descriptor.identity,child_shape);
    const auto table=m.word(child);
    const NativeTu15PhysicsVirtualOwner owner{m.word(table+4u)};
    if(!services.query)throw std::runtime_error("Native wrapped query requires actual child virtual+4");
    if(stats)++stats->child_calls;
    services.query({child},owner,scratch.descriptor,second,configuration,collector,fp);
    if(stats)++stats->child_returns;
}
void native_tu15_query_reversed_wrapped_contact_agent(SourcePhysicsContactMemory&memory,
    NativeTu15IslandCollisionAgent agent,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration configuration,
    NativeTu15ContactQueryCollector collector,NativeTu15ReversedWrappedContactQueryScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ConditionalContactServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15WrappedContactProcessStats*stats){
    Memory m(memory,observe);const auto local=scratch.collector.identity;
    m.byte(0x2f73deu,local+4u,0);
    m.word(0x2f73e2u,local,0x41ca40u);
    m.word(0x2f73e9u,local+8u,collector.identity);
    native_contact_detail::UnwindGuard cleanup([&]{m.word(0x2f0dd0u,local,0x461580u);});
    cleanup.arm(); // Source state0 at2F73F1,37E870 ->2F0DD0.
    native_tu15_query_wrapped_contact_agent(memory,agent,second,first,configuration,
        scratch.collector,scratch.wrapped,fp,services,observe,stats);
    cleanup.disarm();
}
} // namespace halo2

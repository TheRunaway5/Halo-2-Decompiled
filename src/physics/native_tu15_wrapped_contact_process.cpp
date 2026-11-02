#include "halo2/native_tu15_wrapped_contact_process.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;
    const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o)
        :memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p)const{
        std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t n{};
        for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);
        return n;
    }
    std::array<std::byte,16> vector(std::uint32_t p)const{
        if(p&15u)throw std::runtime_error("Native wrapped process requires aligned geometry");
        std::array<std::byte,16>b{};memory_.read_vector(p,b);return b;
    }
    void store(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b){
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(b.size());memory_.read(p,event.before);}
        memory_.write(p,b);
        if(observe_){event.written.assign(b.begin(),b.end());observe_(event);}
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n){
        std::array<std::byte,4>b{};
        for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));
        store(pc,p,b);
    }
    void vector(std::uint32_t pc,std::uint32_t p,const std::array<std::byte,16>&b){
        if(p&15u)throw std::runtime_error("Native wrapped process requires aligned geometry");
        store(pc,p,b);
    }
};
}
void native_tu15_process_wrapped_contact_agent(SourcePhysicsContactMemory&memory,
    NativeTu15IslandCollisionAgent agent,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration configuration,
    NativeTu15IslandContactOutput output,NativeTu15WrappedContactProcessScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15CompoundProcessServices&services,
    const SourcePhysicsContactRecordObserver&observe,
    NativeTu15WrappedContactProcessStats*stats){
    Memory m(memory,observe);if(stats)++stats->calls;
    const auto original_transform=m.word(first.identity+8u);
    const auto first_vector=m.vector(original_transform);
    const auto shape=m.word(first.identity);
    const auto current_transform=m.word(first.identity+8u);
    m.vector(0x2f6cbcu,scratch.geometry,first_vector);
    const auto second_vector=m.vector(original_transform+16u);
    m.vector(0x2f6cd1u,scratch.geometry+16u,second_vector);
    NativeTu15GeometryMemory geometry(memory,scratch.geometry+32u,
        current_transform+32u,shape+16u,observe);
    native_tu15_compose_affine_geometry(geometry,fp);
    if(stats)++stats->geometry_calls;
    const NativeTu15IslandCollisionAgent child{m.word(agent.identity+12u)};
    m.word(0x2f6cdeu,scratch.descriptor.identity+12u,first.identity);
    m.word(0x2f6ce6u,scratch.descriptor.identity+8u,scratch.geometry);
    const auto inherited_word=m.word(first.identity+4u);
    const auto child_shape=m.word(shape+12u);
    m.word(0x2f6cf0u,scratch.descriptor.identity+4u,inherited_word);
    m.word(0x2f6d04u,scratch.descriptor.identity,child_shape);
    const auto table=m.word(child.identity);
    const NativeTu15PhysicsVirtualOwner owner{m.word(table+16u)};
    if(!services.process_child)
        throw std::runtime_error("Native wrapped process requires its actual child vtable+10 owner");
    if(stats)++stats->child_calls;
    services.process_child({child,owner,scratch.descriptor,second,configuration,output},fp);
    if(stats)++stats->child_returns;
}
}

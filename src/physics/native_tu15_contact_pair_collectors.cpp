#include "halo2/native_tu15_contact_pair_collectors.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include "halo2/native_tu15_contact_agent_factory.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
void native_tu15_mark_contact_pair(SourcePhysicsContactMemory&memory,
    NativeTu15ContactQueryCollector collector,const SourcePhysicsContactRecordObserver&observe){
    const std::array written{std::byte{1}};
    SourcePhysicsContactRecordStore event{0x30ba80u,collector.identity+4u,{},{}};
    if(observe){event.before.resize(1);memory.read(event.address,event.before);event.written.assign(written.begin(),written.end());}
    memory.write(event.address,written);if(observe)observe(event);
}
void native_tu15_forward_reversed_contact_pair(SourcePhysicsContactMemory&memory,
    NativeTu15ContactQueryCollector collector,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ContactPairCollectorServices&services){
    const auto word=[&](std::uint32_t p){std::array<std::byte,4>b{};memory.read(p,b);
        std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;};
    const auto destination=word(collector.identity+8u),table=word(destination);
    const NativeTu15PhysicsVirtualOwner owner{word(table+4u)};
    if(!services.emit_pair)throw std::runtime_error("Native reversed pair collector requires its current callback owner");
    services.emit_pair({destination},owner,second,first,fp);
}
void native_tu15_collect_object_overlap_pair(SourcePhysicsContactMemory&memory,
    NativeTu15ContactQueryCollector collector,NativeTu15PhysicsBody second,
    const SourcePhysicsContactRecordObserver&observe){
    const auto word=[&](std::uint32_t p,unsigned bytes=4){std::array<std::byte,4>b{};
        memory.read(p,std::span(b).first(bytes));std::uint32_t n{};
        for(unsigned k=0;k<bytes;++k)n|=std::to_integer<unsigned>(b[k])<<(8u*k);return n;};
    auto root=second.identity,parent=word(root+12u);
    while(parent){root=parent;parent=word(root+12u);}
    if(word(root+0x18u)!=1u)return;
    const auto body=word(root+0x20u);if(!body)return;
    const auto count=word(body+0x34u);std::uint32_t component{};
    if(std::bit_cast<std::int32_t>(count)>0){
        const auto properties=word(body+0x30u);
        for(std::uint32_t n=0;std::bit_cast<std::int32_t>(n)<std::bit_cast<std::int32_t>(count);++n)
            if(word(properties+n*8u)==0x2001u){component=word(properties+n*8u+4u);break;}
    }
    const auto shape=word(second.identity),table=word(shape),owner=word(table+0x14u);
    std::uint32_t kind;
    if(owner==0xa9520u)kind=native_tu15_contact_child_class_23();
    else if(owner==0xab4b0u)kind=native_tu15_contact_child_class_24();
    else if(const auto value=native_tu15_activation_type(owner))kind=*value;
    else throw std::runtime_error("Native object overlap collector requires qualified shape class owner="+std::to_string(owner));
    if(kind!=24u){
        const auto current_shape=word(second.identity),user_data=word(current_shape+8u);
        if(user_data){
            const auto signed_data=std::bit_cast<std::int32_t>(user_data);
            const auto bounded=signed_data<1?1u:signed_data>16?16u:user_data;
            if(bounded!=user_data&&word(user_data+0x1eu,1)!=0xffu)return;
        }
    }
    const auto excluded=word(collector.identity+12u);
    if(excluded!=0xffffffffu&&excluded==component)return;
    for(const auto [pc,offset]:{std::pair{0x1ce036u,4u},std::pair{0x1ce03au,8u}}){
        const std::array written{std::byte{1}};
        SourcePhysicsContactRecordStore event{pc,collector.identity+offset,{},{}};
        if(observe){event.before.resize(1);memory.read(event.address,event.before);event.written.assign(written.begin(),written.end());}
        memory.write(event.address,written);if(observe)observe(event);
    }
}
}

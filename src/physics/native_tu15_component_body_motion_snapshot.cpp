#include "halo2/native_tu15_component_body_motion_snapshot.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>bytes{};m.read(p,std::span(bytes).first(width));std::uint32_t result{};
    for(unsigned n=0;n<width;++n)result|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return result;
}
void write(SourcePhysicsContactMemory&m,std::uint32_t site,std::uint32_t p,std::uint32_t value,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ComponentBodyMotionStats*stats){
    std::array<std::byte,4>bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
    SourcePhysicsContactRecordStore event{site,p,{},{}};
    if(observe){event.before.resize(4);m.read(p,event.before);event.written.assign(bytes.begin(),bytes.end());}
    m.write(p,bytes);if(stats)++stats->stores;if(observe)observe(event);
}
std::array<std::uint32_t,3> vector_words(const SourcePhysicsContactMemory&m,std::uint32_t p){
    const auto x=word(m,p),y=word(m,p+4u),z=word(m,p+8u);return{x,y,z};
}
}
std::array<std::uint32_t,3> native_tu15_component_body_position(SourcePhysicsContactMemory&m,
    std::uint32_t component,std::uint32_t index,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ComponentBodyMotionServices&services,NativeTu15ComponentBodyMotionStats*stats){
    if(stats)++stats->position_queries;
    if(word(m,component+4u)&0x40u){
        if(!services.component_frame){
            if(stats)++stats->unbound_services;
            throw std::runtime_error("Native TU1.5 body snapshot requires component frame owner1D8B90");
        }
        const auto frame=services.component_frame(component,index,fp);if(stats)++stats->component_frames;
        return{frame[10],frame[11],frame[12]};
    }
    const auto rows=word(m,component+0x70u),body=word(m,rows+index*96u+0x40u),motion=word(m,body+0x3cu);
    const auto z=word(m,motion+0xb8u),y=word(m,motion+0xb4u),x=word(m,motion+0xb0u);
    return{x,y,z};
}
void native_tu15_publish_component_body_motion(SourcePhysicsContactMemory&m,
    std::uint32_t component,std::uint32_t index,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ComponentBodyMotionServices&services,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ComponentBodyMotionStats*stats){
    if(stats)++stats->calls;
    const auto initial_rows=word(m,component+0x70u),destination=initial_rows+index*96u;
    const auto position=native_tu15_component_body_position(m,component,index,fp,services,stats);
    const auto rows=word(m,component+0x70u),body=word(m,rows+index*96u+0x40u);
    const auto disabled=word(m,body+0x40u,1),zero_vector=word(m,0x474d04u);
    std::array<std::uint32_t,3>linear,angular;
    if(!disabled){
        if(stats)++stats->body_motion_queries;
        linear=vector_words(m,word(m,body+0x3cu)+0x40u);
    }else{linear=vector_words(m,zero_vector);if(stats)++stats->disabled_bodies;}
    if(!disabled)angular=vector_words(m,word(m,body+0x3cu)+0x50u);
    else angular=vector_words(m,zero_vector);
    constexpr std::array<std::uint32_t,9>sites{0x1d731bu,0x1d7321u,0x1d7328u,
        0x1d732eu,0x1d7334u,0x1d733eu,0x1d7342u,0x1d7345u,0x1d7349u};
    for(unsigned n=0;n<3;++n)write(m,sites[n],destination+n*4u,position[n],observe,stats);
    for(unsigned n=0;n<3;++n)write(m,sites[n+3],destination+12u+n*4u,linear[n],observe,stats);
    for(unsigned n=0;n<3;++n)write(m,sites[n+6],destination+24u+n*4u,angular[n],observe,stats);
}
}

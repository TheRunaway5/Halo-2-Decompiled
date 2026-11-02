#include "halo2/native_tu15_collision_component.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned size=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(size));std::uint32_t n{};for(unsigned k=0;k<size;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n,unsigned size=4){SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observe_){event.before.resize(size);memory_.read(p,event.before);}std::array<std::byte,4>b{};for(unsigned k=0;k<size;++k)b[k]=std::byte(n>>(k*8u));memory_.write(p,std::span(b).first(size));if(observe_){event.written.assign(b.begin(),b.begin()+size);observe_(event);}}
};
std::int32_t signed_short(std::uint32_t n){return std::bit_cast<std::int16_t>(std::uint16_t(n));}
}
bool native_tu15_collision_channel_enabled(SourcePhysicsContactMemory&memory,
    std::uint32_t channel,std::uint32_t instance){
    if(channel==0xffffffffu)return true;
    const SourcePhysicsContactRecordObserver none;Memory m(memory,none);
    const auto bsp=std::uint32_t(signed_short(m.word(0x474c24u,2))),visibility=m.word(0x4f9b50u);
    const auto first=instance==0xffffffffu?visibility+(bsp<<5u)+1u:visibility+(bsp<<10u)+instance+0x201u;
    const auto bit=1u<<(channel&31u),word=std::uint32_t(std::bit_cast<std::int32_t>(channel)>>5);
    return(m.word(first+word*4u)&bit)!=0;
}
bool native_tu15_collision_component_enabled(SourcePhysicsContactMemory&memory,
    std::uint32_t key){
    const SourcePhysicsContactRecordObserver none;Memory m(memory,none);
    const auto kind=key>>29u,index=key&0xffffu,part=(key>>16u)&0x1fffu;
    switch(kind){
    case 1:{
        const auto model=m.word(0x4ecc10u),surface=m.word(model+0x2cu)+index*8u;
        if(!(m.word(surface+4u,1)&8u))return true;
        const auto channel=m.word(surface+5u,1);
        // The source's byte-zero-extension makes its subsequent -1 test
        // unreachable. Preserve the real bitmap read for channel255.
        const auto bsp=std::uint32_t(signed_short(m.word(0x474c24u,2)));
        const auto word=(channel>>5u)+bsp*8u,bit=1u<<(channel&31u),visibility=m.word(0x4f9b50u);
        return(m.word(visibility+word*4u+1u)&bit)!=0;
    }
    case 2:return true;
    case 5:{
        const auto runtime=m.word(0x4ecc18u),instances=m.word(runtime+0x144u);
        const auto geometry_index=std::uint32_t(signed_short(m.word(instances+index*0x58u+0x34u,2)));
        const auto geometry=m.word(runtime+0x13cu),surfaces=m.word(geometry+geometry_index*0xc8u+0x9cu);
        const auto surface=surfaces+part*8u;
        if(!(m.word(surface+4u,1)&8u))return true;
        const auto channel=m.word(surface+5u,1);return native_tu15_collision_channel_enabled(memory,channel,index);
    }
    case 3:case 4:{
        const auto objects=m.word(0x52be70u),datum=m.word(objects+index*4u);
        if(datum==0xffffffffu)return false;
        const auto entries=m.word(m.word(0x4ecbd0u)+0x44u),header=entries+(datum&0xffffu)*12u;
        if(m.word(header+2u,1)&0x10u)return false;
        const auto object=m.word(header+8u),offset=std::uint32_t(signed_short(m.word(object+0x11au,2)));
        const auto generation=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(m.word(object+offset+(part&31u),1)))));
        return generation==((part>>5u)&255u);
    }
    default:throw std::runtime_error("Native TU1.5 collision key requires its unsupported encoded-kind owner");
    }
}
NativeTu15ContactComponent native_tu15_resolve_surface_component(
    SourcePhysicsContactMemory&memory,NativeTu15CollisionResolveArguments arguments,
    NativeTu15CollisionResolveScratch scratch,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15CollisionSurfaceStats*stats){
    Memory m(memory,observe);const auto key=m.word(arguments.identity),kind=key>>29u;
    if(kind!=1u&&kind!=5u)throw std::runtime_error("Native TU1.5 component resolver requires its shared/object-key native owner");
    if(!native_tu15_collision_component_enabled(memory,key)){
        const NativeTu15ContactComponent output{m.word(arguments.identity+4u)};
        if(!output.identity)return output;
        m.word(0x1f7d5du,output.identity+6u,1,2);m.word(0x1f7d63u,output.identity+8u,0);m.word(0x1f7d6au,output.identity,0x462180u);return output;
    }
    const auto index=key&0xffffu,part=(key>>16u)&0x1fffu;
    if(kind==1u){
        const NativeTu15ContactComponent output{m.word(arguments.identity+4u)};
        if(!output.identity)return output;
        const auto data=scratch.unplaced_arguments.identity;
        m.word(0x1f7c4eu,data+0x10u,key);m.word(0x1f7c4fu,data+0xcu,index);
        const auto model=m.word(0x4ecc10u);
        m.word(0x1f7c55u,data+8u,0xffffffffu);m.word(0x1f7c57u,data+4u,1);m.word(0x1f7c58u,data,model);
        return native_tu15_build_collision_surface_shape(memory,output,{data},{0},scratch.unplaced_vertex,fp,observe,stats);
    }
    const auto runtime=m.word(0x4ecc18u),instances=m.word(runtime+0x144u),geometry=m.word(runtime+0x13cu);
    const auto instance=instances+index*0x58u,geometry_index=std::uint32_t(signed_short(m.word(instance+0x34u,2)));
    const auto collision=m.word(geometry+geometry_index*0xc8u+0xb4u);
    m.word(0x1f7d32u,scratch.placed_resource,part);
    const NativeTu15ContactComponent output{m.word(arguments.identity+4u)};
    if(!output.identity)return output;
    const auto data=scratch.placed_arguments.identity;
    m.word(0x1f67c4u,data+0x10u,key);const auto resource=m.word(scratch.placed_resource);
    m.word(0x1f67c9u,data+0xcu,resource);const auto resource_identity=m.word(collision+0x30u),model=m.word(collision+0xcu);
    m.word(0x1f67d0u,data+8u,resource_identity);m.word(0x1f67d1u,data+4u,0);m.word(0x1f67d3u,data,model);
    return native_tu15_build_collision_surface_shape(memory,output,{data},{instance},scratch.placed_vertex,fp,observe,stats);
}
} // namespace halo2

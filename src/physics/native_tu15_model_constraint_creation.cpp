#include "halo2/native_tu15_model_constraint_creation.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4){
    std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(n*8u));
    SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(width);m.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
    m.write(p,std::span(b).first(width));if(o)o(event);
}
}
std::uint32_t native_tu15_create_model_constraint(SourcePhysicsContactMemory&m,
    NativeTu15ModelConstraintRequest request,std::uint32_t flags,NativeTu15ConstraintLocalPointScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ModelConstraintCreationServices&services,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15ModelConstraintCreationStats*stats){
    if(stats)++stats->calls;
    if(depth>3||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
       (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)){
        if(stats)++stats->unsupported_numerical_profiles;
        throw std::runtime_error("Native TU1.5 model constraint creation requires its unsupported numerical-fault owner");
    }
    const auto pool=read(m,0x52d68cu),datum=native_tu15_data_array_allocate(m,pool);
    if(stats)++stats->allocations;const auto instances=read(m,pool+0x44u),object_pool=read(m,0x4ecbd0u);
    const auto objects=read(m,object_pool+0x44u),component_pool=read(m,0x52be5cu),components=read(m,component_pool+0x44u);
    const auto first=read(m,request.identity+4u);
    const auto object_datum=read(m,components+(first&0xffffu)*160u+8u);
    const auto object=read(m,objects+(object_datum&0xffffu)*12u+8u),definition_datum=read(m,object);
    const auto instance=instances+(datum&0xffffu)*160u,definitions=read(m,0x4f0414u);
    const auto definition=read(m,definitions+(definition_datum&0xffffu)*16u+8u);
    const auto second=read(m,request.identity+0x10u);
    if(second!=0xffffffffu){
        const auto next_object_datum=read(m,components+(second&0xffffu)*160u+8u);
        const auto next_object=read(m,objects+(next_object_datum&0xffffu)*12u+8u),next_definition_datum=read(m,next_object);
        const auto next_definition=read(m,definitions+(next_definition_datum&0xffffu)*16u+8u);
        const auto first_limit=read(m,definition+0x1au,1),second_limit=read(m,next_definition+0x1au,1);
        store(m,observer,0x22cf47u,instance+0xbu,first_limit>second_limit?second_limit:first_limit,1);
        if(stats)++stats->second_definitions;
    }else store(m,observer,0x22cf4fu,instance+0xbu,read(m,definition+0x1au,1),1);
    store(m,observer,0x22cf5cu,instance+2u,0,1);
    const auto distance=native_tu15_model_constraint_reference_distance(m,request.identity+0x1cu,flags,fp,
        stats?&stats->distance:nullptr);
    store(m,observer,0x22cf69u,instance+0xcu,flags,1);
    store(m,observer,0x22cf6fu,instance+4u,distance);
    store(m,observer,0x22cf74u,instance+8u,0,2);store(m,observer,0x22cf7au,instance+0xau,0,1);
    store(m,observer,0x22cf81u,instance+0x24u,read(m,request.identity+4u));
    store(m,observer,0x22cf87u,instance+0x28u,read(m,request.identity+0x10u));
    store(m,observer,0x22cf8du,instance+0xdu,read(m,request.identity+0x34u,1),1);
    store(m,observer,0x22cf93u,instance+0xeu,read(m,request.identity+0x38u,1),1);
    store(m,observer,0x22cf96u,instance+0xfu,0,1);
    store(m,observer,0x22cf9eu,instance+0x2cu,read(m,request.identity+0xcu,2),2);
    store(m,observer,0x22cfa6u,instance+0x2eu,read(m,request.identity+0x18u,2),2);
    store(m,observer,0x22cfafu,instance+0x1cu,0x7fu,1);
    store(m,observer,0x22cfb2u,instance+0x1eu,0x7fu,1);
    store(m,observer,0x22cfb5u,instance+0x20u,0x7fu,1);
    store(m,observer,0x22cfb8u,instance+0x30u,0xffffffffu);
    store(m,observer,0x22cfbbu,instance+0x34u,0xffffffffu);
    store(m,observer,0x22cfbeu,instance+0x38u,0xffffffffu);
    store(m,observer,0x22cfc1u,instance+0x3cu,0xffffffffu);
    store(m,observer,0x22cfc4u,instance+0x1du,0xffu,1);
    store(m,observer,0x22cfc7u,instance+0x10u,0,1);
    store(m,observer,0x22cfcbu,instance+0x11u,0,1);
    store(m,observer,0x22cfCFu,instance+0x14u,0);
    store(m,observer,0x22cfd4u,instance+0x18u,0);
    store(m,observer,0x22cfd9u,instance+0x1fu,0xffu,1);
    store(m,observer,0x22cfe4u,instance+0x5cu,read(m,request.identity+0x28u));
    store(m,observer,0x22cfe9u,instance+0x60u,read(m,request.identity+0x2cu));
    store(m,observer,0x22cfefu,instance+0x64u,read(m,request.identity+0x30u));
    const auto fallback=read(m,0x474d10u);
    native_tu15_normalize_vector(m,instance+0x5cu,fallback,fp,depth,observer,stats?&stats->normalize:nullptr);
    store(m,observer,0x22d002u,instance+0x68u,read(m,request.identity+0x1cu));
    store(m,observer,0x22d008u,instance+0x6cu,read(m,request.identity+0x20u));
    store(m,observer,0x22d00eu,instance+0x70u,read(m,request.identity+0x24u));
    store(m,observer,0x22d011u,instance+0x48u,0);store(m,observer,0x22d016u,instance+0x4cu,0);
    store(m,observer,0x22d01bu,instance+3u,0xffu,1);
    store(m,observer,0x22d01eu,instance+0x8cu,0);
    store(m,observer,0x22d026u,instance+0x90u,0,1);store(m,observer,0x22d02du,instance+0x91u,0,1);
    store(m,observer,0x22d037u,instance+0x9cu,read(m,request.identity+0x3au));
    store(m,observer,0x22d03du,instance+0x94u,0xffffffffu);store(m,observer,0x22d043u,instance+0x98u,0);
    const auto second_motion=read(m,request.identity+0x10u),first_motion=read(m,request.identity+4u);
    const bool faster=native_tu15_second_constraint_body_moves_faster(m,first_motion,second_motion,fp,stats?&stats->motion:nullptr);
    const auto previous=read(m,instance+2u,1);
    store(m,observer,0x22d063u,instance+2u,faster?(previous|2u):(previous&0xfdu),1);
    if(read(m,request.identity+0x34u)==0xffffffffu){
        store(m,observer,0x22d06bu,instance+8u,read(m,instance+8u,2)+1u,2);
        const auto second_slot=read(m,request.identity+0x14u),first_slot=read(m,request.identity+8u);
        native_tu15_update_constraint_local_points(m,instance,{first_slot,second_slot},scratch,fp,observer,stats?&stats->points:nullptr);
        if(stats)++stats->local_point_updates;
        native_tu15_release_constraint_reference(m,instance,datum,fp,depth,services.reference,observer,stats?&stats->reference:nullptr);
        if(stats)++stats->reference_releases;
    }
    if(stats)++stats->created;return datum;
}
} // namespace halo2

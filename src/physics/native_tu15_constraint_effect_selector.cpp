#include "halo2/native_tu15_constraint_effect_selector.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
class Environment {
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    explicit Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),previous_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 effect selector requires its SIMD fault owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t v{};for(unsigned n=0;n<width;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,std::uint32_t pc,std::uint32_t p,std::uint32_t v,unsigned width=4){
    std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(v>>(8u*n));SourcePhysicsContactRecordStore e{pc,p,{},{}};
    if(o){e.before.resize(width);m.read(p,e.before);e.written.assign(b.begin(),b.begin()+width);}m.write(p,std::span(b).first(width));if(o)o(e);
}
std::int32_t signed16(std::uint32_t v){return std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v)));}
std::int32_t signed8(std::uint32_t v){return std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v)));}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
__m128 scalar(std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(v)));}
std::uint32_t bits(__m128 v){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool greater(__m128 a,__m128 b){unsigned char v;asm volatile("comiss %2,%1; seta %0":"=qm"(v):"x"(a),"x"(b):"cc","memory");return v;}
std::uint32_t object_data(const SourcePhysicsContactMemory&m,std::uint32_t component,std::uint32_t components,std::uint32_t objects){
    const auto datum=read(m,components+(component&0xffffu)*160u+8u);return read(m,objects+(datum&0xffffu)*12u+8u);
}
std::uint32_t component_definition(const SourcePhysicsContactMemory&m,std::uint32_t object,std::uint32_t tags){return read(m,tags+(read(m,object)&0xffffu)*16u+8u);}
std::uint32_t effect_for_definition(const SourcePhysicsContactMemory&m,std::uint32_t definition,std::uint32_t type,
    std::uint16_t material,bool secondary,NativeTu15ConstraintEffectSelectorStats*stats){
    const auto event=native_tu15_find_constraint_effect_event(m,definition,type,secondary,stats);if(event==0xffffffffu)return 0;
    const auto tags=read(m,0x4f0414u),data=read(m,tags+(definition&0xffffu)*16u+8u),bank=read(m,data+4u);
    return native_tu15_find_constraint_effect_material(m,bank+event*24u,material,secondary,stats);
}
std::uint32_t effect_for_material(const SourcePhysicsContactMemory&m,std::uint16_t material,std::uint32_t type,
    std::uint16_t other,bool secondary,NativeTu15ConstraintEffectSelectorStats*stats){
    std::uint32_t found{};
    while(material!=0xffffu&&signed16(material)>=0){
        if(stats)++stats->material_queries;const auto scenario=read(m,0x4ecc1cu),count=read(m,scenario+0x150u);
        if(!less(std::uint32_t(signed16(material)),count))return found;
        const auto bank=read(m,scenario+0x154u),row=bank+std::uint32_t(signed16(material))*0xb4u;if(!row)return found;
        const auto definition=read(m,row+0xb0u);if(definition!=0xffffffffu)found=effect_for_definition(m,definition,type,other,secondary,stats);
        material=std::uint16_t(read(m,row+8u,2));if(found)return found;
    }
    return found;
}
}
std::uint32_t native_tu15_find_constraint_effect_event(const SourcePhysicsContactMemory&m,std::uint32_t definition,
    std::uint32_t type,bool secondary,NativeTu15ConstraintEffectSelectorStats*stats){
    if(stats)++stats->event_queries;if(type==0xffffffffu)return type;
    const auto tags=read(m,0x4f0414u),data=read(m,tags+(definition&0xffffu)*16u+8u),count=read(m,data);
    while(true){
        if(less(type,count)){const auto bank=read(m,data+4u),row=bank+type*24u;
            if(less(0u,read(m,row+(secondary?0x10u:8u))))return type;}
        if(type-13u>7u)return 0xffffffffu;
        const auto target=read(m,0x18aefcu+(type-13u)*4u);if(stats)++stats->event_fallbacks;
        switch(target){case 0x18aed0u:type=15;break;case 0x18aed7u:type=16;break;case 0x18aedeu:type=19;break;case 0x18aee5u:type=14;break;
            case 0x18aeeau:return 0xffffffffu;default:if(stats)++stats->unknown_constant_targets;throw std::runtime_error("Native TU1.5 effect event has an unknown authored classification");}
    }
}
std::uint32_t native_tu15_find_constraint_effect_material(const SourcePhysicsContactMemory&m,std::uint32_t event,
    std::uint16_t material,bool secondary,NativeTu15ConstraintEffectSelectorStats*stats){
    const auto count=read(m,event+(secondary?0x10u:8u));std::uint32_t found{};
    while(material!=0xffffu&&signed16(material)>=0){
        if(stats)++stats->material_rows;const auto scenario=read(m,0x4ecc1cu),extent=read(m,scenario+0x150u);
        if(!less(std::uint32_t(signed16(material)),extent))return found;
        const auto bank=read(m,scenario+0x154u),row=bank+std::uint32_t(signed16(material))*0xb4u;if(!row)return found;
        if(less(0u,count)){std::uint32_t index{},offset{};do{
            const auto candidates=read(m,event+(secondary?0x14u:0xcu)),candidate=candidates+offset;
            if(stats)++stats->material_candidates;if(read(m,candidate+0x14u,2)==material){found=candidate;break;}
            ++index;offset+=24u;if(!less(index,count))break;
        }while(true);}
        material=std::uint16_t(read(m,row+8u,2));if(found){if(stats)++stats->selected_rows;return found;}
    }
    return found;
}
std::uint32_t native_tu15_constraint_effect_radius(const SourcePhysicsContactMemory&m,std::uint32_t type,
    NativeTu15ConstraintEffectSelectorStats*stats){
    const auto ordinary=read(m,0x46a110u);if(type>20u)return ordinary;
    const auto classification=read(m,0x18b1b4u+type,1),target=read(m,0x18b1a0u+classification*4u);
    switch(target){case 0x18b19eu:return ordinary;case 0x18b17bu:return read(m,0x46a1a8u);case 0x18b184u:return read(m,0x46a108u);
        case 0x18b18du:return read(m,0x46a1a4u);case 0x18b196u:return read(m,0x46a118u);
        default:if(stats)++stats->unknown_constant_targets;throw std::runtime_error("Native TU1.5 effect radius has an unknown authored classification");}
}
bool native_tu15_constraint_effect_near_camera(const SourcePhysicsContactMemory&m,std::uint32_t point,
    std::uint32_t radius,bool scaled,NativeTu15PhysicsFrameFp&fp,NativeTu15ConstraintEffectSelectorStats*stats){
    if(stats)++stats->proximity_queries;const auto list=read(m,0x4f54f0u);Environment environment(fp);
    const auto square=mul(scalar(radius),scalar(radius));std::uint32_t selected=0xffffffffu;
    for(unsigned n=0;n<4;++n)if(read(m,list+0xcu+n*4u)!=0xffffffffu){selected=n;break;}
    if(selected==0xffffffffu)return false;const auto bsp=read(m,0x474c24u,2);
    while(true){
        if(stats)++stats->camera_tests;const auto camera=bsp==0xffffu?0u:0x4f655cu+selected*0x358u;
        auto z=scalar(read(m,camera+8u));z=sub(z,scalar(read(m,point+8u)));
        auto x=scalar(read(m,camera));x=sub(x,scalar(read(m,point)));
        auto y=scalar(read(m,camera+4u));y=sub(y,scalar(read(m,point+4u)));
        const auto zz=mul(z,z);x=mul(x,x);const auto yy=mul(y,y);auto distance=add(x,zz);distance=add(distance,yy);
        if(scaled){const auto factor=scalar(read(m,camera+0x50u));distance=mul(mul(factor,factor),distance);}
        if(greater(square,distance))return true;
        std::uint32_t next=0xffffffffu;for(unsigned n=selected+1u;n<4;++n)if(read(m,list+0xcu+n*4u)!=0xffffffffu){next=n;break;}
        if(next==0xffffffffu)return false;selected=next;
    }
}
void native_tu15_select_constraint_effects(SourcePhysicsContactMemory&m,NativeTu15ConstraintEffectSelection input,
    NativeTu15ConstraintEffectSelectorScratch scratch,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintEffectSelectorStats*stats){
    if(stats)++stats->calls;const auto second=read(m,input.instance+0x28u);std::uint32_t component{};bool chose_secondary=false;
    auto right=input.first_key,left=input.second_key;
    if(second!=0xffffffffu){
        const auto manager=read(m,0x52be5cu),components=read(m,manager+0x44u),objects_pool=read(m,0x4ecbd0u),objects=read(m,objects_pool+0x44u);
        const auto first=read(m,input.instance+0x24u),tags=read(m,0x4f0414u),object=object_data(m,first,components,objects),definition=component_definition(m,object,tags);
        if(read(m,definition+0x58u)==0xffffffffu){const auto other=object_data(m,second,components,objects),other_definition=component_definition(m,other,tags);
            if(read(m,other_definition+0x58u)!=0xffffffffu){component=second;chose_secondary=true;right=input.second_key;left=input.first_key;if(stats)++stats->secondary_components;}}
    }
    if(!chose_secondary)component=read(m,input.instance+0x24u);
    const auto source_kind=signed8(read(m,input.instance+0xbu,1)),kind=signed8(read(m,input.instance+0xdu,1));
    for(unsigned n=0;n<3;++n)store(m,{},0,scratch.position+n*4u,read(m,input.position+n*4u));
    store(m,{},0,scratch.key,0xffffu,2);
    if(second==0xffffffffu){Environment environment(fp);const auto reach=scalar(read(m,0x48b874u));
        for(unsigned n=0;n<3;++n){auto v=scalar(read(m,input.instance+0x5cu+n*4u));v=mul(v,reach);v=add(v,scalar(read(m,scratch.position+n*4u)));
            store(m,{},0,scratch.position+n*4u,bits(v));}}
    native_tu15_constraint_effect_location(m,scratch.position,scratch.location,fp,{},stats?&stats->location:nullptr);
    const auto manager=read(m,0x52be5cu);
    if(read(m,scratch.location+4u,2)==0xffffu){
        if(stats)++stats->location_fallbacks;const auto components=read(m,manager+0x44u),datum=read(m,components+(component&0xffffu)*160u+8u),objects=read(m,0x4ecbd0u),bank=read(m,objects+0x44u);
        const auto object=read(m,bank+(datum&0xffffu)*12u+8u),first=read(m,object+0x28u),last=read(m,object+0x2cu);
        store(m,{},0,scratch.location,first);store(m,{},0,scratch.location+4u,last);
    }
    if(native_tu15_constraint_effect_location_key(m,scratch.location,scratch.position,scratch.key,fp,{},stats?&stats->location:nullptr))left=std::uint16_t(read(m,scratch.key,2));
    auto definition=std::uint32_t(0xffffffffu);
    if(component!=0xffffffffu&&kind==-1){
        const auto components=read(m,manager+0x44u),objects_pool=read(m,0x4ecbd0u),objects=read(m,objects_pool+0x44u),tags=read(m,0x4f0414u);
        const auto object=object_data(m,component,components,objects),data=component_definition(m,object,tags);definition=read(m,data+0x58u);
    }
    const auto type=input.mode==0?std::uint32_t(source_kind+10):input.mode==2?14u:13u;
    std::array<std::uint32_t,6>values;values.fill(0xffffffffu);std::uint32_t material{};
    if(left!=0xffffu&&signed16(left)>=0){const auto scenario=read(m,0x4ecc1cu),extent=read(m,scenario+0x150u);
        if(less(std::uint32_t(signed16(left)),extent))material=read(m,scenario+0x154u)+std::uint32_t(signed16(left))*0xb4u;}
    const auto first_type=type==13u?14u:type;
    if(native_tu15_constraint_effect_near_camera(m,scratch.position,native_tu15_constraint_effect_radius(m,type,stats),false,fp,stats)){
        std::uint32_t first_row{},second_row{};
        if(definition!=0xffffffffu){first_row=effect_for_definition(m,definition,first_type,left,false,stats);second_row=effect_for_definition(m,definition,type,left,true,stats);}
        if(!first_row)first_row=effect_for_material(m,right,first_type,left,false,stats);
        if(!second_row)second_row=effect_for_material(m,right,type,left,true,stats);
        if(first_row){
            const auto override=read(m,first_row+0x16u,1);
            if(override==1u&&source_kind!=-1){
                if(type>=13u&&type<=14u)values[2]=read(m,material+0x54u);
                else values[2]=read(m,material+std::uint32_t(source_kind)*8u+0x3cu);}
            values[0]=read(m,first_row+4u);values[4]=read(m,first_row+0xcu);
        }
        if(second_row){
            if(read(m,second_row+0x16u,1)==1u&&source_kind!=-1){
                if(type==13u)values[3]=read(m,material+0x94u);else if(type==14u)values[3]=read(m,material+0x8cu);
                else values[3]=read(m,material+std::uint32_t(source_kind)*8u+0x74u);}
            values[1]=read(m,second_row+4u);values[5]=read(m,second_row+0xcu);
        }
    }else if(stats)++stats->distance_refusals;
    constexpr std::array<std::uint32_t,6>sites{0x18b384u,0x18b392u,0x18b3a0u,0x18b3aeu,0x18b3bcu,0x18b3cau};
    for(unsigned n=0;n<6;++n)if(input.outputs[n])store(m,observer,sites[n],input.outputs[n],values[n]);
}
} // namespace halo2

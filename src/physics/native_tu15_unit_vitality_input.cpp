#include "halo2/native_tu15_unit_vitality_input.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t value,NativeTu15UnitMaximumVitalityStats*stats){
    std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));
    SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(4);m.read(p,event.before);event.written.assign(b.begin(),b.end());}
    m.write(p,b);if(stats)++stats->stores;if(o)o(event);
}
class Simd {
    NativeTu15PhysicsFrameFp&fp_;unsigned host_;
public:
    explicit Simd(NativeTu15PhysicsFrameFp&fp):fp_(fp),host_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native maximum vitality requires its masked SIMD input");
        _mm_setcsr(fp.mxcsr);
    }
    ~Simd(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);}
};
bool positive(std::uint32_t bits){
    const auto value=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));
    const auto zero=_mm_setzero_ps();unsigned char result;
    asm volatile("comiss %2,%1;seta %0":"=qm"(result):"x"(value),"x"(zero):"cc","memory");return result;
}
std::uint32_t body(const SourcePhysicsContactMemory&m,std::uint32_t unit){
    const auto pool=read(m,0x4ecbd0u),headers=read(m,pool+0x44u);
    return read(m,headers+(unit&0xffffu)*12u+8u);
}
std::uint32_t damage_information(const SourcePhysicsContactMemory&m,std::uint32_t unit,
    NativeTu15UnitMaximumVitalityStats*stats){
    if(stats)++stats->definition_queries;
    if(unit==0xffffffffu)return 0;
    const auto unit_body=body(m,unit),definition=read(m,unit_body),tags=read(m,0x4f0414u);
    const auto object_tag=read(m,tags+(definition&0xffffu)*16u+8u),model=read(m,object_tag+0x38u);
    if(model==0xffffffffu){if(stats)++stats->null_models;return 0;}
    const auto model_tag=read(m,tags+(model&0xffffu)*16u+8u),count=read(m,model_tag+0x60u);
    if(std::bit_cast<std::int32_t>(count)<=0){if(stats)++stats->no_damage_information;return 0;}
    return read(m,model_tag+0x64u);
}
}
void native_tu15_set_unit_maximum_vitality(SourcePhysicsContactMemory&m,std::uint32_t unit,
    NativeTu15UnitMaximumVitalityInput input,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&o,NativeTu15UnitMaximumVitalityStats*stats){
    if(stats)++stats->calls;const auto destination=body(m,unit),information=damage_information(m,unit,stats);
    std::uint32_t health{},shield{};
    if(information){health=read(m,information+0x28u);shield=read(m,information+0x8cu);if(stats)++stats->default_vitality;}
    if(input.health){health=read(m,input.health);if(stats)++stats->explicit_health;}
    if(input.shield){shield=read(m,input.shield);if(stats)++stats->explicit_shield;}
    const auto one=read(m,0x46a0d0u);Simd environment(fp);
    store(m,o,0xd822fu,destination+0xe4u,health,stats);
    const auto has_health=positive(health);
    store(m,o,0xd823au,destination+0xe8u,shield,stats);
    const auto has_shield=positive(shield);
    store(m,o,0xd824du,destination+0xecu,has_health?one:0u,stats);
    store(m,o,0xd825au,destination+0xf0u,has_shield?one:0u,stats);
}
bool native_tu15_set_live_unit_maximum_vitality(SourcePhysicsContactMemory&m,std::uint32_t unit,
    NativeTu15UnitMaximumVitalityInput input,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&o,NativeTu15UnitMaximumVitalityStats*stats){
    if(stats)++stats->live_requests;
    if(unit==0xffffffffu){if(stats)++stats->none_units;return false;}
    if((read(m,body(m,unit)+0x10au,1)>>2u)&1u){if(stats)++stats->dead_units;return false;}
    native_tu15_set_unit_maximum_vitality(m,unit,input,fp,o,stats);if(stats)++stats->applied;return true;
}
}

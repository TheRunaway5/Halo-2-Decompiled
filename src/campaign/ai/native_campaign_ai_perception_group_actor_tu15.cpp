#include "halo2/native_campaign_ai_perception_group_actor_tu15.hpp"
#include "halo2/native_campaign_ai_perception_group_storage_tu15.hpp"
#include "halo2/native_campaign_object_world_point_tu15.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t at,unsigned width=4){std::array<std::byte,4>b{};m.read(at,std::span(b).first(width));std::uint32_t v{};for(unsigned n=0;n<width;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8u);return v;}
void write(SourcePhysicsContactMemory &m,std::uint32_t at,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(n*8u));m.write(at,b);}
std::uint32_t row(std::uint32_t rows,std::uint32_t datum,std::uint32_t stride){return rows+(datum&0xffffu)*stride;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class DistanceEnvironment {
    alignas(16) std::array<std::byte,108> ambient_{};
    std::array<std::byte,28> source_{};
    unsigned ambient_mxcsr_{};NativeTu15PhysicsFrameFp &fp_;
public:
    explicit DistanceEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp){
        if((fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u))throw std::runtime_error("Native perception distance requires its original FP trap owner");
        ambient_mxcsr_=_mm_getcsr();
        asm volatile("fnsave %0":"=m"(ambient_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(source_.data(),ambient_.data(),source_.size());const std::uint16_t empty=0xffffu;
        std::memcpy(source_.data(),&fp.control_word,2);std::memcpy(source_.data()+4,&fp.status,2);std::memcpy(source_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(source_):"memory");
    }
    ~DistanceEnvironment(){
        asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));
        asm volatile("frstor %0"::"m"(ambient_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(ambient_mxcsr_);
    }
};
std::uint32_t distance(std::array<std::uint32_t,3> actor,std::array<std::uint32_t,3> target){
    const float x=std::bit_cast<float>(actor[0]),y=std::bit_cast<float>(actor[1]),z=std::bit_cast<float>(actor[2]);
    const float tx=std::bit_cast<float>(target[0]),ty=std::bit_cast<float>(target[1]),tz=std::bit_cast<float>(target[2]);float result;
    // One balanced native extended-precision norm. These are scalar operands,
    // not a virtual FP register file. Source order is z*z+x*x+y*y, with the
    // original precision/rounding and the final single-precision store.
    asm volatile("flds %1; fsubs %2; flds %3; fsubs %4; flds %5; fsubs %6; "
        "fld %%st(0); fmul %%st(1),%%st; fld %%st(3); fmul %%st(4),%%st; faddp; "
        "fld %%st(2); fmul %%st(3),%%st; faddp; fsqrt; fstp %%st(3); "
        "fstp %%st(0); fstp %%st(0); fstps %0"
        :"=m"(result):"m"(x),"m"(tx),"m"(y),"m"(ty),"m"(z),"m"(tz):"memory","st","st(1)","st(2)","st(3)","st(4)");
    return std::bit_cast<std::uint32_t>(result);
}
#endif
}
void native_campaign_ai_perception_property_link_actor_tu15(SourcePhysicsContactMemory &m,
        std::uint32_t actor,std::uint32_t property,NativeTu15PhysicsFrameFp &fp,unsigned occupied_x87){
    const auto actors=read(m,0x5020c8u),actor_rows=read(m,actors+0x44u),record=row(actor_rows,actor,0x888u);
    const auto previous_head=read(m,record+0x58u),properties=read(m,0x50eef0u),property_rows=read(m,properties+0x44u);
    const auto member=row(property_rows,property,0x3cu);
    if(previous_head!=none)write(m,row(property_rows,previous_head,0x3cu)+0x30u,property);
    write(m,member+0x2cu,read(m,record+0x58u));write(m,record+0x58u,property);
    const auto object=read(m,member+0x20u);write(m,member+4u,actor);
    const auto point=native_campaign_object_world_point_value_tu15(m,object,fp);
    const auto x=read(m,record+0x238u);
    if(occupied_x87>3u)throw std::runtime_error("Native perception distance requires its original x87 stack-fault owner");
    const auto y=read(m,record+0x23cu),z=read(m,record+0x240u);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    DistanceEnvironment environment(fp);write(m,member+0x28u,distance({x,y,z},point));
#else
    (void)x;(void)y;(void)z;(void)fp;throw std::runtime_error("Exact native perception distance backend unavailable");
#endif
}
void native_campaign_ai_perception_group_link_actors_tu15(SourcePhysicsContactMemory &m,
        std::uint32_t clump,std::uint32_t group,NativeTu15PhysicsFrameFp &fp,unsigned occupied_x87){
    const auto groups=read(m,0x50eef4u),rows=read(m,groups+0x44u),offset=(group&0xffffu)*0xc4u;
    const auto previous=read(m,clump+0x14u);write(m,rows+offset+0x14u,previous);
    auto actor=read(m,clump+0x18u);write(m,clump+0x14u,group);
    while(actor!=none){
        const auto actors=read(m,0x5020c8u),actor_rows=read(m,actors+0x44u),current=actor;
        actor=read(m,row(actor_rows,current,0x888u)+0x80u);
        const auto property=native_campaign_ai_perception_property_create_tu15(m,group);
        if(property==none)continue;
        const auto current_groups=read(m,0x50eef4u),group_rows=read(m,current_groups+0x44u);
        const auto properties=read(m,0x50eef0u),property_rows=read(m,properties+0x44u),record=group_rows+offset;
        const auto previous_property=read(m,record+0x18u),member=row(property_rows,property,0x3cu);
        if(previous_property!=none)write(m,row(property_rows,previous_property,0x3cu)+0x38u,property);
        write(m,member+0x34u,previous_property);write(m,record+0x18u,property);write(m,member+8u,group);
        native_campaign_ai_perception_property_link_actor_tu15(m,current,property,fp,occupied_x87);
    }
}
}

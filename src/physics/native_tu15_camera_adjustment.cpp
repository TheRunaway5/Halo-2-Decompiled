#include "halo2/native_tu15_camera_adjustment.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::array<std::byte,4>b{};m_.read(p,std::span(b).first(width));std::uint32_t n{};for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(4);m_.read(p,e.before);e.written.assign(b.begin(),b.end());}m_.write(p,b);if(o_)o_(e);}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    void pop_angle(std::uint32_t pc,std::uint32_t p){
        alignas(16)std::array<std::byte,108>pending{};asm volatile("fnsave %0;frstor %0":"=m"(pending)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::uint32_t n;asm volatile("fstps %0":"=m"(n)::"memory","st");std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(4);m_.read(p,e.before);e.written.assign(b.begin(),b.end());}
        try{m_.write(p,b);}catch(...){asm volatile("frstor %0"::"m"(pending):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}if(o_)o_(e);
    }
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};NativeTu15PhysicsFrameFp&fp_;unsigned prior_;bool active_{};
    void validate(){if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native camera adjustment requires its numerical-fault owner");}
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),prior_(_mm_getcsr()){
        if(depth>4)throw std::runtime_error("Native camera adjustment requires its x87 capacity owner");validate();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(prior_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>decltype(auto)dependency(F&&f){publish();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto value=f();resume();return value;}}catch(...){resume();throw;}}
};
using F=__m128;
F scalar(std::uint32_t n){F out{};std::memcpy(&out,&n,4);return out;}
std::uint32_t bits(F n){std::uint32_t out;std::memcpy(&out,&n,4);return out;}
F mul(F a,F b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F add(F a,F b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F sub(F a,F b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F divide(F a,F b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
struct Comparison {bool below,equal;bool above()const{return!below&&!equal;}};
Comparison compare(F a,F b){unsigned char below,equal;asm volatile("comiss %3,%2;setb %0;sete %1":"=qm"(below),"=qm"(equal):"x"(a),"x"(b):"cc","memory");return{below!=0,equal!=0};}
#endif
}
void native_tu15_adjust_unit_camera_position(SourcePhysicsContactMemory&memory,
    std::uint32_t object,std::uint32_t position,std::uint32_t forward,std::uint32_t perpendicular,
    NativeTu15CameraAdjustmentScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15CameraAdjustmentStats*stats){
    if(stats)++stats->calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        Memory m(memory,observe);const auto data=scratch.data;
        const auto objects=m.word(0x4ecbd0u),headers=m.word(objects+0x44u),offset=(object&0xffffu)*12u,unit=m.word(headers+offset+8u),definition_datum=m.word(unit);
        m.word(0xe18f9u,data+4u,unit);const auto tags=m.word(0x4f0414u),definition=m.word(tags+(definition_datum&0xffffu)*16u+8u);
        if(stats)++stats->orthogonality_calls;(void)native_tu15_vectors_orthonormal(memory,forward,perpendicular,fp,depth);
        {
            Environment environment(fp,depth);const auto read=[&](std::uint32_t p){return scalar(m.word(p));};
            const auto work=[&]{
                const auto upper=read(definition+0x228u),lower=read(definition+0x224u);if(!compare(upper,lower).above())return;
                const auto zero=F{},negative_one=read(0x46a0dcu),z=read(forward+8u);const auto branch=compare(negative_one,z);
                auto delta=sub(upper,read(definition+0x224u));m.word(0xe1942u,data+8u,bits(delta));F angle;
                if(!branch.below)angle=read(0x41c4ccu);
                else if(!compare(zero,read(forward+8u)).above())angle=zero;
                else{
                    const auto vertical=m.word(forward+8u);asm volatile("flds %0;fchs"::"m"(vertical):"memory","st");
                    native_tu15_look_acos_active(memory);if(stats)++stats->downward_angles;
                    delta=read(data+8u);const auto half_pi=m.word(0x41c4ccu);asm volatile("fsubrs %0"::"m"(half_pi):"memory","st");
                    m.pop_angle(0xe1973u,data);angle=read(data);
                }
                auto parameter=divide(sub(angle,read(definition+0x224u)),delta);auto limit=zero;
                if(!compare(limit,parameter).above()){limit=read(0x46a0d0u);if(!compare(parameter,limit).above())limit=parameter;}
                const auto curved=environment.dependency([&]{return native_tu15_evaluate_transition_curve(memory,5u,bits(limit),fp,depth);});if(stats)++stats->curves;
                const auto positive=compare(scalar(curved),read(0x46a0e8u)).above();m.word(0xe19b4u,data,curved);if(!positive)return;
                const auto cached_unit=m.word(data+4u);if(!m.word(cached_unit+0x3c8u,1))return;
                environment.dependency([&]{native_tu15_camera_horizontal_basis(memory,forward,perpendicular,data+8u,data+16u,fp,depth,observe);});
                const auto current_objects=m.word(0x4ecbd0u),current_headers=m.word(current_objects+0x44u),current_unit=m.word(current_headers+offset+8u);
                const auto node_offset=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(m.word(current_unit+0x116u,2)))));
                const auto first_y=read(data+12u),first_x=read(data+8u);const auto original_unit=m.word(data+4u),node=current_unit+node_offset+0x28u;
                const auto pz=read(position+8u),nz=read(node+8u);const auto dz=sub(pz,nz);
                const auto px=read(position),nx=read(node);const auto dx=sub(px,nx);
                const auto py=read(position+4u),ny=read(node+4u);const auto dy=sub(py,ny);m.word(0xe1a2fu,data+36u,bits(dz));
                const auto first_y_dot=mul(first_y,dy),first_x_dot=mul(first_x,dx);const auto object_x=m.word(original_unit+0x3ccu);const auto first_dot=add(first_y_dot,first_x_dot);
                const auto second_y=read(data+20u),second_y_dot=mul(second_y,dy),second_x=read(data+16u);
                m.word(0xe1a62u,data+16u,object_x);const auto object_y=m.word(original_unit+0x3d0u),object_z=m.word(original_unit+0x3d4u);
                const auto second_x_dot=mul(second_x,dx),current_x=read(data+16u);const auto x_above=compare(current_x,read(definition+0x238u)).above();const auto second_dot=add(second_y_dot,second_x_dot);
                m.word(0xe1a84u,data+20u,object_y);m.word(0xe1a88u,data+24u,object_z);
                const auto selected_x=x_above?current_x:read(definition+0x238u);const auto difference_x=sub(selected_x,first_dot),current_y=read(data+20u),coefficient_x=read(definition+0x22cu);
                const auto difference_y=sub(current_y,second_dot),current_z=read(data+24u),difference_z=sub(current_z,read(data+36u));
                auto change_x=mul(coefficient_x,difference_x);const auto coefficient_y=read(definition+0x230u),angle_x=read(data);change_x=mul(change_x,angle_x);
                auto change_y=mul(coefficient_y,difference_y);change_y=mul(change_y,read(data));
                const auto coefficient_z=read(definition+0x234u);const auto second_y_change=mul(second_y,change_y),first_y_change=mul(first_y,change_x);
                auto new_y=add(second_y_change,first_y_change);new_y=add(new_y,read(position+4u));const auto second_x_change=mul(second_x,change_y);
                m.word(0xe1af8u,position+4u,bits(new_y));const auto current_position_z=read(position+8u);auto change_z=mul(coefficient_z,difference_z);change_z=mul(change_z,read(data));
                const auto first_x_change=mul(first_x,change_x);auto new_x=add(second_x_change,first_x_change);new_x=add(new_x,read(position));const auto new_z=add(current_position_z,change_z);
                m.word(0xe1b1cu,position,bits(new_x));m.word(0xe1b20u,position+8u,bits(new_z));if(stats)++stats->adjustments;
            };work();
        }
        if(stats)++stats->orthogonality_calls;(void)native_tu15_vectors_orthonormal(memory,forward,perpendicular,fp,depth);
        if(stats)++stats->completed;
#else
        (void)memory;(void)object;(void)position;(void)forward;(void)perpendicular;(void)scratch;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Exact native camera adjustment backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
}

#pragma once
// Boundary adapter for render-object draw flags
// (native_tu15_object_draw_flags_systems.hpp): 135E00, called by 1362A0 with
// the visibility row flags in DX and the view pointer on the stack (RET 4).
// Pure leaf: executes no child calls and no x87 instructions, only one MOVSS
// and one UCOMISS.
//
// Admission: TU1.5 image on x86-64, no MMX alias, the exact live
// instruction-cursor contract, and a certified required owner.
// HALO2_REFERENCE_OBJECT_DRAW_FLAGS=1 restores the translated body.
// HALO2_CENSUS_NATIVE_BRIDGES=1 also admits the uncursored census callbacks
// (the closure 135E00..135E9B holds no service PC).
#include "rendering/native_tu15_object_draw_flags_systems.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>
#include <cstdlib>
#include <thread>
#if defined(__x86_64__) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include <xmmintrin.h>
#endif

namespace halo2::native_object_draw_flags_detail {
namespace logic=halo2::native_object_draw_flags;
using logic::Access;

struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_OBJECT_DRAW_FLAGS");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if defined(__x86_64__) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    if(native_census_detail::uncursored(s,0x135e00u,0x135e9bu)){
    }else if(s.services.instruction){
        const auto*c=s.instruction_cursor;
        if(!s.literal_service_sites||!c||c!=&s.services.instruction_cursor||!c->position||!c->callback_type||
           s.services.instruction.target_type()!=*c->callback_type||c->executor!=std::this_thread::get_id()||
           !c->literal_service_sites||c->service_sites.size()!=3||c->service_sites[0]!=0x38846eu||
           c->service_sites[1]!=0x21f590u||c->service_sites[2]!=0x33b595u||
           (s.observe&&!c->observation_only_stores)||(c->first_trace&&*c->first_trace)||
           (c->second_trace&&*c->second_trace))return false;
    }else if(s.instruction_cursor||s.literal_service_sites||s.observe)return false;
    return true;
#else
    (void)s;return false;
#endif
}
template<class State>bool admitted(const State&s){return !reference()&&policy_admitted(s);}

#if defined(__x86_64__) && !defined(_MSC_VER)
struct Hardware {std::uint32_t value,flags;};
// SHR reg,2 with source flags seeded (count 2 leaves hardware-defined OF/AF
// observed by PUSH ESI at 135E09).
inline Hardware shift_right2(std::uint32_t value,std::uint32_t flags){
    auto ah=(flags&0xd5u)<<8u;unsigned char overflow;
    asm volatile("sahf;shrl $2,%1;lahf;seto %2":"+a"(ah),"+r"(value),"=qm"(overflow)::"cc");
    return{value,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)};
}
// AND reg,FFFF; SHL reg,4 (tag instance row offset, hardware OF/AF).
inline Hardware tag_row(std::uint32_t value,std::uint32_t flags){
    auto ah=(flags&0xd5u)<<8u;unsigned char overflow;
    asm volatile("sahf;andl $0xffff,%1;shll $4,%1;lahf;seto %2":"+a"(ah),"+r"(value),"=qm"(overflow)::"cc");
    return{value,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)};
}
inline std::uint32_t ucomiss(__m128 a,__m128 b){
    unsigned char below,equal,unordered;
    asm volatile("ucomiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(a),"x"(b):"cc","memory");
    return std::uint32_t(below)|(std::uint32_t(unordered)<<2u)|(std::uint32_t(equal)<<6u);
}

template<class State,class Before>class Adapter:public native_mopp_ray_caller_detail::Frame<State,Before> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
    std::uint32_t&r(unsigned n){return s_.gp[n];}
    static __m128 loaded_float(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}

    std::uint32_t before(Access a){
        using enum Access;auto&f=s_.flags;
        switch(a){
        // MOVZX EAX,DX; MOV ECX,EAX; PUSH EBX; SHR ECX,2; PUSH ESI; MOV ESI,EAX;
        // AND ECX,2C00; AND ESI,4000; OR ECX,ESI; AND EAX,200; SHL EAX,5;
        // SHR ECX,9; OR ECX,EAX
        case view_argument:{
            r(eax)=r(edx)&0xffffu;r(ecx)=r(eax);
            push(0x135e05u,r(ebx));
            const auto shifted=shift_right2(r(ecx),f.bits());r(ecx)=shifted.value;f.load(shifted.flags);
            push(0x135e09u,r(esi));
            r(esi)=r(eax);
            r(ecx)=f.logic(r(ecx)&0x2c00u,4);
            r(esi)=f.logic(r(esi)&0x4000u,4);
            r(ecx)=f.logic(r(ecx)|r(esi),4);
            r(eax)=f.logic(r(eax)&0x200u,4);
            r(eax)=f.shift(r(eax),5u,4);
            r(ecx)=f.right(r(ecx),9u,4,false);
            r(ecx)=f.logic(r(ecx)|r(eax),4);
            return 0x135e27u;
        }
        case view_flag_byte:return 0x135e2bu;
        case fog_mode:return 0x135e38u;
        case fog_tag:return 0x135e41u;
        case fog_density:return 0x135e4cu;
        case zero_constant:return 0x135e54u;
        case tag_instances:return 0x135e66u;
        case fog_definition:{
            const auto row=tag_row(r(esi),f.bits());r(esi)=row.value;f.load(row.flags);
            return 0x135e74u;
        }
        case fog_definition_flags:return 0x135e78u;
        case fog_override:return 0x135e83u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t v){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case view_argument:r(eax)=v;break;
        // MOV BL,[EAX+2A60]; TEST BL,BL; JNS; OR ECX,1
        case view_flag_byte:
            s_.set(ebx,v,1,0);f.logic(v&0xffu,1);
            if(v&0x80u)r(ecx)=f.logic(r(ecx)|1u,4);
            break;
        case fog_mode:f.minus(v,3u,4);break;
        case fog_tag:r(esi)=v;f.minus(v,0xffffffffu,4);break;
        case fog_density:s_.xmm[0]=loaded_float(v);break;
        case zero_constant:break; // UCOMISS runs in density_is_zero
        case tag_instances:r(eax)=v;break;
        case fog_definition:r(eax)=v;break;
        // TEST BYTE [EAX],2; JNZ; OR ECX,100
        case fog_definition_flags:
            f.logic(v&2u,1);
            if(!(v&2u))r(ecx)=f.logic(r(ecx)|0x100u,4);
            break;
        // MOV AL,[4C6824]; TEST AL,AL; JZ 135E91; TEST DH,4; JZ 135E97; OR ECX,80
        case fog_override:
            s_.set(eax,v,1,0);f.logic(v&0xffu,1);
            if(!(v&0xffu)){
                r(ecx)=f.logic(r(ecx)|0x80u,4);
            }else{
                const auto dh=(r(edx)>>8u)&0xffu;
                f.logic(dh&4u,1);
                if(dh&4u)r(ecx)=f.logic(r(ecx)|0x80u,4);
            }
            break;
        }
    }
    std::uint32_t load(Access a,std::uint32_t address,unsigned width){
        const auto pc=before(a);at(pc);const auto v=word(address,width);loaded(a,v);return v;
    }
public:
    Adapter(State&s,Before&before):Base{s,before}{}

    // ---- Memory
    std::uint32_t load32(Access a,std::uint32_t address){return load(a,address,4);}
    std::uint32_t load8(Access a,std::uint32_t address){return load(a,address,1);}
    float load_float(Access a,std::uint32_t address){return std::bit_cast<float>(load(a,address,4));}

    // ---- Frame
    std::uint32_t row_flags()const{return s_.gp[edx]&0xffffu;}
    std::uint32_t view_argument(){
        const auto pc=before(Access::view_argument);at(pc);const auto v=word(r(esp)+0xcu,4);loaded(Access::view_argument,v);return v;
    }
    // UCOMISS XMM0,[46A0E8]; LAHF; TEST AH,44; JP 135E83; TEST DH,4
    bool density_is_zero(float density,float zero){
        s_.xmm[0]=loaded_float(std::bit_cast<std::uint32_t>(density));
        const auto cmp=ucomiss(s_.xmm[0],loaded_float(std::bit_cast<std::uint32_t>(zero)));
        s_.flags.load(cmp);
        const auto ah=(cmp&0xd5u)|2u;
        s_.set(eax,ah,1,8);
        s_.flags.logic(ah&0x44u,1);
        if(s_.flags.pf)return false;
        const auto dh=(r(edx)>>8u)&0xffu;
        s_.flags.logic(dh&4u,1);
        return true;
    }
    // POP ESI; MOV EAX,ECX; POP EBX; RET 4.
    void finish(std::uint32_t flags){
        r(ecx)=flags;
        r(esi)=pop(0x135e97u);
        r(eax)=r(ecx);
        r(ebx)=pop(0x135e9au);
        pop(0x135e9bu);
        r(esp)+=4u;
    }
};
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
    if(!admitted(s))return false;
    Adapter adapter{s,before};logic::object_draw_flags::compute(adapter,adapter);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_object_draw_flags_detail

#pragma once
// Boundary adapter for render-view queue reset
// (native_tu15_view_reset_systems.hpp): 1373F0, pure leaf called with the
// RenderView in EDX (RET 0). Executes no FP instructions.
//
// Admission: TU1.5 image, no MMX alias, the exact live instruction-cursor
// contract, and a certified required owner. HALO2_REFERENCE_VIEW_RESET=1
// restores the translated body. HALO2_CENSUS_NATIVE_BRIDGES=1 also admits the
// uncursored census callbacks (1373F0..13745C holds no service PC).
#include "rendering/native_tu15_view_reset_systems.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <array>
#include <cstdlib>
#include <thread>
#if defined(__x86_64__) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#endif

namespace halo2::native_view_reset_detail {
namespace logic=halo2::native_view_reset;
using logic::Access;

struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_VIEW_RESET");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if defined(__x86_64__) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    if(native_census_detail::uncursored(s,0x1373f0u,0x13745cu)){
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
struct Hardware {std::uint32_t value,bytes,flags;};
// SHL ECX,2; MOV EBX,ECX; SHR ECX,2 (hardware OF/AF reaches REP STOSD at 137435).
inline Hardware shift_roundtrip(std::uint32_t value,std::uint32_t flags){
    auto ah=(flags&0xd5u)<<8u;std::uint32_t bytes{};unsigned char overflow;
    asm volatile("sahf;shll $2,%1;movl %1,%2;shrl $2,%1;lahf;seto %3":"+a"(ah),"+r"(value),"=r"(bytes),"=qm"(overflow)::"cc");
    return{value,bytes,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)};
}

template<class State,class Before>class Adapter:public native_mopp_ray_caller_detail::Frame<State,Before> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before>;
    using Base::s_;using Base::at;using Base::word;using Base::io;using Base::push;using Base::pop;using Base::store;
    enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
    std::uint32_t&r(unsigned n){return s_.gp[n];}
    void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        io([&]{s_.memory.write(address,std::span(bytes).first(width));});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);io([&]{s_.observe(event);});}
    }
    void stosd(std::uint32_t pc,std::uint32_t dst,std::uint32_t count,std::uint32_t value){
        r(eax)=value;r(ecx)=count;r(edi)=dst;
        const auto delta=(s_.other_flags&0x400u)?0xfffffffcu:4u;
        while(r(ecx)){
            at(pc);
            store(pc,r(edi),r(eax),[&]{r(edi)+=delta;--r(ecx);},true);
        }
    }
    std::uint32_t before(Access a){
        using enum Access;
        switch(a){
        case queue0:return 0x1373f0u;
        case clear_queue0:return 0x1373f7u;
        case queue1:return 0x1373fbu;
        case clear_queue1:return 0x1373feu;
        case queue2:return 0x137402u;
        case clear_queue2:return 0x137405u;
        case queue3:return 0x137409u;
        case clear_queue3:return 0x13740cu;
        case entry_count:return 0x137420u;
        case clear_entry_count:return 0x13743eu;
        case clear_active_count:return 0x137445u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t v){
        using enum Access;
        switch(a){
        case queue0:r(eax)=v;break;
        case queue1:r(ecx)=v;break;
        case queue2:r(eax)=v;break;
        case queue3:r(ecx)=v;break;
        case entry_count:r(ecx)=v&0xffffu;break;
        default:break;
        }
    }
    std::uint32_t load(Access a,std::uint32_t address,unsigned width){
        const auto pc=before(a);at(pc);const auto v=word(address,width);loaded(a,v);return v;
    }
public:
    Adapter(State&s,Before&before):Base{s,before}{}

    // ---- Memory
    std::uint32_t load32(Access a,std::uint32_t address){return load(a,address,4);}
    std::uint32_t load16(Access a,std::uint32_t address){return load(a,address,2);}
    void store16(Access a,std::uint32_t address,std::uint32_t value){write(before(a),address,value&0xffffu,2);}

    // ---- Frame
    std::uint32_t entry_view()const{return s_.gp[edx];}
    void save_prolog(){
        push(0x1373f3u,r(ebx));
        push(0x1373f4u,r(esi));
        r(esi)=s_.flags.logic(0u,4);
    }
    void fill_bitmask(std::uint32_t dst,std::uint32_t dwords,std::uint32_t value){
        push(0x137410u,r(edi));
        r(eax)=s_.flags.logic(value,4);
        stosd(0x13741eu,dst,dwords,r(eax));
    }
    void fill_entries(std::uint32_t dst,std::uint32_t count,std::uint32_t value){
        const auto rt=shift_roundtrip(count&0xffffu,s_.flags.bits());
        r(ebx)=rt.bytes;s_.flags.load(rt.flags);
        stosd(0x137435u,dst,rt.value,value);
        r(ecx)=s_.flags.logic(r(ebx)&3u,4);
    }
    void fill_cluster_slots(std::uint32_t dst,std::uint32_t dwords,std::uint32_t value){
        r(eax)=s_.flags.logic(r(eax)|value,4);
        stosd(0x137457u,dst,dwords,r(eax));
    }
    void finish(){
        r(edi)=pop(0x137459u);
        r(esi)=pop(0x13745au);
        r(ebx)=pop(0x13745bu);
        pop(0x13745cu);
    }
};
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
    if(!admitted(s))return false;
    Adapter adapter{s,before};logic::view_reset::reset(adapter,adapter);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_view_reset_detail

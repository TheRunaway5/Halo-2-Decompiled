#pragma once
// Boundary adapter for the object parent-hierarchy attached predicate
// (native_tu15_object_hierarchy_root_systems.hpp): BBD10, called with the
// object handle in ECX (RET 0). Pure leaf: executes no child calls and no FP
// instructions.
//
// Admission: TU1.5 image on x86-64, no MMX alias, the exact live
// instruction-cursor contract, and a certified required owner.
// HALO2_REFERENCE_OBJECT_HIERARCHY_ROOT=1 restores the translated body.
// HALO2_CENSUS_NATIVE_BRIDGES=1 also admits the uncursored census callbacks
// (the closure BBD10..BBD47 holds no service PC).
#include "rendering/native_tu15_object_hierarchy_root_systems.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>
#include <thread>
#if defined(__x86_64__) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#endif

namespace halo2::native_object_hierarchy_root_detail {
namespace logic=halo2::native_object_hierarchy_root;
using logic::Access;

struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_OBJECT_HIERARCHY_ROOT");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if defined(__x86_64__) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    if(native_census_detail::uncursored(s,0xbbd10u,0xbbd47u)){
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
template<class State,class Before>class Adapter:public native_mopp_ray_caller_detail::Frame<State,Before> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
    std::uint32_t&r(unsigned n){return s_.gp[n];}

    std::uint32_t before(Access a){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case table_pointer:return 0xbbd10u;
        // PUSH ESI
        case table_data:push(0xbbd15u,r(esi));return 0xbbd16u;
        // AND ECX,FFFF; LEA ECX,[ECX+ECX*2]
        case header_flags:
            r(ecx)=f.logic(r(ecx)&0xffffu,4);
            r(ecx)=r(ecx)*3u;
            return 0xbbd29u;
        case header_object:return 0xbbd2eu;
        case object_flags:return 0xbbd37u;
        case parent_handle:return 0xbbd3cu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t v){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case table_pointer:r(eax)=v;break;
        case table_data:r(esi)=v;break;
        case header_flags:f.logic(v&0x10u,1);break;
        case header_object:r(edx)=v;break;
        case object_flags:f.logic(v&(r(eax)&0xffu),1);break;
        case parent_handle:r(ecx)=v;f.minus(v,0xffffffffu,4);break;
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

    // ---- Frame
    std::uint32_t entry_handle()const{return s_.gp[ecx];}
    // MOV AL,1 (BBD19) after ESI = table->data.
    void begin_walk(std::uint32_t){s_.set(eax,1u,1,0);}
    // LEA ECX,[ESI+ECX*4] (BBD32).
    void step_header(std::uint32_t row,std::uint32_t,std::uint32_t){r(ecx)=row;}
    // If !result: XOR AL,AL (BBD44). Then POP ESI (BBD46); RET (BBD47).
    void finish(bool result){
        if(!result)s_.set(eax,s_.flags.logic(0u,1),1,0);
        r(esi)=pop(0xbbd46u);
        pop(0xbbd47u);
    }
};
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
    if(!admitted(s))return false;
    Adapter adapter{s,before};logic::object_hierarchy_root::check(adapter,adapter);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_object_hierarchy_root_detail

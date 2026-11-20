#pragma once
// Boundary adapter for the PRT object partition
// (native_tu15_prt_partition_systems.hpp): 136D50, called by 136230 with the
// view as its one stack argument (stdcall, RET 4). Its caller and the queue
// swap 166C30 are separate owners, so this adapter owns that ABI and nothing
// else:
//   - the saved registers (EBX, ESI always; EBP, EDI once the queue is not
//     empty), the argument slot and every push, pop and return address;
//   - every register as the original leaves it at each access, at the swap
//     and at the exit, with the flags of each instruction. The three tag
//     lookups (AND EAX,FFFF; SHL EAX,4) run on the host with the source
//     flags seeded, because SHL's count-dependent OF/AF reach the next read;
//   - the values the logic keeps across the swap (Kept), loaded into their
//     callee-saved registers before it and read back after it;
//   - the diagnostic cursor (each access publishes its original instruction)
//     and the store stream (every push goes through the provider and the
//     store observer).
// The original executes no x87, MMX or SSE instruction, so there is no FP
// scope.
//
// The swap 166C30 has no body in the Shadow unit: it is entered through the
// unit's dispatcher (the Call supplied by the generated owner), which hands
// it to the State's required boundary.
//
// Admission: TU1.5 image, no MMX alias, the exact live instruction-cursor
// contract, and a certified required owner (the native parent skips the
// remaining instruction callbacks after a required owner returns).
// HALO2_REFERENCE_PRT_PARTITION=1 restores the translated body.
// HALO2_CENSUS_NATIVE_BRIDGES=1 also admits the uncursored census callbacks
// (the closure 136D50..136E09 holds no service PC).
#include "rendering/native_tu15_prt_partition_systems.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#endif

namespace halo2::native_prt_partition_detail {
namespace logic=halo2::native_prt_partition;
using logic::Access;
using logic::Kept;

struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_PRT_PARTITION");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    if(native_census_detail::uncursored(s,0x136d50u,0x136e09u)){
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

inline constexpr std::uint32_t swap_entry=0x166c30u;
inline constexpr std::uint32_t swap_site=0x136de8u;

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// AND EAX,FFFF; SHL EAX,4 (a tag instance's offset), with the source flags
// seeded (SAHF) in the same assembly block.
struct Hardware {std::uint32_t value,flags;};
inline Hardware tag_row(std::uint32_t tag,std::uint32_t flags){
    auto ah=(flags&0xd5u)<<8u;unsigned char overflow;
    asm volatile("sahf;andl $0xffff,%1;shll $4,%1;lahf;seto %2":"+a"(ah),"+r"(tag),"=qm"(overflow)::"cc");
    return{tag,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)};
}

template<class State,class Call,class Before>class Adapter:public native_mopp_ray_caller_detail::Frame<State,Before> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before>;
    using Base::s_;using Base::before_;using Base::at;using Base::word;using Base::push;using Base::pop;
    enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
    Call&call_;
    // The entry just examined has PRT data (INC EBX is pending), and whether
    // the swap ran (otherwise CMP ESI,EBX precedes the INC).
    bool found_{},swapped_{};
    std::uint32_t&r(unsigned n){return s_.gp[n];}
    void tag_offset(){const auto row=tag_row(r(eax),s_.flags.bits());r(eax)=row.value;s_.flags.load(row.flags);}

    // The original instruction of each role, with the register work between
    // the previous access and it.
    std::uint32_t before(Access a){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case entry_view:return 0x136d50u;
        case entry_queue:return 0x136d54u;
        // PUSH EBX; PUSH ESI; XOR ESI,ESI; XOR EBX,EBX
        case entry_count:push(0x136d57u,r(ebx));push(0x136d58u,r(esi));r(esi)=f.logic(0u,4);r(ebx)=f.logic(0u,4);return 0x136d5du;
        case object_headers:push(0x136d67u,r(ebp));return 0x136d68u;
        case tag_instances:push(0x136d6eu,r(edi));return 0x136d6fu;
        case queue_handles:return 0x136d75u;
        case queue_handle:return 0x136d7bu;
        // AND EAX,FFFF; LEA ECX,[EAX+EAX*2]
        case object_rows:r(eax)=f.logic(r(eax)&0xffffu,4);r(ecx)=r(eax)*3u;return 0x136d8bu;
        case object_data:return 0x136d8eu;
        case object_definition_tag:return 0x136d96u;
        case object_definition:tag_offset();return 0x136da5u;
        case model_tag:return 0x136dadu;
        case model_definition:tag_offset();return 0x136dbdu;
        case render_model_tag:return 0x136dc5u;
        case render_model:tag_offset();return 0x136dd5u;
        case prt_info_count:return 0x136dd9u;
        // [CMP ESI,EBX;] INC EBX for a PRT entry.
        case next_view:
            if(found_){if(!swapped_)f.minus(r(esi),r(ebx),4);r(ebx)=f.increment(r(ebx),4,false);}
            found_=swapped_=false;return 0x136deeu;
        case next_queue:return 0x136df2u;
        case next_count:return 0x136df5u;
        }
        return 0;
    }
    // The access's own effect (the destination register, a compare's flags).
    void loaded(Access a,std::uint32_t v){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case entry_view:r(eax)=v;break;
        case entry_queue:r(edx)=v;break;
        case entry_count:f.minus(v,r(esi),2);break;                                // CMP [EDX+4],SI
        case object_headers:r(ebp)=v;break;
        case tag_instances:r(edi)=v;break;
        case queue_handles:r(eax)=v;r(ecx)=logic::widen16(r(esi));break;           // MOVSX ECX,SI
        case queue_handle:case object_definition_tag:case model_tag:case render_model_tag:
            r(eax)=v;f.minus(v,0xffffffffu,4);break;                               // CMP EAX,-1
        case object_rows:r(eax)=v;break;
        case object_data:case object_definition:case model_definition:r(eax)=v;f.logic(v,4);break;  // TEST EAX,EAX
        case render_model:r(ecx)=v;break;
        case prt_info_count:r(eax)=v;f.logic(v,4);found_=logic::signed_value(v)>0;break;
        case next_view:r(edx)=v;break;
        case next_queue:r(edx)=v;break;
        // MOVSX EAX; INC ESI; CMP ESI,EAX
        case next_count:r(eax)=logic::widen16(v);r(esi)=f.increment(r(esi),4,false);f.minus(r(esi),r(eax),4);break;
        }
    }
    std::uint32_t load(Access a,std::uint32_t address,unsigned width){
        const auto pc=before(a);at(pc);const auto v=word(address,width);loaded(a,v);return v;
    }
public:
    Adapter(State&s,Call&call,Before&before):Base{s,before},call_(call){}

    // ---- Memory
    std::uint32_t load32(Access a,std::uint32_t address){return load(a,address,4);}
    std::uint32_t load16(Access a,std::uint32_t address){return load(a,address,2);}

    // ---- Frame
    // The view: [ESP+4] at entry, [ESP+14] inside the loop (four saved
    // registers above the return address).
    std::uint32_t argument(Access a){return load(a,r(esp)+(a==Access::entry_view?4u:0x14u),4);}
    // CMP ESI,EBX; MOV ECX,EBX; MOV EAX,ESI; CALL 166C30 (the queue in EDX).
    void swap(Kept&k,std::uint32_t queue){
        r(esi)=k.index;r(ebx)=k.front;r(ebp)=k.objects;r(edi)=k.tags;r(edx)=queue;
        s_.flags.minus(r(esi),r(ebx),4);r(ecx)=r(ebx);r(eax)=r(esi);
        push(swap_site,swap_site+5u);call_(s_,swap_entry,swap_site,before_);
        k.index=r(esi);k.front=r(ebx);k.objects=r(ebp);k.tags=r(edi);swapped_=true;
    }
    // [POP EDI; POP EBP;] POP ESI; POP EBX; RET 4.
    void finish(bool looped){
        if(looped){r(edi)=pop(0x136e02u);r(ebp)=pop(0x136e03u);}
        r(esi)=pop(0x136e04u);r(ebx)=pop(0x136e05u);pop(0x136e06u);r(esp)+=4u;
    }
};
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call call,Before before={}){
    if(!admitted(s))return false;
    Adapter adapter{s,call,before};logic::prt_partition::partition(adapter,adapter);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_prt_partition_detail

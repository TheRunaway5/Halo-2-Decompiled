#pragma once
// Boundary adapter for the render-object submission loop
// (native_tu15_object_pass_systems.hpp): 136230, called with the view in EAX
// and one stack argument (RET 4). Calls 136D50 (PRT partition) and 1362A0
// (object draw submission) through the unit's dispatcher. Executes no FP
// instructions.
//
// Admission: TU1.5 image, no MMX alias, the exact live instruction-cursor
// contract, and a certified required owner. HALO2_REFERENCE_OBJECT_PASS=1
// restores the translated body. HALO2_CENSUS_NATIVE_BRIDGES=1 also admits the
// uncursored census callbacks (136230..136291 holds no service PC).
#include "rendering/native_tu15_object_pass_systems.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <array>
#include <cstdlib>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#endif

namespace halo2::native_object_pass_detail {
namespace logic=halo2::native_object_pass;
using logic::Access;
using logic::Kept;

struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_OBJECT_PASS");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    if(native_census_detail::uncursored(s,0x136230u,0x136291u)){
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

inline constexpr std::uint32_t partition_entry=0x136d50u;
inline constexpr std::uint32_t submit_entry=0x1362a0u;

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State,class Call,class Before>class Adapter:public native_mopp_ray_caller_detail::Frame<State,Before> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before>;
    using Base::s_;using Base::before_;using Base::at;using Base::word;using Base::io;using Base::push;using Base::pop;
    enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
    Call&call_;
    std::uint32_t&r(unsigned n){return s_.gp[n];}
    void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        io([&]{s_.memory.write(address,std::span(bytes).first(width));});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);io([&]{s_.observe(event);});}
    }
    std::uint32_t before(Access a){
        using enum Access;auto&f=s_.flags;
        switch(a){
        // PUSH ECX; PUSH EBX; PUSH EBP
        case argument:
            push(0x136230u,r(ecx));push(0x136231u,r(ebx));push(0x136232u,r(ebp));
            return 0x136233u;
        case entry_queue:return 0x136241u;
        // XOR EBX,EBX
        case local_clear:r(ebx)=f.logic(0u,4);return 0x136246u;
        // XOR EDI,EDI
        case entry_count:r(edi)=f.logic(0u,4);return 0x13624cu;
        case retry_clear:return 0x136250u;
        case retry_word:return 0x136256u;
        case retry_reset:return 0x13626bu;
        case view_kind:return 0x136271u;
        case retry_set:return 0x13627au;
        case next_queue:return 0x136280u;
        case next_count:return 0x136283u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t v){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case argument:r(ebp)=v;break;
        case entry_queue:r(eax)=v;break;
        case entry_count:f.minus(v,r(ebx),2);break;
        case retry_word:r(edx)=v;break;
        case view_kind:f.minus(v,1u,4);break;
        case next_queue:r(eax)=v;break;
        // MOVSX ECX,WORD [EAX+4]; INC EDI; CMP EDI,ECX
        case next_count:
            r(ecx)=logic::widen16(v);
            r(edi)=f.increment(r(edi),4,false);
            f.minus(r(edi),r(ecx),4);
            break;
        default:break;
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
    void store32(Access a,std::uint32_t address,std::uint32_t value){write(before(a),address,value,4);}
    void store8(Access a,std::uint32_t address,std::uint32_t value){write(before(a),address,value&0xffu,1);}

    // ---- Frame
    std::uint32_t entry_view()const{return s_.gp[eax];}
    // At 136233: after 3 pushes (ECX, EBX, EBP), [ESP+0x10] is the caller's argument.
    std::uint32_t argument(){
        const auto pc=before(Access::argument);at(pc);const auto v=word(r(esp)+0x10u,4);loaded(Access::argument,v);return v;
    }
    // After PUSH ESI, PUSH EDI (total 5 pushes): local slot is [ESP+0x10],
    // caller's argument slot (reused as retry byte) is [ESP+0x18].
    std::uint32_t local_slot()const{return s_.gp[esp]+0x10u;}
    std::uint32_t retry_slot()const{return s_.gp[esp]+0x18u;}
    // PUSH ESI; PUSH EDI; MOV ESI,EAX; PUSH ESI; CALL 136D50 (RET 4).
    void partition(Kept&k){
        push(0x136237u,r(esi));push(0x136238u,r(edi));
        r(esi)=k.view;
        push(0x13623bu,r(esi));
        push(0x13623cu,0x136241u);
        call_(s_,partition_entry,0x13623cu,before_);
        k.zero=r(ebx);k.parameter=r(ebp);k.view=r(esi);k.index=r(edi);
    }
    // PUSH EBP; PUSH -1; LEA ECX,[ESP+18]; PUSH ECX; PUSH EDX; PUSH EDI; CALL 1362A0 (RET 20); CMP AL,BL.
    std::uint32_t submit(Kept&k,std::uint32_t retry_word,std::uint32_t local_address){
        r(ebx)=k.zero;r(ebp)=k.parameter;r(esi)=k.view;r(edi)=k.index;r(edx)=retry_word;
        push(0x13625au,r(ebp));
        push(0x13625bu,0xffffffffu);
        r(ecx)=local_address;
        push(0x136261u,r(ecx));
        push(0x136262u,r(edx));
        push(0x136263u,r(edi));
        push(0x136264u,0x136269u);
        call_(s_,submit_entry,0x136264u,before_);
        k.zero=r(ebx);k.parameter=r(ebp);k.view=r(esi);k.index=r(edi);
        s_.flags.minus(r(eax)&0xffu,r(ebx)&0xffu,1);
        return r(eax)&0xffu;
    }
    // DEC EDI (13627F).
    void retry_step(Kept&k){
        r(edi)=s_.flags.increment(r(edi),4,true);
        k.index=r(edi);
    }
    // POP EDI; POP ESI; POP EBP; POP EBX; POP ECX; RET 4.
    void finish(){
        r(edi)=pop(0x13628cu);
        r(esi)=pop(0x13628du);
        r(ebp)=pop(0x13628eu);
        r(ebx)=pop(0x13628fu);
        r(ecx)=pop(0x136290u);
        pop(0x136291u);
        r(esp)+=4u;
    }
};
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call call,Before before={}){
    if(!admitted(s))return false;
    Adapter adapter{s,call,before};logic::object_pass::run(adapter,adapter);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_object_pass_detail

#pragma once
#include "halo2/native_tu15_model_draw_submission_kernel.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#endif

namespace halo2::native_model_draw_submission_detail {
// Retained ABI view for the shared typed 136A80 submission policy. The body
// executes no x87/MMX instruction, so provider calls need no FP parking; the
// 2CBD0 draw unit remains its own owner behind the State's required boundary.
struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_MODEL_DRAW_SUBMISSION");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    // The returning 2CBD0 owner must certify that it preserves the caller's
    // instruction callback, cursor and trace policy (REQUIRED_OWNER_POLICY).
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    // Census diagnostic: full callbacks without a cursor. The 136A80 body
    // holds no service PC; 2CBD0 keeps its own required-owner route.
    if(native_census_detail::uncursored(s,0x136a80u,0x136d46u)){
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
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// AND then SHL by 4: SHL's count-dependent OF/AF reach the next access.
inline std::pair<std::uint32_t,std::uint32_t> owner_row(std::uint32_t value,std::uint32_t flags){
    auto ah=(flags&0xd5u)<<8u;unsigned char overflow;
    asm volatile("sahf;andl $0xffff,%1;shll $4,%1;lahf;seto %2":"+a"(ah),"+r"(value),"=qm"(overflow)::"cc","memory");
    return{value,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)|2u};
}
template<class State,class Before>class View:public native_mopp_ray_caller_detail::Frame<State,Before> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before>;
    using Base::s_;using Base::at;using Base::word;using Base::io;using Base::push;using Base::pop;using Base::store;
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4){at(pc);return word(address,width);}
    void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4,bool positioned=false){
        if(!positioned)at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        io([&]{s_.memory.write(address,std::span(bytes).first(width));});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);io([&]{s_.observe(event);});}
    }
    void byte(unsigned reg,std::uint32_t value){s_.set(reg,value,1,0);}
    static std::uint32_t signed_half(std::uint32_t value){return std::uint32_t(std::int32_t(std::int16_t(value)));}
    bool less()const{return s_.flags.sf!=s_.flags.of;}
    bool less_equal()const{return s_.flags.zf||s_.flags.sf!=s_.flags.of;}
public:
    View(State&s,Before&before):Base{s,before}{}
    bool begin(){
        auto&g=s_.gp;push(0x136a80u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff8u,4);g[4]=s_.flags.minus(g[4],0x214u,4);
        push(0x136a8cu,g[3]);push(0x136a8du,g[6]);g[1]=s_.flags.logic(0u,4);s_.flags.minus(read(0x136a90u,g[0]+0x166u,2),0u,2);
        g[6]=g[0];push(0x136a99u,g[7]);write(0x136a9au,g[4]+0x20u,g[6]);write(0x136a9eu,g[4]+12u,g[1]);
        if(less_equal())return false;
        g[0]=s_.flags.plus(g[0],0x48u,4);write(0x136aabu,g[4]+0x1cu,g[1]);write(0x136aafu,g[4]+0x14u,g[1]);g[1]=g[0];g[3]=0x18u;
        write(0x136abau,g[4]+0x34u,g[0]);write(0x136abeu,g[4]+0x28u,g[1]);return true;
    }
    // Later sections may require their first part to match section 0's.
    bool skip_section(){
        auto&g=s_.gp;s_.flags.minus(g[3],0x18u,4);if(less_equal())return false;
        byte(2,read(0x136ad5u,g[5]+8u,1));s_.flags.logic(g[2]&255u,1);if(s_.flags.zf)return false;
        g[2]=read(0x136adcu,g[1]);g[0]=read(0x136adeu,g[0]);g[2]=read(0x136ae0u,g[2]);
        s_.flags.minus(g[2],read(0x136ae2u,g[0]),4);return !s_.flags.zf;
    }
    bool owned(){s_.flags.minus(read(0x136aeau,s_.gp[6]+0x1c4u),1u,4);return s_.flags.zf;}
    bool unit_owner(){
        auto&g=s_.gp;g[0]=read(0x136af7u,g[1]);g[0]=read(0x136af9u,g[0]);g[2]=read(0x136afbu,0x4ecbd0u);
        g[0]=s_.flags.logic(g[0]&0xffffu,4);g[1]=g[0]*3u;g[0]=read(0x136b09u,g[2]+0x44u);g[0]=read(0x136b0cu,g[0]+g[1]*4u+8u);
        s_.flags.minus(read(0x136b10u,g[0]+0xaau,1),2u,1);return s_.flags.zf;
    }
    bool shadowed(){
        auto&g=s_.gp;g[1]=read(0x136b19u,g[0]);g[2]=read(0x136b1bu,0x4f0414u);
        const auto row=owner_row(g[1],s_.flags.bits());g[1]=row.first;s_.flags.load(row.second);
        g[0]=read(0x136b2au,g[1]+g[2]+8u);s_.flags.minus(read(0x136b2eu,g[0]+0x290u,2),0u,2);
        byte(0,!s_.flags.zf);s_.flags.logic(g[0]&255u,1);return !s_.flags.zf;
    }
    // The record copy and its patched override fields replace ESI.
    void copy_record(){
        auto&g=s_.gp;g[6]=read(0x136b3du,g[4]+0x20u);s_.xmm[0]=_mm_setzero_ps();g[1]=0x79u;g[7]=g[4]+0x38u;
        const auto delta=(s_.other_flags&0x400u)?0xfffffffcu:4u;
        // Each iteration retires (ESI/EDI/ECX advance) before its completed
        // store is observed, as on the CPU.
        while(g[1]){at(0x136b4du);const auto value=word(g[6]);store(0x136b4du,g[7],value,[&]{g[6]+=delta;g[7]+=delta;--g[1];},true);}
        g[0]=s_.flags.logic(0xffffffffu,4);write(0x136b52u,g[4]+0x204u,0u,1);write(0x136b5au,g[4]+0x1f8u,0x1fu);
        write(0x136b65u,g[4]+0x1fcu,g[0]);write(0x136b6cu,g[4]+0x208u,g[0]);write(0x136b73u,g[4]+0x20cu,0u,1);
        write(0x136b7bu,g[4]+0x210u,0u);g[6]=g[4]+0x38u;
    }
    bool parts(){
        auto&g=s_.gp;g[0]=read(0x136b88u,g[6]+0x170u);g[1]=read(0x136b8eu,g[4]+12u);byte(2,read(0x136b92u,g[1]+g[6]+0x70u,1));
        g[7]=g[0];g[7]=s_.flags.logic(g[7]&4u,4);g[7]=s_.flags.shift(g[7],6u,4);g[0]=s_.flags.logic(g[0]&0x40u,4);g[7]=s_.flags.logic(g[7]|g[0],4);
        byte(0,read(0x136ba3u,g[6]+0x16cu,1));g[7]=s_.flags.shift(g[7],7u,4);byte(2,s_.flags.minus(0u,g[2]&255u,1));
        g[1]=read(0x136baeu,g[6]+0x1acu);g[2]=s_.flags.borrow(g[2],g[2],4);g[2]=s_.flags.logic(g[2]&0x800u,4);g[7]=s_.flags.logic(g[7]|g[2],4);
        g[2]=read(0x136bbeu,g[6]+0x1b0u);byte(0,s_.flags.minus(0u,g[0]&255u,1));g[0]=s_.flags.borrow(g[0],g[0],4);
        g[0]=s_.flags.logic(g[0]&0x400u,4);g[7]=s_.flags.logic(g[7]|g[0],4);
        g[0]=read(0x136bcfu,g[3]+g[6]);g[7]=s_.flags.logic(g[7]|g[2],4);g[2]=s_.flags.logic(0u,4);g[7]=s_.flags.logic(g[7]|g[1],4);
        s_.flags.logic(g[0],4);write(0x136bdau,g[4]+0x18u,g[2]);return !less_equal();
    }
    void begin_parts(){auto&g=s_.gp;g[1]=read(0x136be4u,g[4]+0x14u);g[0]=g[1]+g[6]+0xc6u;write(0x136befu,g[4]+0x10u,g[0]);}
    // One part: its texture-slot override, two optional side bytes, then the
    // 21 ordered draw-unit arguments and the 2CBD0 required boundary.
    void submit(){
        auto&g=s_.gp;g[1]=read(0x136c00u,g[4]+0x1cu);g[0]=g[1]+g[6]+0x75u;byte(0,read(0x136c08u,g[0]+g[2],1));s_.flags.minus(g[0]&255u,0xffu,1);
        if(s_.flags.zf)write(0x136c18u,g[4]+0x24u,0xffffffffu);
        else{g[0]&=255u;write(0x136c12u,g[4]+0x24u,g[0]);}
        g[1]=read(0x136c20u,g[6]+0x170u);s_.flags.logic(g[1]&2u,1);write(0x136c29u,g[4]+0x2cu,0u,1);write(0x136c2eu,g[4]+0x30u,0u,1);
        if(!s_.flags.zf){
            g[0]=g[1];g[0]=s_.flags.right(g[0],3u,4,false);byte(0,s_.flags.logic(g[0]&1u,1));
            g[1]=s_.flags.right(g[1],4u,4,false);byte(1,s_.flags.logic(g[1]&1u,1));
            write(0x136c42u,g[4]+0x2cu,g[0]&255u,1);write(0x136c46u,g[4]+0x30u,g[1]&255u,1);
        }
        g[1]=read(0x136c4au,g[6]+0x1a0u);push(0x136c50u,g[1]);g[0]=read(0x136c51u,g[4]+0x14u);g[1]=g[6]+0x194u;push(0x136c5bu,g[1]);
        g[1]=s_.flags.logic(0u,4);s_.set(1,read(0x136c5eu,g[0],2),2,0);g[0]=read(0x136c61u,g[6]+0x1e0u);push(0x136c67u,g[1]);
        g[1]=read(0x136c68u,g[6]+0x1dcu);push(0x136c6eu,g[0]);g[0]=read(0x136c6fu,g[6]+0x1d8u);push(0x136c75u,g[1]);push(0x136c76u,g[0]);
        g[0]=read(0x136c77u,g[4]+0x48u);g[1]=s_.flags.logic(0u,4);byte(1,read(0x136c7du,g[6]+0x1d4u,1));push(0x136c83u,g[1]);
        g[1]=read(0x136c84u,g[4]+0x48u);push(0x136c88u,g[0]);g[0]=read(0x136c89u,g[6]+0x1c4u);push(0x136c8fu,g[1]);
        g[1]=read(0x136c90u,g[6]+0x1c0u);push(0x136c96u,g[0]);g[0]=read(0x136c97u,g[6]+0x1b4u);push(0x136c9du,g[1]);
        g[1]=read(0x136c9eu,g[6]+0x1d0u);push(0x136ca4u,g[0]);push(0x136ca5u,g[1]);
        g[0]=s_.flags.logic(0u,4);byte(0,read(0x136ca8u,g[6]+0x1bcu,1));g[1]=s_.flags.logic(0u,4);s_.set(1,read(0x136cb0u,g[6]+0x1beu,2),2,0);
        push(0x136cb7u,0u);push(0x136cb9u,g[0]);g[0]=read(0x136cbau,g[3]+g[6]+0x30u);push(0x136cbeu,g[1]);
        g[1]=read(0x136cbfu,g[4]+0x4cu);push(0x136cc3u,g[0]);g[0]=read(0x136cc4u,g[3]+g[6]-0x14u);push(0x136cc8u,g[2]);
        g[2]=read(0x136cc9u,g[1]+g[6]+0x2cu,1);g[1]=read(0x136cceu,g[3]+g[6]+0x44u);push(0x136cd2u,g[0]);
        g[0]=read(0x136cd3u,g[4]+0x70u);push(0x136cd7u,g[1]);s_.set(1,read(0x136cd8u,g[6]+0x16au,2),2,0);push(0x136cdfu,g[7]);
        push(0x136ce0u,0x136ce5u);s_.required(0x136ce0u,0x2cbd0u,false);
    }
    bool next_part(){
        auto&g=s_.gp;g[0]=read(0x136ce5u,g[4]+0x18u);g[2]=read(0x136ce9u,g[4]+0x10u);g[1]=read(0x136cedu,g[3]+g[6]);
        g[0]=s_.flags.increment(g[0],4,false);g[2]=s_.flags.plus(g[2],2u,4);s_.flags.minus(g[0],g[1],4);
        write(0x136cf6u,g[4]+0x18u,g[0]);write(0x136cfau,g[4]+0x10u,g[2]);
        if(!less())return false;
        g[2]=read(0x136bf5u,g[4]+0x18u);return true;
    }
    bool next_section(){
        auto&g=s_.gp;g[2]=read(0x136d04u,g[4]+0x14u);g[0]=read(0x136d08u,g[4]+12u);g[1]=read(0x136d0cu,g[4]+0x28u);g[7]=read(0x136d10u,g[4]+0x1cu);
        g[2]=s_.flags.plus(g[2],0x20u,4);write(0x136d17u,g[4]+0x14u,g[2]);g[2]=signed_half(read(0x136d1bu,g[6]+0x166u,2));
        g[0]=s_.flags.increment(g[0],4,false);g[1]=s_.flags.plus(g[1],4u,4);g[7]=s_.flags.plus(g[7],0x10u,4);g[3]=s_.flags.plus(g[3],4u,4);
        s_.flags.minus(g[0],g[2],4);write(0x136d2eu,g[4]+12u,g[0]);write(0x136d32u,g[4]+0x28u,g[1]);write(0x136d36u,g[4]+0x1cu,g[7]);
        if(!less())return false;
        g[0]=read(0x136ac4u,g[4]+0x34u);g[6]=read(0x136ac8u,g[4]+0x20u);return true;
    }
    void finish(){auto&g=s_.gp;g[7]=pop(0x136d40u);g[6]=pop(0x136d41u);g[3]=pop(0x136d42u);g[4]=g[5];g[5]=pop(0x136d45u);pop(0x136d46u);g[4]+=4u;}
};
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
    if(!admitted(s))return false;
    View view{s,before};native_model_draw_submission_detail::submit_model(view);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_model_draw_submission_detail

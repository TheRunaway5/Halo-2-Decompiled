#pragma once
#include "halo2/native_tu15_visibility_association_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "physics/runtime/native_tu15_sse_mmx_scope.hpp"
#endif

namespace halo2::native_visibility_association_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_VISIBILITY_ASSOCIATION");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction){
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
template<class State>bool admitted(const State&s){
    if(reference()||!policy_admitted(s))return false;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return native_sse_mmx_detail::supported(s);
    return native_raw_x87_detail::supported(s);
#else
    return false;
#endif
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class Scope>struct Io {Scope*scope;template<class Callback>void operator()(Callback&&callback){scope->io(std::forward<Callback>(callback));}};
struct Offset {std::uint32_t value{},flags{};};
inline std::uint32_t flags(std::uint32_t ah,unsigned char overflow){return((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)|2u;}
inline Offset mask_zero_offset(std::uint32_t context,std::uint32_t index){
    auto carrier=index,offset=index*16u;unsigned char overflow;
    // Preserve physical shift-undefined flags from the original last ADD and
    // immediate shift, despite the intervening native provider callbacks.
    asm volatile("addl %%edx,%%ecx;leal 0xa6(%%eax),%%ecx;shll $4,%%ecx;lahf;seto %2"
        :"+c"(offset),"+a"(carrier),"=qm"(overflow):"d"(context):"cc","memory");
    return{offset,flags(carrier,overflow)};
}
inline std::uint32_t membership_flags(std::uint32_t context,std::uint32_t index,std::int32_t ordinal){
    auto carrier=index,offset=index*16u,word=std::uint32_t(ordinal);std::uint32_t bit;unsigned char overflow;
    // Thread the source's undefined flags through the same producers. LEAs,
    // moves and memory accesses between them have no arithmetic flag effect.
    asm volatile("addl %%edx,%%ecx;leal 0xa6(%%eax),%%ecx;shll $4,%%ecx;movl %%edi,%%ecx;sarl $5,%%edi;andl $31,%%ecx;movl $1,%%esi;shll %%cl,%%esi;lahf;seto %4"
        :"+a"(carrier),"+c"(offset),"+D"(word),"=&S"(bit),"=qm"(overflow):"d"(context):"cc","memory");
    return flags(carrier,overflow);
}
template<class State,class Before,class Scope>class View:public native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;using Base::io;
    std::uint32_t context_{};
    void count_store(std::uint32_t pc,std::uint32_t value){
        at(pc);const auto address=s_.gp[2]+0xa5eu;SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(2);io([&]{s_.memory.read(address,event.before);});}
        const std::array bytes{std::byte(value),std::byte(value>>8u)};
        io([&]{s_.memory.write(address,bytes);});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
    }
public:
    View(State&s,Before&before,Scope&scope):Base{s,before,Io<Scope>{&scope}},context_(s.gp[2]){}
    std::uint32_t reserve_index(){
        auto&g=s_.gp;g[1]=s_.flags.logic(0u,4);at(0x136e12u);g[1]=word(g[2]+0xa5eu,2);g[0]=g[1];
        push(0x136e1cu,g[6]);g[6]=0;g[1]+=1u;s_.flags.minus(g[0],0u,4);count_store(0x136e22u,g[1]);return g[0];
    }
    bool valid_index(std::uint32_t index){s_.flags.minus(index,512u,4);return index<512u;}
    void saturate(){count_store(0x136e84u,512u);s_.gp[0]=s_.flags.logic(0u,4);}
    std::uint32_t unavailable(){s_.gp[6]=pop(0x136e8fu);pop(0x136e90u);s_.gp[4]+=4u;return s_.gp[0];}
    void clear_mask(std::uint32_t index){
        auto&g=s_.gp;g[1]=s_.flags.plus(index*16u,g[2],4);
        Base::store(0x136e39u,g[1]+0xa6cu,0u,[]{});
        Base::store(0x136e3fu,g[1]+0xa68u,0u,[]{});
        Base::store(0x136e45u,g[1]+0xa64u,0u,[]{});
        const auto offset=mask_zero_offset(context_,index);g[1]=offset.value;s_.flags.load(offset.flags);
        Base::store(0x136e54u,g[1]+g[2],0u,[]{});
    }
    std::int32_t ordinal(){at(0x136e57u);s_.gp[1]=std::uint32_t(std::int32_t(std::int16_t(word(s_.gp[4]+8u,2))));return std::int32_t(s_.gp[1]);}
    void mark(std::uint32_t index,std::int32_t ordinal,BitLocation location){
        auto&g=s_.gp;push(0x136e5cu,g[7]);g[7]=std::uint32_t(location.word);
        g[6]=0x298u+g[7]+index*4u;g[2]+=g[6]*4u;g[1]=std::uint32_t(ordinal)&31u;g[6]=location.mask;
        s_.flags.load(membership_flags(context_,index,ordinal));
        at(0x136e79u);g[1]=word(g[2]);g[7]=pop(0x136e7bu);
        g[1]=s_.flags.logic(g[1]|g[6],4);Base::store(0x136e7eu,g[2],g[1],[]{});
    }
    std::uint32_t completed(std::uint32_t){s_.gp[6]=pop(0x136e80u);pop(0x136e81u);s_.gp[4]+=4u;return s_.gp[0];}
};
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
    if(!admitted(s))return false;
    const auto execute=[&](auto&scope){View view{s,before,scope};reserve(view);};
    if constexpr(requires{s.active_mmx;})if(s.active_mmx){native_sse_mmx_detail::Scope scope{s};execute(scope);return true;}
    native_raw_x87_detail::Scope scope{s};execute(scope);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
template<class State>void retained_mask_zero_offset(State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(policy_admitted(s)){
        const auto offset=mask_zero_offset(s.gp[2],s.gp[0]);s.gp[1]=offset.value;s.flags.load(offset.flags);return;
    }
#endif
    s.gp[1]=s.flags.shift(s.gp[1],4u,4);
}
template<class State>void retained_membership_bit(State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(policy_admitted(s)){
        const auto ordinal=s.gp[7]*32u+s.gp[1];
        const auto context=s.gp[2]-(0x298u+s.gp[7]+s.gp[0]*4u)*4u;
        const auto captured=membership_flags(context,s.gp[0],std::int32_t(ordinal));
        s.gp[6]<<=(s.gp[1]&31u);s.flags.load(captured);return;
    }
#endif
    s.gp[6]=s.flags.shift(s.gp[6],s.reg(1,1,0),4);
}
} // namespace halo2::native_visibility_association_detail

#pragma once
#include "halo2/native_tu15_cluster_frustum_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "rendering/native_tu15_sphere_frustum_abi.hpp"
#endif

namespace halo2::native_cluster_frustum_abi_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_CLUSTER_FRUSTUM");return p&&*p;}();return value;}
template<class State>bool admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction){
        const auto*c=s.instruction_cursor;
        if(!s.literal_service_sites||!c||c!=&s.services.instruction_cursor||!c->position||!c->callback_type||
           s.services.instruction.target_type()!=*c->callback_type||c->executor!=std::this_thread::get_id()||
           !c->literal_service_sites||c->service_sites.size()!=3||c->service_sites[0]!=0x38846eu||
           c->service_sites[1]!=0x21f590u||c->service_sites[2]!=0x33b595u||
           (c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
    }else if(s.instruction_cursor||s.literal_service_sites||s.observe)return false;
    return !s.observe||(s.instruction_cursor&&s.instruction_cursor->observation_only_stores);
#else
    (void)s;return false;
#endif
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// IMUL's escaped undefined flags are obtained from the actual operand form,
// seeded from source flags in the same assembly block as the multiply.
template<unsigned Factor,class State>std::uint32_t multiply(State&s,std::uint32_t value){
    auto status=((s.flags.bits()&0xd5u)<<8u)|((s.flags.bits()>>11u)&1u);unsigned overflow;
    asm volatile("addb $127,%%al;sahf;imull %3,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(status),"=c"(overflow),"+r"(value):"i"(Factor):"cc");
    s.flags.load((status&0xd5u)|((overflow&1u)<<11u));return value;
}
template<class State,class Call,class Before>class View {
    State&s_;Call&call_;Before&before_;native_raw_x87_detail::Scope<State>&scope_;
    void at(std::uint32_t pc){if(s_.instruction_cursor&&s_.instruction_cursor->position)*s_.instruction_cursor->position=pc;scope_.io([&]{before_(s_,pc);});}
    std::uint32_t word(std::uint32_t pc,std::uint32_t address,unsigned width=4){
        at(pc);std::array<std::byte,4>bytes{};scope_.io([&]{s_.memory.read(address,std::span(bytes).first(width));});
        std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
    }
    template<class Commit>void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width,Commit commit){
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},std::vector<std::byte>(width)};
        if(s_.observe){event.before.resize(width);scope_.io([&]{s_.memory.read(address,event.before);});}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(8u*n));
        scope_.io([&]{s_.memory.write(address,event.written);});commit();
        if(s_.observe)scope_.io([&]{s_.observe(event);});
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4){write(pc,address,value,width,[]{});}
    void push(std::uint32_t pc,std::uint32_t value){const auto next=s_.gp[4]-4u;write(pc,next,value,4,[&]{s_.gp[4]=next;});}
    std::uint32_t pop(std::uint32_t pc){const auto result=word(pc,s_.gp[4]);s_.gp[4]+=4u;return result;}
    static std::int32_t signed_half(std::uint32_t value){return std::int16_t(value);}
public:
    View(State&s,Call&call,Before&before,native_raw_x87_detail::Scope<State>&scope):s_(s),call_(call),before_(before),scope_(scope){}
    std::int32_t begin(){
        auto&g=s_.gp;g[0]=multiply<26>(s_,g[0]);push(0x167ef3u,g[3]);push(0x167ef4u,g[5]);
        g[5]=word(0x167ef5u,g[4]+0x10u);s_.set(3,s_.flags.logic(0,1),1,0);store(0x167efbu,g[5],0,1);
        const auto count=word(0x167efeu,g[7],2);s_.flags.minus(count,0,2);g[0]+=g[7]+0xa6eu;
        store(0x167f09u,g[4]+0x10u,0);return signed_half(count);
    }
    void open_groups(){s_.gp[2]=s_.gp[0]+2u;push(0x167f16u,s_.gp[6]);}
    native_cluster_frustum_detail::Range range(){
        s_.gp[6]=s_.flags.logic(0,4);const auto count=word(0x167f19u,s_.gp[2],2);s_.flags.minus(count,0,2);return{0,signed_half(count)};
    }
    bool contained(){const auto value=word(0x167f20u,s_.gp[5],1);s_.flags.minus(value,0,1);return value!=0;}
    unsigned classify(const native_cluster_frustum_detail::Range&){
        auto&g=s_.gp;g[0]=std::uint32_t(signed_half(word(0x167f26u,g[2]+12u,2)));g[1]=word(0x167f2au,g[4]+0x10u);
        g[0]=s_.flags.plus(g[0],g[6],4);g[0]=multiply<264>(s_,g[0]);g[0]+=g[7]+0x1974u;s_.xmm[1]=s_.xmm[7];
        push(0x167f40u,0x167f45u);scope_.child([&]{call_(s_,before_);});s_.flags.logic(g[0],4);return g[0];
    }
    void accept(bool contained){
        auto&g=s_.gp;s_.flags.minus(g[0],2,4);s_.set(1,contained,1,0);s_.set(3,1,1,0);store(0x167f51u,g[5],g[1]&255u,1);
    }
    native_cluster_frustum_detail::Range next(native_cluster_frustum_detail::Range){
        auto&g=s_.gp;g[0]=std::uint32_t(signed_half(word(0x167f54u,g[2],2)));g[6]=s_.flags.increment(g[6],4,false);s_.flags.minus(g[6],g[0],4);
        return{std::bit_cast<std::int32_t>(g[6]),std::bit_cast<std::int32_t>(g[0])};
    }
    bool next_group(){
        auto&g=s_.gp;g[0]=word(0x167f5cu,g[4]+0x14u);g[1]=std::uint32_t(signed_half(word(0x167f60u,g[7],2)));
        g[0]=s_.flags.increment(g[0],4,false);g[2]=s_.flags.plus(g[2],2,4);s_.flags.minus(g[0],g[1],4);
        store(0x167f69u,g[4]+0x14u,g[0]);return std::bit_cast<std::int32_t>(g[0])<std::bit_cast<std::int32_t>(g[1]);
    }
    void close_groups(){s_.gp[6]=pop(0x167f6fu);}
    unsigned finish(){s_.gp[5]=pop(0x167f70u);s_.set(0,s_.gp[3]&255u,1,0);s_.gp[3]=pop(0x167f73u);pop(0x167f74u);s_.gp[4]+=8u;return s_.gp[0];}
};
template<class State,class Call,class Before=Noop>bool bridge_with(State&s,Call call,Before before={}){
    if(!admitted(s))return false;native_raw_x87_detail::Scope scope{s};View view{s,call,before,scope};native_cluster_frustum_detail::aggregate(view);return true;
}
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
    return bridge_with(s,[](auto&child,auto&observe){if(!native_sphere_frustum_abi_detail::bridge(child,observe))throw std::runtime_error("Qualified sphere child refused after cluster admission");},std::move(before));
}
#else
template<class State,class...Args>bool bridge_with(State&,Args&&...){return false;}
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_cluster_frustum_abi_detail

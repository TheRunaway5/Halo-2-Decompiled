#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_facet_ring_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_facet_ring_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Io>class Schedule {
    State&s_;Io&io_;
public:
    Schedule(State&s,Io&io):s_(s),io_(io){}
    bool begin(){
        auto&g=s_.gp;
        g[4]=s_.flags.minus(g[4],12u,4u);io_.push(0x314cb3u,g[6]);
        io_.load(0x314cb4u,6u,g[4]+28u);io_.store(0x314cb8u,g[4]+12u,g[1]);
        io_.load(0x314cbcu,1u,g[4]+20u);io_.load(0x314cc0u,2u,g[1]+4u);io_.push(0x314cc3u,g[7]);
        g[7]=s_.flags.logic(0u,4u);g[0]=s_.flags.logic(0u,4u);s_.flags.logic(g[2],4u);
        io_.store(0x314ccau,g[4]+12u,g[7]);
        return !s_.flags.zf&&s_.flags.sf==s_.flags.of;
    }
    void save_loop_state(){auto&g=s_.gp;io_.push(0x314cd4u,g[3]);io_.push(0x314cd5u,g[5]);}
    bool have_next_node(){
        s_.flags.logic(s_.gp[6],4u);
        if(!s_.flags.zf)return true;
        s_.gp[7]=2u;return false;
    }
    void acquire_facet_and_successor(){
        auto&g=s_.gp;
        io_.load(0x314cdau,7u,g[1]);io_.load(0x314cdcu,1u,g[7]+g[0]*4u);
        g[3]=g[0]+1u;s_.flags.minus(g[3],g[2],4u);io_.store(0x314ce4u,g[4]+16u,g[3]);
        if(s_.flags.sf==s_.flags.of)io_.load(0x314cf0u,3u,g[7]);
        else io_.load(0x314ceau,3u,g[7]+4u+g[0]*4u);
    }
    void link_facet(){
        auto&g=s_.gp;
        io_.store(0x314cf2u,g[1]+32u,g[1]);io_.store(0x314cf5u,g[1]+48u,g[1]);io_.store(0x314cf8u,g[1]+64u,g[1]);
        g[0]=g[1]+52u;io_.store(0x314cfeu,g[1]+40u,g[0]);
        g[2]=g[1]+20u;io_.store(0x314d04u,g[1]+56u,g[2]);
        g[7]=g[1]+36u;io_.store(0x314d0au,g[1]+24u,g[7]);
        io_.load(0x314d0du,5u,g[6]+4u);io_.load(0x314d10u,5u,g[5]);io_.store(0x314d13u,g[2],g[5]);
        io_.load(0x314d15u,5u,g[6]);io_.store(0x314d17u,g[7],g[5]);
        io_.load(0x314d19u,7u,g[4]+44u);io_.load(0x314d1du,7u,g[7]);io_.store(0x314d1fu,g[0],g[7]);
        io_.store(0x314d21u,g[6]+8u,g[2]);io_.store(0x314d24u,g[1]+28u,g[6]);
        io_.store(0x314d27u,g[3]+44u,g[0]);g[3]=s_.flags.plus(g[3],36u,4u);io_.store(0x314d2du,g[1]+60u,g[3]);
    }
    void build_plane(){io_.call(0x314d30u,0x314d35u,0x314a30u);}
    bool advance(){
        auto&g=s_.gp;
        io_.load(0x314d35u,0u,g[6]+4u);io_.load(0x314d38u,0u,g[0]);io_.load(0x314d3au,6u,g[0]+16u);
        io_.load(0x314d3du,1u,g[4]+32u);io_.store(0x314d41u,g[0]+16u,0u);
        io_.load(0x314d48u,0u,g[4]+16u);io_.load(0x314d4cu,2u,g[1]+4u);
        s_.flags.minus(g[0],g[2],4u);
        if(s_.flags.sf!=s_.flags.of)return true;
        io_.load(0x314d53u,7u,g[4]+20u);return false;
    }
    void restore_loop_state(){io_.pop(0x314d5eu,5u);io_.pop(0x314d5fu,3u);}
    void clear_tables_and_finish(){
        auto&g=s_.gp;
        io_.load(0x314d60u,1u,g[4]+16u);io_.call(0x314d64u,0x314d69u,0x314870u);
        io_.compare_active_node();g[0]=s_.flags.zf?g[7]:2u;
        io_.pop(0x314d76u,7u);io_.pop(0x314d77u,6u);g[4]=s_.flags.plus(g[4],12u,4u);io_.ret(0x314d7bu);
    }
};
template<class State,class Before,class Call>class PhysicalIo:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>&fp_;
    Call&call_;
public:
    using Base::push;
    PhysicalIo(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp,Call&call)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);}
    void pop(std::uint32_t pc,unsigned reg){s_.gp[reg]=Base::pop(pc);}
    void ret(std::uint32_t pc){Base::pop(pc);s_.gp[4]+=16u;}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t bits){Base::store(pc,address,bits,[]{});}
    void call(std::uint32_t site,std::uint32_t returned,std::uint32_t target){
        push(site,returned);fp_.child([&]{call_(s_,target,site);});
    }
    void compare_active_node(){at(0x314d69u);const auto value=word(s_.gp[4]+32u);s_.flags.minus(value,s_.gp[6],4u);}
};
template<class State,class Before,class Call>void run_private_owner(State&s,Call call,Before before){
    if(!native_raw_x87_detail::supported(s))throw std::runtime_error("Private facet ring requires dense masked FP transport");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before,Call>io{s,before,fp,call};Schedule<State,decltype(io)>view{s,io};
    native_tu15_solver_hull_facet_ring_kernel(view);
}
} // namespace halo2::native_solver_hull_facet_ring_detail
#endif

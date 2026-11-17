#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_facet_visit_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_facet_visit_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void begin() {
        auto& g=s_.gp;
        g[4]=s_.flags.minus(g[4],8u,4u);
        io_.load(0x3148e3u,0u,g[4]+20u);
        io_.push(0x3148e7u,g[3]);io_.push(0x3148e8u,g[5]);
        g[3]=s_.flags.logic(0u,4u);io_.push(0x3148ebu,g[6]);
        g[0]=s_.flags.plus(g[0],28u,4u);io_.push(0x3148efu,g[7]);g[7]=g[1];
        io_.store(0x3148f2u,g[4]+20u,4u,g[3]);io_.store(0x3148f6u,g[4]+16u,4u,g[0]);
    }
    bool load_neighbor() {
        auto& g=s_.gp;
        io_.load(0x314900u,1u,g[4]+16u);io_.load(0x314904u,5u,g[1]);
        io_.load(0x314906u,6u,g[5]+12u);io_.load_low(0x314909u,0u,g[6]+68u);
        s_.flags.logic(g[0]&0xffu,1u);return s_.flags.zf;
    }
    void clip_neighbor() {
        auto& g=s_.gp;
        io_.load(0x314910u,0u,g[4]+40u);g[2]=g[7]+4u;
        io_.push(0x314917u,g[2]);io_.push(0x314918u,g[7]);io_.push(0x314919u,g[0]);g[1]=g[6];
        io_.call(0x31491cu,0x314921u,0x3147d0u);
        s_.flags.minus(g[0]&0xffu,1u,1u);io_.store(0x314923u,g[6]+68u,1u,g[0]);
    }
    bool neighbor_intersects()const{return s_.flags.zf;}
    void visit_neighbor() {
        auto& g=s_.gp;
        io_.load(0x314928u,1u,g[4]+40u);io_.load(0x31492cu,2u,g[4]+32u);io_.load(0x314930u,0u,g[4]+28u);
        io_.push(0x314934u,g[1]);io_.push(0x314935u,g[6]);io_.push(0x314936u,g[2]);io_.push(0x314937u,g[0]);g[1]=g[7];
        io_.call(0x31493au,0x31493fu,0x3148e0u);g[3]=g[0];s_.flags.minus(g[3],2u,4u);
    }
    bool child_stops_walk()const{return s_.flags.zf;}
    bool neighbor_stops_walk(){s_.flags.minus(s_.gp[0]&0xffu,4u,1u);return s_.flags.zf;}
    void mark_stop(){s_.gp[3]=2u;}
    bool collect_neighbor(){io_.compare_status();return s_.flags.zf;}
    void link_neighbor() {
        auto& g=s_.gp;
        io_.load(0x314952u,1u,g[5]);io_.load(0x314955u,0u,g[4]+28u);io_.load(0x314959u,2u,g[4]+32u);
        io_.store(0x31495du,g[1]+16u,4u,g[5]);io_.increment(0x314960u,g[0]);
        io_.store(0x314962u,g[2],4u,g[5]);
    }
    bool advance() {
        auto& g=s_.gp;
        io_.load(0x314964u,0u,g[4]+20u);io_.load(0x314968u,2u,g[4]+16u);
        g[0]=s_.flags.increment(g[0],4u,false);g[2]=s_.flags.plus(g[2],16u,4u);s_.flags.minus(g[0],3u,4u);
        io_.store(0x314973u,g[4]+20u,4u,g[0]);io_.store(0x314977u,g[4]+16u,4u,g[2]);
        return s_.flags.sf!=s_.flags.of;
    }
    void finish() {
        auto& g=s_.gp;
        io_.load(0x314984u,0u,g[4]+36u);io_.store(0x314988u,g[0]+16u,4u,0x7f7fffffu);
        io_.load(0x31498fu,1u,g[7]+12u);io_.load(0x314992u,2u,g[7]+8u);
        io_.store(0x314995u,g[2]+g[1]*4u,4u,g[0]);io_.increment(0x314998u,g[7]+12u);
        io_.pop(0x31499bu,7u);io_.pop(0x31499cu,6u);io_.pop(0x31499du,5u);g[0]=g[3];
        io_.pop(0x3149a0u,3u);g[4]=s_.flags.plus(g[4],8u,4u);io_.ret(0x3149a4u);
    }
};

template<class State,class Before,class Call> class PhysicalIo:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>& fp_;
    Call& call_;
    std::uint32_t byte(std::uint32_t address) {
        std::uint32_t value{};
        fp_.io([&]{value=physics_step_detail::r(s_.memory,address,1u);});
        return value;
    }
public:
    using Base::push;
    PhysicalIo(State& s,Before& before,native_raw_x87_detail::Scope<State>& fp,Call& call)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);}
    void load_low(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);s_.set(reg,byte(address),1u,0u);
    }
    void pop(std::uint32_t pc,unsigned reg){s_.gp[reg]=Base::pop(pc);}
    void ret(std::uint32_t pc){Base::pop(pc);s_.gp[4]+=16u;}
    void store(std::uint32_t pc,std::uint32_t address,unsigned width,std::uint32_t value) {
        if(width==4u){Base::store(pc,address,value,[]{});return;}
        at(pc);fp_.io([&]{physics_step_detail::w(s_.memory,s_.observe,pc,address,1u,value);});
    }
    void call(std::uint32_t site,std::uint32_t returned,std::uint32_t target) {
        push(site,returned);fp_.child([&]{call_(s_,target,site);});
    }
    void compare_status() {
        at(0x31494cu);s_.flags.minus(byte(s_.gp[6]+68u),2u,1u);
    }
    void increment(std::uint32_t pc,std::uint32_t address) {
        at(pc);auto flags=s_.flags;const auto value=flags.increment(word(address),4u,false);
        Base::store(pc,address,value,[&]{s_.flags=flags;},true);
    }
};
template<class State,class Call,class Before> void run_private_owner(State& s,Call call,Before before) {
    if(!native_raw_x87_detail::supported(s)||s.fp.count>7u)
        throw std::runtime_error("Private hull facet visit requires dense masked FP with one free slot");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};native_tu15_solver_hull_facet_visit_kernel(view);
}
} // namespace halo2::native_solver_hull_facet_visit_detail
#endif

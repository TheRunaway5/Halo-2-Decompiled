#pragma once
#include "physics/runtime/native_tu15_solver_hull_facet_visit_source_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_facet_search_kernel.hpp"

namespace halo2::native_solver_hull_facet_search_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void begin() {
        auto& g=s_.gp;
        io_.push(0x314c40u,g[3]);io_.load(0x314c41u,3u,g[4]+12u);
        io_.push(0x314c45u,g[5]);io_.load(0x314c46u,5u,g[4]+12u);
        io_.push(0x314c4au,g[6]);g[6]=g[1];
        io_.push(0x314c4du,g[7]);io_.load(0x314c4eu,7u,g[4]+28u);
        io_.store(0x314c52u,g[3],4u,0u);g[0]=g[6]+4u;
        io_.push(0x314c5bu,g[0]);io_.store(0x314c5cu,g[5],4u,0u);
        io_.store(0x314c63u,g[0],4u,0x7f7fffffu);
    }
    void clip_seed_facet() {
        auto& g=s_.gp;
        io_.load(0x314c69u,0u,g[4]+36u);
        io_.push(0x314c6du,g[6]);io_.push(0x314c6eu,g[0]);g[1]=g[7];
        io_.store(0x314c71u,g[6],4u,0x3727c5acu);
        io_.call(0x314c77u,0x314c7cu,0x3147d0u);
    }
    bool mark_seed_status() {
        auto& g=s_.gp;
        s_.flags.minus(g[0]&0xffu,1u,1u);
        io_.store(0x314c7eu,g[7]+68u,1u,g[0]);
        return s_.flags.zf;
    }
    void collect_connected_facets() {
        auto& g=s_.gp;
        io_.load(0x314c83u,1u,g[4]+32u);
        io_.push(0x314c87u,g[1]);io_.push(0x314c88u,g[7]);io_.push(0x314c89u,g[3]);io_.push(0x314c8au,g[5]);g[1]=g[6];
        io_.call(0x314c8du,0x314c92u,0x3148e0u);
    }
    void finish(bool intersects) {
        auto& g=s_.gp;
        if(intersects) {
            io_.pop(0x314c92u,7u);io_.pop(0x314c93u,6u);io_.pop(0x314c94u,5u);
            io_.pop(0x314c95u,3u);io_.ret(0x314c96u);
        } else {
            io_.pop(0x314c99u,7u);io_.pop(0x314c9au,6u);io_.pop(0x314c9bu,5u);
            g[0]=2u;io_.pop(0x314ca1u,3u);io_.ret(0x314ca2u);
        }
    }
};
template<class State,class Call,class Before> void run_private_owner(State& s,Call call,Before before) {
    if(!native_raw_x87_detail::supported(s)||s.fp.count>7u)
        throw std::runtime_error("Private hull facet search requires dense masked FP with one free slot");
    native_raw_x87_detail::Scope fp{s};
    native_solver_hull_facet_visit_detail::PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};native_tu15_solver_hull_facet_search_kernel(view);
}
template<class State,class Call,class Checkpoint> void run_private_source(State& s,Call call,Checkpoint checkpoint) {
    native_solver_hull_facet_visit_detail::SourceIo<State,Call,Checkpoint> io{s,call,checkpoint};
    Schedule<State,decltype(io)> view{s,io};native_tu15_solver_hull_facet_search_kernel(view);
}
} // namespace halo2::native_solver_hull_facet_search_detail
#endif

#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_fallback_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp"

namespace halo2::native_simplex_fallback_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    void load(std::uint32_t pc,unsigned dest,std::uint32_t address){s_.gp[dest]=io_.word(pc,address);}
    bool signed_at_most()const{return s_.flags.zf||s_.flags.sf!=s_.flags.of;}
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    bool permits_hull_fallback(){
        auto& g=s_.gp;g[0]=(g[0]&0xffffff00u)|io_.byte(0x30f581u,g[6]+12u);
        s_.flags.logic(g[0]&0xffu,1u);return s_.flags.zf;
    }
    void call_hull_fallback(){
        auto& g=s_.gp;
        load(0x30f588u,0u,g[5]+20u);load(0x30f58bu,1u,g[5]+16u);load(0x30f58eu,2u,g[5]+12u);
        io_.push(0x30f591u,g[0]);load(0x30f592u,0u,g[5]+8u);
        io_.push(0x30f595u,g[1]);io_.push(0x30f596u,g[2]);io_.push(0x30f597u,g[0]);
        g[1]=g[6];io_.call(0x30f59au,0x30f59fu,0x30de60u);
        io_.store(0x30f59fu,g[4]+32u,g[0]);
    }
    void restrict_overlap(){
        auto& g=s_.gp;
        load(0x30f5a5u,0u,g[7]);load(0x30f5a7u,1u,g[6]);g[2]=g[0]+g[1];
        s_.flags.minus(g[2],4u,4u);
        if(!signed_at_most()){
            s_.flags.minus(g[1],g[0],4u);
            if(signed_at_most()){io_.store(0x30f5c3u,g[6],1u);io_.store(0x30f5c9u,g[7],3u);}
            else {io_.store(0x30f5b5u,g[6],3u);io_.store(0x30f5bbu,g[7],1u);}
        }
        io_.store(0x30f5cfu,g[4]+32u,3u);
    }
};
template<class State,class Call,class Before> void run_private_fallback(State& s,Call call,Before before){
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>4u)
        throw std::runtime_error("Private simplex fallback requires supported actual TU1.5 with four free x87 slots");
    native_raw_x87_detail::Scope fp{s};native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};native_tu15_simplex_fallback_kernel(view);
}
} // namespace halo2::native_simplex_fallback_detail
#endif

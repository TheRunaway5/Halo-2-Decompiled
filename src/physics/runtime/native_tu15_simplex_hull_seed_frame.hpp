#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_hull_seed_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp"

namespace halo2::native_simplex_hull_seed_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    void acquire(std::uint32_t pc,unsigned reg,std::uint32_t address){s_.gp[reg]=io_.word(pc,address);}
    void context_vector(std::uint32_t pc,unsigned reg,std::uint32_t offset){io_.vector(pc,reg,s_.gp[6]+offset);}
    void store_vector(std::uint32_t pc,unsigned reg,std::uint32_t offset){io_.store_vector(pc,s_.gp[4]+offset,s_.xmm[reg]);}
    void difference(unsigned target,unsigned first,unsigned second) {
        auto& x=s_.xmm;x[target]=x[first];
        asm volatile("subps %1,%0":"+x"(x[target]):"x"(x[second]):"memory");
    }
    bool signed_less() const{return s_.flags.sf!=s_.flags.of;}
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void open_frame() {
        auto& g=s_.gp;
        io_.push(0x30de60u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4u);g[4]=s_.flags.minus(g[4],0x134u,4u);
        io_.push(0x30de6cu,g[3]);io_.push(0x30de6du,g[6]);g[6]=g[1];
    }
    bool seed_first_pair() {
        auto& g=s_.gp;
        context_vector(0x30de70u,2u,0x20u);context_vector(0x30de74u,0u,0xa0u);
        acquire(0x30de7bu,0u,g[6]+4u);io_.push(0x30de7eu,g[7]);g[7]=g[6]+4u;
        difference(1u,2u,0u);g[2]=3u;s_.flags.minus(g[0],g[2],4u);
        store_vector(0x30de8fu,1u,0xd0u);context_vector(0x30de97u,1u,0xe0u);
        io_.store(0x30de9eu,g[4]+0xa0u,4u);store_vector(0x30dea9u,2u,0x20u);store_vector(0x30deaeu,1u,0x60u);
        return !signed_less();
    }
    void seed_second_shape_rows() {
        context_vector(0x30deb9u,3u,0xb0u);difference(4u,2u,3u);context_vector(0x30dec6u,3u,0xf0u);
        store_vector(0x30decdu,4u,0xe0u);store_vector(0x30ded5u,3u,0x70u);
        context_vector(0x30dedau,3u,0xc0u);difference(4u,2u,3u);context_vector(0x30dee7u,3u,0x100u);
        store_vector(0x30deeeu,2u,0x30u);store_vector(0x30def3u,4u,0xf0u);
        store_vector(0x30defbu,2u,0x40u);store_vector(0x30df00u,3u,0x80u);
    }
    bool second_shape_has_four() const{return !s_.flags.zf&&!signed_less();}
    void seed_fourth_from_second() {
        context_vector(0x30df0au,0u,0xd0u);difference(1u,2u,0u);context_vector(0x30df17u,0u,0x110u);
        store_vector(0x30df1eu,1u,0x100u);store_vector(0x30df26u,2u,0x50u);store_vector(0x30df2bu,0u,0x90u);
    }
    bool first_shape_has_two() {
        s_.flags.minus(io_.word(0x30df38u,s_.gp[6]),1u,4u);return !s_.flags.zf&&!signed_less();
    }
    bool first_shape_has_three() {
        auto& g=s_.gp;acquire(0x30df4fu,1u,g[6]);s_.flags.minus(g[1],g[2],4u);return !signed_less();
    }
    void seed_first_shape_rows() {
        context_vector(0x30df59u,3u,0x30u);difference(4u,3u,0u);
        store_vector(0x30df63u,4u,0xe0u);store_vector(0x30df6bu,3u,0x30u);
        context_vector(0x30df70u,3u,0x40u);difference(4u,3u,0u);
        store_vector(0x30df7au,1u,0x70u);store_vector(0x30df7fu,4u,0xf0u);
        store_vector(0x30df87u,3u,0x40u);store_vector(0x30df8cu,1u,0x80u);
    }
    bool first_shape_has_four() const{return !s_.flags.zf&&!signed_less();}
    void seed_fourth_from_first() {context_vector(0x30df96u,2u,0x50u);store_fourth_from_first();}
    void seed_fourth_from_first_pair() {context_vector(0x30df3du,2u,0x30u);store_fourth_from_first();}
    void store_fourth_from_first() {
        difference(3u,2u,0u);store_vector(0x30dfa0u,3u,0x100u);
        store_vector(0x30dfa8u,2u,0x50u);store_vector(0x30dfadu,1u,0x90u);
    }
    bool second_shape_has_two() {
        s_.flags.minus(s_.gp[0],1u,4u);return !s_.flags.zf&&!signed_less();
    }
    void seed_fourth_from_second_pair() {
        context_vector(0x30dfbcu,0u,0xb0u);difference(1u,2u,0u);context_vector(0x30dfc9u,0u,0xf0u);
        store_vector(0x30dfd0u,1u,0x100u);store_vector(0x30dfd8u,2u,0x50u);store_vector(0x30dfddu,0u,0x90u);
    }
    void keep_three(){io_.store(0x30df43u,s_.gp[4]+0xa0u,s_.gp[2]);}
    void keep_one(){io_.store(0x30dfe7u,s_.gp[4]+0xa0u,1u);}
    void call_hull() {
        auto& g=s_.gp;
        acquire(0x30dff2u,3u,g[5]+0x10u);g[0]=g[4]+0x110u;io_.push(0x30dffcu,g[0]);
        acquire(0x30dffdu,0u,g[5]+0xcu);g[1]=g[4]+0xd4u;io_.push(0x30e007u,g[1]);
        acquire(0x30e008u,1u,g[5]+8u);g[2]=g[4]+0x28u;io_.push(0x30e00fu,g[2]);
        io_.push(0x30e010u,g[3]);io_.push(0x30e011u,g[0]);io_.push(0x30e012u,g[1]);
        io_.call(0x30e013u,0x30e018u,0x314710u);
    }
};

template<class State,class Call,class Before> void run_private_hull_seed(State& s,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>4u)
        throw std::runtime_error("Private simplex hull seed requires actual TU1.5 with four free masked x87 slots");
    native_raw_x87_detail::Scope fp{s};native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};native_tu15_simplex_hull_seed_kernel(view);
}
} // namespace halo2::native_simplex_hull_seed_detail
#endif

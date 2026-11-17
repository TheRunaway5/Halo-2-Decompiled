#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_seed_facets_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_seed_facets_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    bool begin_facets() {
        auto& g=s_.gp;
        io_.push(0x314d80u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4u);
        io_.push(0x314d86u,g[1]);io_.push(0x314d87u,g[3]);io_.push(0x314d88u,g[6]);g[6]=g[1];
        io_.load(0x314d8bu,1u,g[6]+0x1b0u);io_.store(0x314d91u,g[6]+0x1b4u,4u,4u);
        io_.load(0x314d9bu,0u,g[6]+0x1b4u);
        g[0]=s_.flags.shift(g[0]+g[0]*4u,4u,4u);g[3]=g[1];
        g[0]=s_.flags.plus(g[0],g[3],4u);s_.flags.minus(g[1],g[0],4u);
        io_.push(0x314dadu,g[7]);g[2]=0x41d7b0u;
        return !s_.flags.zf;
    }
    bool begin_links() {
        auto& g=s_.gp;
        g[0]=g[1]+36u;io_.store(0x314db8u,g[1]+24u,4u,g[0]);g[0]=g[1]+52u;g[7]=g[1]+68u;
        io_.store(0x314dc1u,g[1]+40u,4u,g[0]);g[0]=g[1]+20u;s_.flags.minus(g[0],g[7],4u);
        io_.store(0x314dc9u,g[7],1u,0u);io_.store(0x314dccu,g[1]+56u,4u,g[0]);
        return s_.flags.cf;
    }
    void link_edge() {
        auto& g=s_.gp;
        io_.store(0x314dd1u,g[0]+12u,4u,g[1]);io_.load(0x314dd4u,7u,g[2]);
        io_.add_word(0x314dd6u,7u,g[6]+0x2100u);io_.store(0x314ddcu,g[0],4u,g[7]);
        io_.load(0x314ddeu,3u,g[2]+4u);io_.load(0x314de1u,7u,g[2]+8u);
        g[7]=s_.flags.plus(g[7],g[3],4u);io_.load(0x314de6u,3u,g[6]+0x1b0u);
        g[7]+=g[3]+20u;io_.store(0x314df0u,g[0]+8u,4u,g[7]);
        g[0]=s_.flags.plus(g[0],16u,4u);g[7]=g[1]+68u;g[2]=s_.flags.plus(g[2],12u,4u);
        s_.flags.minus(g[0],g[7],4u);
    }
    bool more_links()const{return s_.flags.cf;}
    bool next_facet() {
        auto& g=s_.gp;
        io_.load(0x314e00u,0u,g[6]+0x1b4u);io_.load(0x314e06u,3u,g[6]+0x1b0u);
        g[0]=s_.flags.shift(g[0]+g[0]*4u,4u,4u);
        g[1]=s_.flags.plus(g[1],80u,4u);g[0]=s_.flags.plus(g[0],g[3],4u);
        s_.flags.minus(g[1],g[0],4u);return !s_.flags.zf;
    }
    void build_first_plane() {
        auto& g=s_.gp;
        io_.load(0x314e1bu,1u,g[6]+0x1b0u);g[1]=s_.flags.plus(g[1],0xf0u,4u);
        io_.call(0x314e27u,0x314e2cu,0x314a30u);
    }
    bool requires_flip() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.load(0x314e2cu,0u,g[6]+0x2100u);io_.load_vector(0x314e32u,2u,g[0]);
        io_.load_vector(0x314e35u,0u,g[0]+32u);io_.load(0x314e39u,1u,g[6]+0x1b0u);
        io_.load_vector(0x314e3fu,1u,g[1]+0xf0u);g[2]=g[0]+32u;
        asm volatile("subps %1,%0":"+x"(x[2]):"x"(x[0]):"memory");
        x[0]=x[2];asm volatile("mulps %1,%0":"+x"(x[0]):"x"(x[1]):"memory");
        x[1]=_mm_shuffle_ps(x[0],x[0],85);x[2]=x[0];g[0]=g[4]+12u;
        asm volatile("addss %1,%0":"+x"(x[1]):"x"(x[0]):"memory");
        x[2]=_mm_shuffle_ps(x[2],x[0],170);
        asm volatile("addss %1,%0":"+x"(x[2]):"x"(x[1]):"memory");
        std::uint32_t projection;std::memcpy(&projection,&x[2],4u);
        io_.store(0x314e6cu,g[0],4u,projection);io_.load_float(0x314e70u,g[4]+12u);
        io_.compare_float(0x314e74u,0x46a0e8u);
        g[0]=(g[0]&0xffff0000u)|io_.status();s_.flags.logic((g[0]>>8u)&0x41u,1u);
        return s_.flags.zf;
    }
    void flip_winding() {
        auto& g=s_.gp;
        io_.store(0x314e81u,g[1]+0xb4u,4u,g[2]);io_.store(0x314e87u,g[1]+0x64u,4u,g[2]);
        io_.store(0x314e8au,g[1]+0x14u,4u,g[2]);io_.load(0x314e8du,0u,g[6]+0x2100u);
        io_.store(0x314e93u,g[1]+0x104u,4u,g[0]);io_.store(0x314e99u,g[1]+0x74u,4u,g[0]);
        io_.store(0x314e9cu,g[1]+0x34u,4u,g[0]);
    }
    void begin_rebuild(){auto& g=s_.gp;io_.load(0x314e9fu,6u,g[6]+0x1b0u);g[7]=4u;}
    void rebuild_plane(){auto& g=s_.gp;g[1]=g[6];io_.call(0x314eb2u,0x314eb7u,0x314a30u);}
    bool advance_rebuild() {
        auto& g=s_.gp;g[6]=s_.flags.plus(g[6],80u,4u);g[7]=s_.flags.increment(g[7],4u,true);
        return !s_.flags.zf;
    }
    void finish() {
        auto& g=s_.gp;
        io_.pop(0x314ebdu,7u);io_.pop(0x314ebeu,6u);io_.pop(0x314ebfu,3u);
        g[4]=g[5];io_.pop(0x314ec2u,5u);io_.ret(0x314ec3u);
    }
};

template<class State,class Before,class Call> class PhysicalIo:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>& fp_;
    Call& call_;
public:
    using Base::push;
    PhysicalIo(State& s,Before& before,native_raw_x87_detail::Scope<State>& fp,Call& call)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);}
    void add_word(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);const auto bits=word(address);s_.gp[reg]=s_.flags.plus(s_.gp[reg],bits,4u);
    }
    void pop(std::uint32_t pc,unsigned reg){s_.gp[reg]=Base::pop(pc);}
    void ret(std::uint32_t pc){Base::pop(pc);}
    void store(std::uint32_t pc,std::uint32_t address,unsigned width,std::uint32_t value) {
        if(width==4u){Base::store(pc,address,value,[]{});return;}
        at(pc);fp_.io([&]{physics_step_detail::w(s_.memory,s_.observe,pc,address,1u,value);});
    }
    void load_vector(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);if(address&15u)throw std::runtime_error("Unaligned original hull seed-facets MOVAPS load");
        std::array<std::byte,16> bytes{};
        fp_.io([&]{s_.memory.read_vector(address,bytes);});std::memcpy(&s_.xmm[reg],bytes.data(),16u);
    }
    void call(std::uint32_t site,std::uint32_t returned,std::uint32_t target) {
        push(site,returned);fp_.child([&]{call_(s_,target,site);});
    }
    void load_float(std::uint32_t pc,std::uint32_t address) {
        at(pc);const auto bits=word(address);fp_.require_capacity(1u);
        asm volatile("flds %0"::"m"(bits):"memory","st");fp_.publish();
    }
    void compare_float(std::uint32_t pc,std::uint32_t address) {
        at(pc);const auto bits=word(address);
        asm volatile("fcomps %0"::"m"(bits):"memory","st");fp_.publish();
    }
    std::uint16_t status() {
        std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;
    }
};
template<class State,class Call,class Before> void run_private_owner(State& s,Call call,Before before) {
    if(!native_raw_x87_detail::supported(s)||s.fp.count>7u)
        throw std::runtime_error("Private hull seed facets require dense masked FP with one free slot");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};native_tu15_solver_hull_seed_facets_kernel(view);
}
} // namespace halo2::native_solver_hull_seed_facets_detail
#endif

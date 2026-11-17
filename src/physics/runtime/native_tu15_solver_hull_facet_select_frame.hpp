#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_facet_select_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_facet_select_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    void status_test(std::uint32_t mask) {
        auto& g=s_.gp;g[0]=(g[0]&0xffff0000u)|io_.status();s_.flags.logic((g[0]>>8u)&mask,1u);
    }
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void begin() {
        auto& g=s_.gp;
        io_.push(0x314b20u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4u);
        g[4]=s_.flags.minus(g[4],20u,4u);
        io_.push(0x314b29u,g[3]);io_.push(0x314b2au,g[6]);io_.push(0x314b2bu,g[7]);
        io_.call(0x314b2cu,0x314b31u,0x314870u);
        io_.load(0x314b31u,6u,g[5]+8u);g[3]=s_.flags.logic(0u,4u);
        io_.store(0x314b36u,g[4]+24u,1u,g[3]);
    }
    bool continue_walk() {
        auto& g=s_.gp;io_.load_low(0x314b40u,0u,g[4]+24u);s_.flags.logic(g[0]&0xffu,1u);
        return s_.flags.zf;
    }
    void begin_edges() {
        auto& g=s_.gp;io_.store(0x314b4cu,g[6]+68u,1u,3u);
        g[7]=s_.flags.logic(0u,4u);g[1]=g[6]+20u;
    }
    bool current_edge_is_distinct() {
        auto& g=s_.gp;s_.flags.minus(g[1],g[3],4u);return !s_.flags.zf;
    }
    bool edge_projection_eligible() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.load(0x314b5du,0u,g[1]);io_.load_vector(0x314b5fu,2u,g[0]);
        io_.load(0x314b62u,0u,g[1]+4u);io_.load(0x314b65u,2u,g[0]);
        io_.load_vector(0x314b67u,1u,g[6]);io_.load_vector(0x314b6au,0u,g[2]);
        asm volatile("subps %1,%0":"+x"(x[0]):"x"(x[2]):"memory");
        x[3]=_mm_shuffle_ps(x[1],x[1],201);x[4]=_mm_shuffle_ps(x[0],x[0],210);
        asm volatile("mulps %1,%0":"+x"(x[4]):"x"(x[3]):"memory");
        x[3]=_mm_shuffle_ps(x[1],x[1],210);x[1]=_mm_shuffle_ps(x[0],x[0],201);
        asm volatile("mulps %1,%0":"+x"(x[1]):"x"(x[3]):"memory");
        x[0]=x[1];asm volatile("subps %1,%0":"+x"(x[0]):"x"(x[4]):"memory");
        asm volatile("mulps %1,%0":"+x"(x[0]):"x"(x[2]):"memory");
        x[1]=_mm_shuffle_ps(x[0],x[0],85);x[2]=x[0];g[0]=g[4]+28u;
        asm volatile("addss %1,%0":"+x"(x[1]):"x"(x[0]):"memory");
        x[2]=_mm_shuffle_ps(x[2],x[0],170);
        asm volatile("addss %1,%0":"+x"(x[2]):"x"(x[1]):"memory");
        std::uint32_t bits;std::memcpy(&bits,&x[2],4u);
        io_.store(0x314bb5u,g[0],4u,bits);io_.load_float(0x314bb9u,g[4]+28u);
        io_.compare_float(0x314bbdu,0x46a30cu);status_test(5u);return !s_.flags.pf;
    }
    bool prefer_neighbor() {
        auto& g=s_.gp;
        io_.load(0x314bcau,0u,g[5]+8u);io_.load_float(0x314bcdu,g[0]+16u);
        io_.load(0x314bd0u,2u,g[1]+8u);io_.compare_float(0x314bd3u,0x46a0e8u);
        io_.load(0x314bd9u,2u,g[2]+12u);status_test(0x41u);
        if(!s_.flags.zf)return true;
        io_.load(0x314be3u,0u,g[5]+8u);io_.load_float(0x314be6u,g[0]+16u);
        io_.scale_float(0x314be9u,0x46a194u);io_.compare_float(0x314befu,g[2]+16u);
        status_test(0x41u);return !s_.flags.zf;
    }
    void adopt_neighbor() {
        auto& g=s_.gp;
        io_.load_low(0x314c08u,0u,g[2]+68u);g[1]=g[7];io_.shift_index();
        io_.load(0x314c10u,3u,g[1]+g[6]+28u);
        s_.flags.minus(g[0]&0xffu,3u,1u);g[6]=g[2];
        io_.store(0x314c18u,g[4]+24u,1u,s_.flags.zf);
    }
    void advance_edge() {
        auto& g=s_.gp;
        g[7]=s_.flags.increment(g[7],4u,false);g[1]=s_.flags.plus(g[1],16u,4u);
    }
    bool edge_remaining(){s_.flags.minus(s_.gp[7],3u,4u);return !s_.flags.zf;}
    void finish() {
        auto& g=s_.gp;io_.pop(0x314c26u,7u);g[0]=g[6];io_.pop(0x314c29u,6u);io_.pop(0x314c2au,3u);
        g[4]=g[5];io_.pop(0x314c2du,5u);io_.ret(0x314c2eu);
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
    void load_low(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);std::uint32_t value{};fp_.io([&]{value=physics_step_detail::r(s_.memory,address,1u);});
        s_.set(reg,value,1u,0u);
    }
    void load_vector(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);if(address&15u)throw std::runtime_error("Unaligned original hull facet select MOVAPS load");
        std::array<std::byte,16> bytes{};
        fp_.io([&]{s_.memory.read_vector(address,bytes);});std::memcpy(&s_.xmm[reg],bytes.data(),16u);
    }
    void pop(std::uint32_t pc,unsigned reg){s_.gp[reg]=Base::pop(pc);}
    void ret(std::uint32_t pc){Base::pop(pc);s_.gp[4]+=4u;}
    void store(std::uint32_t pc,std::uint32_t address,unsigned width,std::uint32_t value) {
        if(width==4u){Base::store(pc,address,value,[]{});return;}
        at(pc);fp_.io([&]{physics_step_detail::w(s_.memory,s_.observe,pc,address,1u,value);});
    }
    void call(std::uint32_t site,std::uint32_t returned,std::uint32_t target) {
        push(site,returned);fp_.child([&]{call_(s_,target,site);});
    }
    void load_float(std::uint32_t pc,std::uint32_t address) {
        at(pc);const auto bits=word(address);fp_.require_capacity(1u);
        asm volatile("flds %0"::"m"(bits):"memory","st");fp_.publish();
    }
    void compare_float(std::uint32_t pc,std::uint32_t address) {
        at(pc);const auto bits=word(address);asm volatile("fcomps %0"::"m"(bits):"memory","st");fp_.publish();
    }
    void scale_float(std::uint32_t pc,std::uint32_t address) {
        at(pc);const auto bits=word(address);asm volatile("fmuls %0"::"m"(bits):"memory","st");fp_.publish();
    }
    std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
    void shift_index() {
        const auto context=s_.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);
        auto value=s_.gp[1];unsigned overflow{};
        asm volatile("addb $127,%%al;sahf;shll $4,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+&a"(flags),"=c"(overflow),"+&r"(value)::"cc");
        s_.gp[1]=value;s_.flags.load((flags&0xd5u)|((overflow&1u)<<11u));
    }
};
template<class State,class Call,class Before> void run_private_owner(State& s,Call call,Before before) {
    if(!native_raw_x87_detail::supported(s)||s.fp.count>7u)
        throw std::runtime_error("Private hull facet select requires dense masked FP with one free slot");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};native_tu15_solver_hull_facet_select_kernel(view);
}
} // namespace halo2::native_solver_hull_facet_select_detail
#endif

#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_seed_route_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_seed_route_detail {
using Exit=NativeTu15SolverHullSeedExit;
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    static __m128 sub(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static std::uint32_t bits(__m128 a){std::uint32_t value;std::memcpy(&value,&a,4u);return value;}
    static __m128 y(__m128 a){return _mm_shuffle_ps(a,a,85);}
    static __m128 z(__m128 a){return _mm_shuffle_ps(a,a,170);}
    static __m128 yzx(__m128 a){return _mm_shuffle_ps(a,a,201);}
    static __m128 zxy(__m128 a){return _mm_shuffle_ps(a,a,210);}
    void status_test(std::uint32_t mask){
        auto& g=s_.gp;g[0]=(g[0]&0xffff0000u)|io_.status();s_.flags.logic((g[0]>>8u)&mask,1u);
    }
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){s_.gp[reg]=io_.word(pc,address);}
    Exit demote(std::uint32_t load_pc,std::uint32_t store_pc) {
        auto& g=s_.gp;load(load_pc,0u,g[6]+0x48u);io_.decrement(store_pc,g[0]+0x80u);return Exit::demote;
    }
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    Exit classify_four() {
        auto& g=s_.gp;auto& x=s_.xmm;
        load(0x313bfcu,0u,g[6]+0x50u);
        io_.vector(0x313bffu,1u,g[0]+16u);io_.vector(0x313c03u,0u,g[0]);io_.vector(0x313c06u,2u,g[0]+32u);
        x[0]=sub(x[0],x[1]);io_.vector(0x313c0du,1u,g[0]+16u);x[1]=sub(x[1],x[2]);
        x[2]=yzx(x[0]);x[3]=zxy(x[1]);x[0]=zxy(x[0]);x[1]=yzx(x[1]);
        x[0]=mul(x[0],x[1]);x[2]=mul(x[2],x[3]);x[2]=sub(x[2],x[0]);
        io_.store_vector(0x313c33u,g[4]+96u,x[2]);
        io_.vector(0x313c38u,1u,g[0]);io_.vector(0x313c3bu,0u,g[0]+48u);
        x[0]=mul(sub(x[0],x[1]),x[2]);x[1]=add(y(x[0]),x[0]);x[0]=z(x[0]);g[1]=g[4]+88u;
        x[0]=add(x[0],x[1]);io_.store(0x313c5cu,g[1],bits(x[0]));
        io_.load_float(0x313c60u,g[4]+88u);io_.absolute();io_.compare_float(0x313c66u,0x46a180u);status_test(0x41u);
        if(s_.flags.pf)return Exit::build_tetrahedron;
        s_.flags.minus(g[7],3u,4u);if(s_.flags.sf==s_.flags.of)return Exit::write_degenerate_result;
        return demote(0x313c80u,0x313c83u);
    }
    Exit classify_three() {
        auto& g=s_.gp;auto& x=s_.xmm;
        load(0x313c89u,0u,g[6]+0x50u);io_.load_float(0x313c8cu,0x46a0e8u);
        io_.vector(0x313c92u,1u,g[0]+16u);io_.vector(0x313c96u,0u,g[0]);io_.vector(0x313c99u,2u,g[0]+32u);
        x[0]=sub(x[0],x[1]);x[1]=sub(x[1],x[2]);x[2]=yzx(x[1]);x[3]=zxy(x[0]);
        x[3]=mul(x[3],x[2]);x[2]=zxy(x[1]);x[1]=yzx(x[0]);
        x[0]=mul(x[1],x[2]);x[0]=sub(x[0],x[3]);io_.store_vector(0x313ccbu,g[4]+96u,x[0]);
        x[0]=mul(x[0],x[0]);x[1]=add(y(x[0]),x[0]);x[2]=z(x[0]);g[2]=g[4]+92u;x[2]=add(x[2],x[1]);
        io_.store(0x313cedu,g[2],bits(x[2]));io_.load_float(0x313cf1u,g[4]+92u);
        io_.compare_pop_pair();status_test(0x44u);
        if(s_.flags.pf)return Exit::reduce_triangle;
        s_.flags.minus(g[7],2u,4u);if(s_.flags.sf==s_.flags.of)return Exit::write_simplex_result;
        return demote(0x313d0bu,0x313d0eu);
    }
    Exit classify_two() {
        auto& g=s_.gp;auto& x=s_.xmm;
        load(0x313d14u,0u,g[6]+0x50u);io_.vector(0x313d17u,1u,g[0]+16u);io_.vector(0x313d1bu,0u,g[0]);
        x[0]=sub(x[0],x[1]);x[0]=mul(x[0],x[0]);x[1]=y(x[0]);x[2]=x[0];g[0]=g[4]+84u;
        x[1]=add(x[1],x[0]);x[2]=add(z(x[2]),x[1]);io_.store(0x313d3eu,g[0],bits(x[2]));
        io_.load_float(0x313d42u,g[4]+84u);io_.compare_float(0x313d46u,0x46a454u);status_test(0x41u);
        if(s_.flags.pf)return Exit::improve_basis;
        s_.flags.minus(g[7],2u,4u);if(s_.flags.sf==s_.flags.of)return Exit::write_degenerate_result;
        return demote(0x313d60u,0x313d63u);
    }
    Exit classify_one() {
        auto& g=s_.gp;auto& x=s_.xmm;
        load(0x313d69u,0u,g[6]+0x50u);io_.vector(0x313d6cu,0u,g[0]);x[0]=mul(x[0],x[0]);
        x[1]=add(y(x[0]),x[0]);x[2]=z(x[0]);g[1]=g[4]+16u;x[2]=add(x[2],x[1]);
        io_.store(0x313d8cu,g[1],bits(x[2]));io_.load_float(0x313d90u,g[4]+16u);
        io_.compare_float(0x313d94u,0x46a454u);status_test(0x41u);
        if(s_.flags.pf)return Exit::append_line;
        s_.flags.logic(g[7],4u);if(s_.flags.sf==s_.flags.of)return Exit::write_degenerate_result;
        return demote(0x313da9u,0x313dacu);
    }
};

template<class State,class Before> class PhysicalIo:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;
    native_raw_x87_detail::Scope<State>& fp_;
    void vector_half(std::uint32_t pc,std::uint32_t address,std::span<const std::byte> bits) {
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(8u);fp_.io([&]{s_.memory.read(address,event.before);});}
        fp_.io([&]{s_.memory.write(address,bits);});
        if(s_.observe){event.written.assign(bits.begin(),bits.end());fp_.io([&]{s_.observe(event);});}
    }
public:
    PhysicalIo(State& s,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp){}
    std::uint32_t word(std::uint32_t pc,std::uint32_t address){at(pc);return Base::word(address);}
    void vector(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);if(address&15u)throw std::runtime_error("Unaligned original hull seed route MOVAPS load");
        std::array<std::byte,16> bytes{};
        fp_.io([&]{s_.memory.read_vector(address,bytes);});std::memcpy(&s_.xmm[reg],bytes.data(),16u);
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t bits){Base::store(pc,address,bits,[]{});}
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value) {
        at(pc);if(address&15u)throw std::runtime_error("Unaligned original hull seed route MOVAPS store");
        std::array<std::byte,16> bits;std::memcpy(bits.data(),&value,16u);
        const std::span<const std::byte,16> bytes(bits);
        vector_half(pc,address,bytes.first<8>());vector_half(pc,address+8u,bytes.last<8>());
    }
    void decrement(std::uint32_t pc,std::uint32_t address) {
        at(pc);auto flags=s_.flags;const auto value=flags.increment(Base::word(address),4u,true);
        Base::store(pc,address,value,[&]{s_.flags=flags;},true);
    }
    void load_float(std::uint32_t pc,std::uint32_t address) {
        const auto bits=word(pc,address);fp_.require_capacity(1u);
        asm volatile("flds %0"::"m"(bits):"memory","st");fp_.publish();
    }
    void compare_float(std::uint32_t pc,std::uint32_t address) {
        const auto bits=word(pc,address);asm volatile("fcomps %0"::"m"(bits):"memory","st");fp_.publish();
    }
    void compare_pop_pair(){asm volatile("fcompp":::"memory","st","st(1)");fp_.publish();}
    void absolute(){asm volatile("fabs":::"memory","st");fp_.publish();}
    std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
};
template<class State,class Before> Exit run_private_route(State& s,NativeTu15SolverHullInitialPath path,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>6u)
        throw std::runtime_error("Private hull seed route requires two free dense masked x87 slots");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before> io{s,before,fp};
    Schedule<State,decltype(io)> view{s,io};return native_tu15_solver_hull_seed_route_kernel(view,path);
}
} // namespace halo2::native_solver_hull_seed_route_detail
#endif

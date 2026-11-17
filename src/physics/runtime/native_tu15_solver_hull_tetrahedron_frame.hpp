#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_tetrahedron_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_tetrahedron_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void open_exception_frame() {
        auto& g=s_.gp;
        io_.push(0x313ab0u,0xffffffffu);io_.push(0x313ab2u,0x37f1f7u);
        io_.load(0x313ab7u,0u,s_.fs_base);io_.push(0x313abdu,g[0]);io_.store(0x313abeu,s_.fs_base,4u,g[4]);
        io_.push(0x313ac5u,g[1]);io_.push(0x313ac6u,g[6]);g[6]=g[1];
    }
    void initialize_inline_storage() {
        auto& g=s_.gp;
        g[0]=g[6]+20u;io_.store(0x313accu,g[6]+8u,4u,g[0]);g[0]=s_.flags.logic(0u,4u);g[1]=0x80000064u;
        io_.store(0x313ad6u,g[4]+4u,4u,g[6]);io_.store(0x313adau,g[6]+12u,4u,g[0]);
        io_.store(0x313addu,g[6]+16u,4u,g[1]);g[2]=g[6]+0x1c0u;
        io_.store(0x313ae6u,g[4]+16u,4u,g[0]);io_.store(0x313aeau,g[6]+0x1b0u,4u,g[2]);
        io_.store(0x313af0u,g[6]+0x1b4u,4u,g[0]);io_.store(0x313af6u,g[6]+0x1b8u,4u,g[1]);
        g[1]=g[6]+0x2110u;io_.store(0x313b02u,g[6]+0x2100u,4u,g[1]);
        io_.store(0x313b08u,g[6]+0x2104u,4u,g[0]);io_.store(0x313b0eu,g[6]+0x2108u,4u,0x80000037u);
    }
    void copy_seed_points() {
        auto& g=s_.gp;
        io_.load(0x313b18u,2u,g[6]+0x2100u);io_.load(0x313b1eu,1u,g[4]+24u);
        io_.store(0x313b22u,g[6]+0x2104u,4u,4u);
        io_.load_vector(0x313b2cu,0u,g[1]);io_.store_vector(0x313b2fu,g[2],s_.xmm[0]);
        io_.load(0x313b32u,2u,g[6]+0x2100u);io_.store(0x313b38u,g[2]+16u,4u,g[0]);
        io_.load(0x313b3bu,1u,g[6]+0x2100u);io_.load(0x313b41u,2u,g[4]+28u);
        io_.load_vector(0x313b45u,0u,g[2]);io_.store_vector(0x313b48u,g[1]+32u,s_.xmm[0]);
        io_.load(0x313b4cu,1u,g[6]+0x2100u);io_.store(0x313b52u,g[1]+48u,4u,g[0]);
        io_.load(0x313b55u,2u,g[6]+0x2100u);io_.load(0x313b5bu,1u,g[4]+32u);
        io_.load_vector(0x313b5fu,0u,g[1]);io_.store_vector(0x313b62u,g[2]+64u,s_.xmm[0]);
        io_.load(0x313b66u,2u,g[6]+0x2100u);io_.store(0x313b6cu,g[2]+80u,4u,g[0]);
        io_.load(0x313b6fu,1u,g[6]+0x2100u);io_.load(0x313b75u,2u,g[4]+36u);
        io_.load_vector(0x313b79u,0u,g[2]);io_.store_vector(0x313b7cu,g[1]+96u,s_.xmm[0]);
        io_.load(0x313b80u,1u,g[6]+0x2100u);io_.store(0x313b86u,g[1]+112u,4u,g[0]);
    }
    void build_facet_planes() {
        auto& g=s_.gp;
        g[1]=g[6];io_.store(0x313b8bu,g[4]+16u,1u,2u);
        io_.call(0x313b90u,0x313b95u,0x314d80u);
    }
    void finish() {
        auto& g=s_.gp;
        io_.load(0x313b95u,1u,g[4]+8u);g[0]=g[6];io_.pop(0x313b9bu,6u);
        io_.store(0x313b9cu,s_.fs_base,4u,g[1]);g[4]=s_.flags.plus(g[4],16u,4u);io_.ret(0x313ba6u);
    }
};

template<class State,class Before,class Call> class PhysicalIo:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>& fp_;
    Call& call_;
    void half(std::uint32_t pc,std::uint32_t address,std::span<const std::byte> bytes) {
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(8u);fp_.io([&]{s_.memory.read(address,event.before);});}
        fp_.io([&]{s_.memory.write(address,bytes);});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());fp_.io([&]{s_.observe(event);});}
    }
public:
    using Base::push;
    PhysicalIo(State& s,Before& before,native_raw_x87_detail::Scope<State>& fp,Call& call)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);}
    void pop(std::uint32_t pc,unsigned reg){s_.gp[reg]=Base::pop(pc);}
    void ret(std::uint32_t pc){Base::pop(pc);s_.gp[4]+=16u;}
    void store(std::uint32_t pc,std::uint32_t address,unsigned width,std::uint32_t value) {
        if(width==4u){Base::store(pc,address,value,[]{});return;}
        at(pc);fp_.io([&]{physics_step_detail::w(s_.memory,s_.observe,pc,address,1u,value);});
    }
    void load_vector(std::uint32_t pc,unsigned reg,std::uint32_t address) {
        at(pc);if(address&15u)throw std::runtime_error("Unaligned original hull tetrahedron MOVAPS load");
        std::array<std::byte,16> bytes{};
        fp_.io([&]{s_.memory.read_vector(address,bytes);});std::memcpy(&s_.xmm[reg],bytes.data(),16u);
    }
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value) {
        at(pc);if(address&15u)throw std::runtime_error("Unaligned original hull tetrahedron MOVAPS store");
        std::array<std::byte,16> bytes;std::memcpy(bytes.data(),&value,16u);
        const std::span<const std::byte,16> span(bytes);
        half(pc,address,span.first<8>());half(pc,address+8u,span.last<8>());
    }
    void call(std::uint32_t site,std::uint32_t returned,std::uint32_t target) {
        push(site,returned);fp_.child([&]{call_(s_,target,site);});
    }
};
template<class State,class Call,class Before> void run_private_owner(State& s,Call call,Before before) {
    if(!native_raw_x87_detail::supported(s)||s.fp.count>7u)
        throw std::runtime_error("Private hull tetrahedron requires dense masked FP with one free slot");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};native_tu15_solver_hull_tetrahedron_kernel(view);
}
} // namespace halo2::native_solver_hull_tetrahedron_detail
#endif

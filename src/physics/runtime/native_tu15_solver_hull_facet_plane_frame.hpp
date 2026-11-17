#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_facet_plane_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_facet_plane_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Io>class Schedule {
    State&s_;Io&io_;
    static __m128 sub(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 add_low(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 sub_low(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static __m128 mul_low(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    static std::uint32_t low(__m128 a){std::uint32_t bits;std::memcpy(&bits,&a,4u);return bits;}
public:
    Schedule(State&s,Io&io):s_(s),io_(io){}
    void capture_triangle(){
        auto&g=s_.gp;
        io_.push(0x314a30u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4u);g[4]=s_.flags.minus(g[4],32u,4u);
        io_.load(0x314a39u,0u,g[1]+0x24u);io_.load_vector(0x314a3cu,1u,g[0]);
        io_.load(0x314a3fu,0u,g[1]+0x14u);io_.load_vector(0x314a42u,0u,g[0]);
        io_.load(0x314a45u,2u,g[1]+0x34u);io_.load_vector(0x314a48u,2u,g[2]);
    }
    void cross_and_length(){
        auto&x=s_.xmm;auto&g=s_.gp;
        x[0]=sub(x[0],x[1]);x[1]=sub(x[1],x[2]);
        x[3]=_mm_shuffle_ps(x[0],x[0],210);x[2]=_mm_shuffle_ps(x[1],x[1],201);x[3]=mul(x[3],x[2]);
        x[2]=_mm_shuffle_ps(x[1],x[1],210);x[1]=_mm_shuffle_ps(x[0],x[0],201);
        x[1]=sub(mul(x[1],x[2]),x[3]);x[0]=mul(x[1],x[1]);
        x[2]=add_low(_mm_shuffle_ps(x[0],x[0],85),x[0]);x[3]=_mm_shuffle_ps(x[0],x[0],170);
        x[0]=add_low(x[3],x[2]);
        io_.store_vector(0x314a95u,g[4]+16u,x[0]);io_.estimate(0x314a9au);
    }
    void refine_scale(){
        auto&x=s_.xmm;auto&g=s_.gp;
        io_.store(0x314a9eu,g[4]+16u,low(x[2]));io_.load_vector(0x314aa4u,2u,g[4]+16u);
        x[0]=mul_low(x[0],x[2]);x[0]=mul_low(x[0],x[2]);
        io_.store(0x314ab1u,g[4]+12u,0x40400000u);io_.load_scalar(0x314ab9u,3u,g[4]+12u);
        x[3]=sub_low(x[3],x[0]);io_.store(0x314ac3u,g[4]+12u,0x3f000000u);io_.load_scalar(0x314acbu,0u,g[4]+12u);
        x[0]=mul_low(x[0],x[2]);x[0]=mul_low(x[0],x[3]);x[2]=_mm_shuffle_ps(x[0],x[0],0);
    }
    void publish_normal(){
        auto&x=s_.xmm;auto&g=s_.gp;
        io_.store_vector(0x314ae0u,g[1],x[1]);x[0]=mul(x[2],x[1]);io_.store_vector(0x314ae9u,g[1],x[0]);
    }
    void publish_plane_distance(){
        auto&x=s_.xmm;auto&g=s_.gp;
        io_.load_vector(0x314aecu,1u,g[0]);x[0]=mul(x[0],x[1]);
        x[1]=add_low(_mm_shuffle_ps(x[0],x[0],85),x[0]);x[2]=_mm_shuffle_ps(x[0],x[0],170);g[0]=g[4]+12u;
        x[2]=add_low(x[2],x[1]);io_.store(0x314b0cu,g[0],low(x[2]));
        io_.load(0x314b10u,2u,g[4]+12u);io_.store(0x314b14u,g[1]+16u,g[2]);
    }
    void finish(){s_.gp[4]=s_.gp[5];io_.pop(0x314b19u,5u);io_.ret(0x314b1au);}
};

template<class State,class Before>class PhysicalIo:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>&fp_;
    void vector_write(std::uint32_t pc,std::uint32_t address,std::span<const std::byte>payload){
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(payload.size());fp_.io([&]{s_.memory.read(address,event.before);});}
        fp_.io([&]{s_.memory.write(address,payload);});
        if(s_.observe){event.written.assign(payload.begin(),payload.end());fp_.io([&]{s_.observe(event);});}
    }
public:
    using Base::push;
    PhysicalIo(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);}
    void pop(std::uint32_t pc,unsigned reg){s_.gp[reg]=Base::pop(pc);}
    void ret(std::uint32_t pc){Base::pop(pc);}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t bits){Base::store(pc,address,bits,[]{});}
    void load_vector(std::uint32_t pc,unsigned reg,std::uint32_t address){
        at(pc);if(address&15u)throw std::runtime_error("Unaligned original facet-plane MOVAPS load");
        std::array<std::byte,16>data{};fp_.io([&]{s_.memory.read_vector(address,data);});
        std::memcpy(&s_.xmm[reg],data.data(),data.size());
    }
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);if(address&15u)throw std::runtime_error("Unaligned original facet-plane MOVAPS store");
        std::array<std::byte,16>data;std::memcpy(data.data(),&value,data.size());
        const std::span<const std::byte,16>bytes(data);
        vector_write(pc,address,bytes.first<8>());vector_write(pc,address+8u,bytes.last<8>());
    }
    void load_scalar(std::uint32_t pc,unsigned reg,std::uint32_t address){
        at(pc);s_.xmm[reg]=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(word(address))));
    }
    void estimate(std::uint32_t pc){
        at(pc);auto&x=s_.xmm;
        asm volatile("rsqrtss %1,%0":"+x"(x[2]):"x"(x[0]):"memory");
    }
};
template<class State,class Before=Noop>void run_private_owner(State&s,Before before={}){
    if(!native_raw_x87_detail::supported(s))throw std::runtime_error("Private facet plane requires dense masked FP transport");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before>io{s,before,fp};Schedule<State,decltype(io)>view{s,io};
    native_tu15_solver_hull_facet_plane_kernel(view);
}
} // namespace halo2::native_solver_hull_facet_plane_detail
#endif

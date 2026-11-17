#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_edge_insert_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_edge_insert_detail {
template<class State,class Io>class Schedule {
    State&s_;Io&io_;
public:
    Schedule(State&s,Io&io):s_(s),io_(io){}
    void append_seed_edge(){
        auto&g=s_.gp;
        g[4]=s_.flags.minus(g[4],0x1a0u,4u);
        io_.load(0x314ed6u,0u,g[1]+0x2104u);g[2]=g[0]+1u;
        io_.store(0x314edfu,g[1]+0x2104u,g[2]);
        io_.load(0x314ee5u,2u,g[4]+0x1acu);io_.load_vector(0x314eecu,0u,g[2]);
        io_.push(0x314eefu,g[6]);io_.load(0x314ef0u,6u,g[1]+0x2100u);
        g[0]=s_.flags.shift(g[0],5u,4u);g[0]=s_.flags.plus(g[0],g[6],4u);
        io_.load(0x314efbu,6u,g[4]+0x1a8u);g[2]=s_.flags.logic(0u,4u);
        io_.store(0x314f04u,g[0]+16u,g[2]);io_.store_vector(0x314f07u,g[0],s_.xmm[0]);
        io_.store(0x314f0au,g[4]+12u,g[2]);io_.store(0x314f0eu,g[4]+4u,g[0]);
    }
    void reserve_facet_slots(){
        auto&g=s_.gp;
        io_.push(0x314f12u,g[6]);g[2]=g[4]+12u;g[0]=g[4]+24u;
        io_.push(0x314f1bu,g[2]);io_.store(0x314f1cu,g[4]+16u,g[0]);
        io_.store(0x314f20u,g[4]+24u,0x80000064u);
        io_.call_local(0x314f28u,0x314f2du,0x3149b0u);
    }
    void link_facet_ring(){
        auto&g=s_.gp;
        io_.load(0x314f2du,2u,g[4]+0x1acu);g[0]=g[4]+4u;
        io_.push(0x314f38u,g[0]);io_.push(0x314f39u,g[2]);io_.push(0x314f3au,g[6]);
        g[0]=g[4]+20u;io_.push(0x314f3fu,g[0]);io_.call_local(0x314f40u,0x314f45u,0x314cb0u);
        g[6]=g[0];io_.load(0x314f47u,0u,g[4]+16u);s_.flags.logic(g[0],4u);
    }
    bool has_owned_dynamic_slots(){return !s_.flags.sf;}
    void release_dynamic_slots(){
        auto&g=s_.gp;
        io_.load(0x314f4fu,1u,0x48c938u);io_.load(0x314f55u,2u,g[1]);
        g[0]=s_.flags.logic(g[0]&0x7fffffffu,4u);io_.push(0x314f5cu,0x12u);
        io_.shift_release_size();io_.push(0x314f61u,g[0]);
        io_.load(0x314f62u,0u,g[4]+16u);io_.push(0x314f66u,g[0]);
        io_.call_release();
    }
    void finish(){
        auto&g=s_.gp;
        g[0]=g[6];io_.pop(0x314f6cu,6u);g[4]=s_.flags.plus(g[4],0x1a0u,4u);io_.ret(0x314f73u);
    }
};
template<class State,class Before,class Call>class PhysicalIo:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;
    native_raw_x87_detail::Scope<State>&fp_;
    Call&call_;
    void vector_half(std::uint32_t pc,std::uint32_t address,std::span<const std::byte>bytes){
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(8u);fp_.io([&]{s_.memory.read(address,event.before);});}
        fp_.io([&]{s_.memory.write(address,bytes);});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());fp_.io([&]{s_.observe(event);});}
    }
public:
    using Base::push;
    PhysicalIo(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp,Call&call)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);}
    void pop(std::uint32_t pc,unsigned reg){s_.gp[reg]=Base::pop(pc);}
    void ret(std::uint32_t pc){Base::pop(pc);s_.gp[4]+=12u;}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t bits){Base::store(pc,address,bits,[]{});}
    void load_vector(std::uint32_t pc,unsigned reg,std::uint32_t address){
        at(pc);if(address&15u)throw std::runtime_error("Unaligned original edge insertion MOVAPS load");
        std::array<std::byte,16>bytes{};fp_.io([&]{s_.memory.read_vector(address,bytes);});
        std::memcpy(&s_.xmm[reg],bytes.data(),16u);
    }
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);if(address&15u)throw std::runtime_error("Unaligned original edge insertion MOVAPS store");
        std::array<std::byte,16>data;std::memcpy(data.data(),&value,16u);
        const std::span<const std::byte,16>bytes(data);
        vector_half(pc,address,bytes.first<8>());vector_half(pc,address+8u,bytes.last<8>());
    }
    void call_local(std::uint32_t pc,std::uint32_t returned,std::uint32_t target){
        push(pc,returned);fp_.child([&]{call_(s_,target,pc);});
    }
    void shift_release_size(){
        const auto context=s_.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);
        auto value=s_.gp[0];unsigned overflow{};
        asm volatile("addb $127,%%al;sahf;shll $2,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+&a"(flags),"=c"(overflow),"+&r"(value)::"cc");
        s_.gp[0]=value;s_.flags.load((flags&0xd5u)|((overflow&1u)<<11u));
    }
    void call_release(){
        constexpr auto pc=0x314f67u;at(pc);const auto target=word(s_.gp[2]+20u),slot=s_.gp[4]-4u;
        Base::store(pc,slot,0x314f6au,[&]{s_.gp[4]=slot;},true);fp_.child([&]{call_(s_,target,pc);});
    }
};
template<class State,class Call,class Before>void run_private_owner(State&s,Call call,Before before){
    if(!native_raw_x87_detail::supported(s))throw std::runtime_error("Private edge insertion requires dense masked FP transport");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before,Call>io{s,before,fp,call};Schedule<State,decltype(io)>view{s,io};
    native_tu15_solver_hull_edge_insert_kernel(view);
}
} // namespace halo2::native_solver_hull_edge_insert_detail
#endif

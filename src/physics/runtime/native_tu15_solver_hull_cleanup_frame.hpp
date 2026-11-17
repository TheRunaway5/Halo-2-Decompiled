#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_cleanup_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

namespace halo2::native_solver_hull_cleanup_detail {
using Storage=NativeTu15SolverHullStorage;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};

// Private physical-publication adapter. The source FS base addresses the
// declared exception-chain bank; this code never changes the host FS selector.
// Pure provider failures before stack writes retain the pre-instruction SP.
template<class State,class Call,class Before>class Frame:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;
    Call&call_;
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){at(pc);s_.gp[reg]=word(address);}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4){
        at(pc);std::array<std::byte,4>bytes{};std::memcpy(bytes.data(),&value,4u);
        const auto payload=std::span(bytes).first(width);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);fp_.io([&]{s_.memory.read(address,event.before);});}
        fp_.io([&]{s_.memory.write(address,payload);});
        if(s_.observe){event.written.assign(payload.begin(),payload.end());fp_.io([&]{s_.observe(event);});}
    }
    template<unsigned Bits>void shift(){
        const auto context=s_.flags.bits();auto seed=((context&0xd5u)<<8u)|((context>>11u)&1u);
        auto value=s_.gp[0];unsigned overflow{};
        asm volatile("addb $127,%%al;sahf;shll %3,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+&a"(seed),"=c"(overflow),"+&r"(value):"i"(Bits):"cc");
        s_.gp[0]=value;s_.flags.load((seed&0xd5u)|((overflow&1u)<<11u));
    }
    void release(std::uint32_t pc,unsigned table_register){
        at(pc);const auto target=word(s_.gp[table_register]+0x14u);
        const auto address=s_.gp[4]-4u;
        Base::store(pc,address,pc+3u,[&]{s_.gp[4]=address;},true);
        fp_.child([&]{call_(s_,target,pc,false);});
    }
public:
    Frame(State&s,Call&call,Before&before,native_raw_x87_detail::Scope<State>&fp):
        Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    void open_frame(){
        auto&g=s_.gp;
        push(0x3139f0u,0xffffffffu);push(0x3139f2u,0x37f1b9u);
        load(0x3139f7u,0u,s_.fs_base);push(0x3139fdu,g[0]);store(0x3139feu,s_.fs_base,g[4]);
        push(0x313a05u,g[1]);push(0x313a06u,g[6]);g[6]=g[1];push(0x313a09u,g[7]);store(0x313a0au,g[4]+8u,g[6]);
    }
    bool owns_storage(Storage storage){
        auto&g=s_.gp;
        switch(storage){
        case Storage::stride_32:load(0x313a0eu,0u,g[6]+0x2108u);s_.flags.logic(g[0],4);store(0x313a16u,g[4]+0x14u,1u);break;
        case Storage::stride_80:load(0x313a3du,0u,g[6]+0x1b8u);s_.flags.logic(g[0],4);store(0x313a45u,g[4]+0x14u,0u,1u);break;
        case Storage::stride_4:load(0x313a6cu,0u,g[6]+0x10u);s_.flags.logic(g[0],4);store(0x313a71u,g[4]+0x14u,0xffffffffu);break;
        }
        return !s_.flags.sf;
    }
    void release_stride_32(){
        auto&g=s_.gp;load(0x313a20u,2u,g[6]+0x2100u);load(0x313a26u,1u,0x48c938u);load(0x313a2cu,7u,g[1]);
        g[0]=s_.flags.logic(g[0]&0x7fffffffu,4);push(0x313a33u,0x12u);shift<5>();
        push(0x313a38u,g[0]);push(0x313a39u,g[2]);release(0x313a3au,7u);
    }
    void release_stride_80(){
        auto&g=s_.gp;load(0x313a4cu,2u,g[6]+0x1b0u);load(0x313a52u,1u,0x48c938u);load(0x313a58u,7u,g[1]);
        g[0]=s_.flags.logic(g[0]&0x7fffffffu,4);g[0]+=g[0]*4u;push(0x313a62u,0x12u);shift<4>();
        push(0x313a67u,g[0]);push(0x313a68u,g[2]);release(0x313a69u,7u);
    }
    void release_stride_4(){
        auto&g=s_.gp;load(0x313a7bu,1u,0x48c938u);load(0x313a81u,2u,g[1]);
        g[0]=s_.flags.logic(g[0]&0x7fffffffu,4);push(0x313a88u,0x12u);shift<2>();push(0x313a8du,g[0]);
        load(0x313a8eu,0u,g[6]+8u);push(0x313a91u,g[0]);release(0x313a92u,2u);
    }
    void close_frame(){
        auto&g=s_.gp;load(0x313a95u,1u,g[4]+0xcu);g[7]=pop(0x313a99u);g[6]=pop(0x313a9au);
        store(0x313a9bu,s_.fs_base,g[1]);g[4]=s_.flags.plus(g[4],0x10u,4);pop(0x313aa5u);
    }
};
template<class State,class Call,class Before=Noop>void run_private_owner(State&s,Call call,Before before={}){
    if(!native_raw_x87_detail::supported(s))throw std::runtime_error("Private hull cleanup requires dense masked FP transport");
    native_raw_x87_detail::Scope fp{s};Frame<State,Call,Before>view{s,call,before,fp};
    native_tu15_solver_hull_cleanup_kernel(view);
}
} // namespace halo2::native_solver_hull_cleanup_detail
#endif

#pragma once
#include "halo2/native_tu15_particle_record_kernel.hpp"
#include "rendering/effects/native_tu15_particle_record_flags.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_particle_record_detail {
template<class State,class Before> class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;using Base::io;
    static __m128 scalar(std::uint32_t bits) {return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
    static std::uint32_t low(__m128 value) {return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {at(pc);return word(address,width);}
    __m128 load(std::uint32_t pc,std::uint32_t address) {return scalar(read(pc,address));}
    template<class Commit=void(*)()>
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4,bool positioned=false,
               Commit commit=[]{}) {
        if(!positioned)at(pc);
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4> bytes{};
        for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));
        io([&]{s_.memory.write(address,std::span(bytes).first(width));});commit();
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);io([&]{s_.observe(event);});}
    }
    // Each REP MOVS iteration is one original step that retires before its
    // store is observed; an empty REP still steps.
    void repeat(std::uint32_t pc,unsigned width) {
        auto& g=s_.gp;
        if(!g[1])at(pc);
        while(g[1]) {
            const auto delta=(s_.other_flags&1024u)?0u-width:width;
            store(pc,g[7],read(pc,g[6],width),width,true,[&]{g[6]+=delta;g[7]+=delta;--g[1];});
        }
    }
    void leave(std::uint32_t restore,std::uint32_t exit) {s_.gp[6]=pop(restore);pop(exit);s_.gp[4]+=8u;}
public:
    Frame(State& state,Before& observer,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,observer,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}} {}
    bool pool_full() {
        auto& g=s_.gp;
        push(0x1e4c0u,g[6]);
        g[6]=read(0x1e4c1u,0x4c2aa0u);
        s_.set(0,s_.flags.logic(s_.reg(0,1,0)^s_.reg(0,1,0),1),1,0);
        s_.flags.minus(g[6],0x400u,4);
        return s_.flags.sf==s_.flags.of;
    }
    bool overflow_pending() {
        s_.set(1,read(0x1e59cu,0x4c2ab1u,1),1,0);
        s_.flags.logic(s_.reg(1,1,0)&s_.reg(1,1,0),1);
        return !s_.flags.zf;
    }
    void clear_overflow() {store(0x1e5a6u,0x4c2ab1u,0u,1);leave(0x1e5adu,0x1e5aeu);}
    void return_full() {leave(0x1e598u,0x1e599u);}
    bool reserve() {
        auto& g=s_.gp;auto& x=s_.xmm;
        g[0]=multiply_slot(s_,g[6]);
        push(0x1e4dau,g[5]);
        g[5]=read(0x1e4dbu,0x4c2aa8u);
        store(0x1e4e1u,g[0]+g[5],0u);
        g[0]=s_.flags.plus(g[0],g[5],4);
        store(0x1e4eau,g[0]+0xcu,0u,1);
        x[0]=load(0x1e4eeu,g[2]);
        x[1]=load(0x1e4f2u,g[2]+4u);
        x[2]=load(0x1e4f7u,g[2]+8u);
        x[0]=subtract(x[0],load(0x1e4fcu,0x491e38u));
        x[1]=subtract(x[1],load(0x1e504u,0x491e3cu));
        x[2]=subtract(x[2],load(0x1e50cu,0x491e40u));
        g[6]=s_.flags.increment(g[6],4,false);
        s_.flags.logic(g[1]&g[1],4);
        store(0x1e517u,0x4c2aa0u,g[6]);
        return !s_.flags.zf;
    }
    void copy_payload() {
        auto& g=s_.gp;
        g[6]=read(0x1e51fu,g[4]+0x10u);
        g[5]=g[1];
        push(0x1e525u,g[7]);
        g[1]=shift_count(s_,g[1]);
        g[7]=g[0]+0x24u;
        repeat(0x1e52cu,4);
        g[1]=g[5];
        g[1]=s_.flags.logic(g[1]&3u,4);
        repeat(0x1e533u,1);
        g[7]=pop(0x1e535u);
    }
    void publish() {
        auto& g=s_.gp;auto& x=s_.xmm;
        g[1]=read(0x1e536u,g[4]+0xcu);
        store(0x1e53au,g[0],3u);
        store(0x1e540u,g[0]+8u,0u);
        store(0x1e547u,g[0]+0x20u,g[1]);
        x[3]=load(0x1e54au,0x491e4cu);x[3]=multiply(x[3],x[2]);
        x[2]=load(0x1e556u,0x491e48u);x[2]=multiply(x[2],x[1]);
        x[1]=load(0x1e562u,0x491e44u);x[1]=multiply(x[1],x[0]);
        x[0]=_mm_setzero_ps();
        x[3]=add(x[3],x[2]);
        x[3]=add(x[3],x[1]);
        x[0]=subtract(x[0],x[3]);
        store(0x1e57du,g[0]+4u,low(x[0]));
        g[1]=read(0x1e582u,g[2]);
        g[0]=s_.flags.plus(g[0],0x64u,4);
        store(0x1e587u,g[0],g[1]);
        g[1]=read(0x1e589u,g[2]+4u);
        store(0x1e58cu,g[0]+4u,g[1]);
        g[2]=read(0x1e58fu,g[2]+8u);
        store(0x1e592u,g[0]+8u,g[2]);
        s_.set(0,1u,1,0);
        g[5]=pop(0x1e597u);
        leave(0x1e598u,0x1e599u);
    }
};

template<class State,class Before> void run_private(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Frame<State,Before> frame{state,before,fp};append(frame);
}
} // namespace halo2::native_particle_record_detail
#endif

#pragma once
#include "halo2/native_tu15_flight_property_query_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include <bit>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_flight_property_query_detail {
template<class State,class Before> class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;using Base::push;
    native_raw_x87_detail::Scope<State>& fp_;
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {at(pc);return word(address,width);}
    __m128 scalar(std::uint32_t pc,std::uint32_t address) {return _mm_set_ss(std::bit_cast<float>(read(pc,address)));}
    void store_byte(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(1);Base::io([&]{s_.memory.read(address,event.before);});}
        const std::array data{std::byte(value)};
        Base::io([&]{s_.memory.write(address,data);});
        if(s_.observe){event.written.assign(data.begin(),data.end());Base::io([&]{s_.observe(event);});}
    }
    std::uint32_t finish(std::uint32_t restore,std::uint32_t ret) {
        s_.gp[6]=pop(restore);pop(ret);s_.gp[4]+=16u;return s_.gp[0];
    }
    std::uint32_t shift4(std::uint32_t value) {
        const auto context=s_.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);
        std::uint32_t overflow{};
        // Original E0931 is C1 E0 04; retain the physical status packet.
        asm volatile("addb $127,%%al;sahf;shll $4,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+a"(flags),"=c"(overflow),"+r"(value)::"cc");
        s_.flags.load((flags&0xd5u)|((overflow&1u)<<11u));return value;
    }
    void load_extended(std::uint32_t pc,std::uint32_t address) {
        const auto bits=read(pc,address);
        asm volatile("flds %0"::"m"(bits):"memory","st");fp_.publish();
    }
public:
    Frame(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp) {}
    Selection select() {
        auto& g=s_.gp;
        g[0]=read(0xe0900u,g[4]+4u);g[1]=read(0xe0904u,0x4ecbd0u);
        g[2]=read(0xe090au,g[1]+0x44u);s_.xmm[2]=_mm_xor_ps(s_.xmm[2],s_.xmm[2]);
        s_.xmm[1]=scalar(0xe0910u,0x46a0d0u);
        g[0]=s_.flags.logic(g[0]&0xffffu,4);g[0]+=g[0]*2u;
        g[1]=read(0xe0920u,g[2]+g[0]*4u+8u);g[0]=read(0xe0924u,g[1]);
        g[2]=read(0xe0926u,0x4f0414u);g[0]=s_.flags.logic(g[0]&0xffffu,4);
        g[0]=shift4(g[0]);g[2]=read(0xe0934u,g[2]+g[0]+8u);
        push(0xe0938u,g[6]);g[6]=read(0xe0939u,g[4]+12u);
        s_.set(0,s_.flags.logic(0u,1),1,0);
        s_.flags.minus(g[6],0x6000542u,4);s_.xmm[0]=s_.xmm[2];
        if(s_.flags.zf)return Selection::flight;
        s_.flags.minus(g[6],0x70000c9u,4);if(s_.flags.zf)return Selection::extra_flag;
        s_.flags.minus(g[6],0xc0005a4u,4);return s_.flags.zf?Selection::speed:Selection::unsupported;
    }
    std::uint32_t unsupported() {return finish(0xe0a05u,0xe0a06u);}
    void flight_value() {
        s_.flags.minus(read(0xe09c1u,s_.gp[1]+0x3dcu,1),2u,1);
        if(s_.flags.zf)s_.xmm[0]=s_.xmm[1];
    }
    bool flag_active() {
        s_.set(2,read(0xe09abu,s_.gp[1]+0x39du,1),1,0);
        s_.flags.logic(s_.reg(2,1,0),1);return !s_.flags.cf&&!s_.flags.zf;
    }
    void flag_value() {
        s_.xmm[0]=scalar(0xe09b5u,0x46a0d0u);s_.set(0,1u,1,0);s_.xmm[1]=s_.xmm[0];
    }
    bool speed_enabled() {
        s_.xmm[0]=scalar(0xe095eu,s_.gp[2]+0x2dcu);
        s_.flags.scalar_compare(s_.xmm[0],s_.xmm[2],false);return !s_.flags.cf&&!s_.flags.zf;
    }
    void speed_value() {
        fp_.require_capacity(5u);
        load_extended(0xe096fu,s_.gp[1]+0x90u);
        load_extended(0xe0975u,s_.gp[1]+0x8cu);
        load_extended(0xe097bu,s_.gp[1]+0x88u);
        // Exact uninterrupted E0981–E0997 squared length and sqrt, including
        // the original reduction order and x87 stack compaction.
        asm volatile(".byte 0xd9,0xc0,0xd8,0xc9,0xd9,0xc2,0xd8,0xcb,0xde,0xc1\n\t"
                     ".byte 0xd9,0xc3,0xd8,0xcc,0xde,0xc1,0xd9,0xfa,0xdd,0xdb,0xdd,0xd8,0xdd,0xd8"
                     :::"memory","st","st(1)","st(2)","st(3)","st(4)");
        fp_.publish();
        const auto limit=read(0xe0999u,s_.gp[2]+0x2dcu);
        asm volatile("fdivs %0"::"m"(limit):"memory","st");fp_.publish();
        at(0xe099fu);const auto pending=fp_.stage_single(true);
        Base::store(0xe099fu,s_.gp[4]+8u,pending.bits,[&]{fp_.commit(pending);},true);
        s_.xmm[0]=scalar(0xe09a3u,s_.gp[4]+8u);
    }
    void clamp() {
        s_.flags.scalar_compare(s_.xmm[2],s_.xmm[0],false);s_.set(0,1u,1,0);
        if(!s_.flags.cf&&!s_.flags.zf)s_.xmm[1]=s_.xmm[2];
        else{
            s_.flags.scalar_compare(s_.xmm[0],s_.xmm[1],false);
            if(s_.flags.cf||s_.flags.zf)s_.xmm[1]=s_.xmm[0];
        }
    }
    std::uint32_t publish() {
        auto& g=s_.gp;s_.flags.scalar_compare(s_.xmm[0],s_.xmm[2],false);
        g[1]=read(0xe09e4u,g[4]+16u);
        Base::store(0xe09e8u,g[1],std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[1])),[]{});
        if(s_.flags.cf||s_.flags.zf){
            g[2]=read(0xe09fdu,g[4]+20u);g[1]=s_.flags.logic(0u,4);
            store_byte(0xe0a03u,g[2],s_.reg(1,1,0));return finish(0xe0a05u,0xe0a06u);
        }
        g[2]=read(0xe09eeu,g[4]+20u);g[1]=1u;
        store_byte(0xe09f7u,g[2],s_.reg(1,1,0));return finish(0xe09f9u,0xe09fau);
    }
};

template<class State,class Before> std::uint32_t run_private(State& state,Before before) {
    native_raw_x87_detail::Scope fp{state};Frame view{state,before,fp};return query(view);
}
} // namespace halo2::native_flight_property_query_detail
#endif

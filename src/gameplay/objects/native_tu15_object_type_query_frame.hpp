#pragma once
#include "halo2/native_tu15_object_type_query_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_host_fp_io.hpp"
#include <bit>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_type_query_detail {
struct HostIo {
    template<class Function>void operator()(Function&&function)const {
        native_host_fp_io_detail::preserve(std::forward<Function>(function));
    }
};
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};

// Private physical ABI adapter. The typed controller owns inheritance traversal;
// this view publishes the original live operands and delegates genuine children.
// Retained-provider PUSH publication and source continuation are separate work.
template<class State,class Call,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,HostIo> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,HostIo>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
    Call&call_;
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {
        at(pc);return word(address,width);
    }
    void store_byte(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(1);Base::io([&]{s_.memory.read(address,event.before);});}
        const std::array data{std::byte(value)};
        Base::io([&]{s_.memory.write(address,data);});
        if(s_.observe){event.written.assign(data.begin(),data.end());Base::io([&]{s_.observe(event);});}
    }
    std::uint32_t finish(std::uint32_t first,std::uint32_t second,std::uint32_t ret) {
        auto&g=s_.gp;g[6]=pop(first);g[5]=pop(second);pop(ret);g[4]+=8u;return g[0];
    }
public:
    Frame(State&s,Call&call,Before&before):Base{s,before,HostIo{}},call_(call){}
    void begin() {
        auto&g=s_.gp;
        g[1]=read(0x10de40u,0x4ecbd0u);
        g[2]=read(0x10de46u,g[1]+0x44u);
        s_.xmm[0]=_mm_xor_ps(s_.xmm[0],s_.xmm[0]);
        Base::push(0x10de4cu,g[5]);
        g[5]=read(0x10de4du,g[4]+8u);
        g[0]=s_.flags.logic(g[5]&0xffffu,4);
        g[0]+=g[0]*2u;
        g[0]=read(0x10de5bu,g[2]+g[0]*4u+8u);
        g[1]=std::uint32_t(std::int32_t(std::int8_t(read(0x10de5fu,g[0]+0xaau,1))));
        g[1]=read(0x10de66u,0x474b90u+g[1]*4u);
        s_.set(0,s_.flags.logic(0u,1),1,0);
        Base::push(0x10de6fu,g[6]);
        Base::store(0x10de70u,g[3],std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[0])),[]{});
        store_byte(0x10de74u,g[7],s_.reg(0,1,0));
        g[6]=g[1]+0x84u;
    }
    bool method_present() {
        s_.gp[1]=read(0x10de80u,s_.gp[6]);
        s_.flags.logic(s_.gp[1],4);return !s_.flags.zf;
    }
    bool callback_present() {
        s_.gp[1]=read(0x10de86u,s_.gp[1]+0x4cu);
        s_.flags.logic(s_.gp[1],4);return !s_.flags.zf;
    }
    void invoke() {
        auto&g=s_.gp;
        g[2]=read(0x10de8du,g[4]+16u);
        Base::push(0x10de91u,g[7]);Base::push(0x10de92u,g[3]);
        Base::push(0x10de93u,g[2]);Base::push(0x10de94u,g[5]);
        at(0x10de95u);const auto target=g[1],address=g[4]-4u;
        Base::store(0x10de95u,address,0x10de97u,[&]{g[4]=address;},true);
        call_(s_,target,0x10de95u,false);
    }
    void advance(){s_.gp[6]=s_.flags.plus(s_.gp[6],4u,4);}
    bool accepted(){s_.flags.logic(s_.reg(0,1,0),1);return !s_.flags.zf;}
    std::uint32_t finish_found(){return finish(0x10de9eu,0x10de9fu,0x10dea0u);}
    std::uint32_t finish_empty() {
        s_.flags.logic(s_.reg(0,1,0),1);
        if(s_.flags.zf){
            s_.xmm[0]=_mm_xor_ps(s_.xmm[0],s_.xmm[0]);
            Base::store(0x10deaau,s_.gp[3],std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[0])),[]{});
            store_byte(0x10deaeu,s_.gp[7],s_.reg(0,1,0));
        }
        return finish(0x10deb0u,0x10deb1u,0x10deb2u);
    }
};
template<class State,class Call,class Before=Noop>
std::uint32_t run_private(State&s,Call call,Before before={}) {
    Frame view{s,call,before};return query(view);
}
} // namespace halo2::native_object_type_query_detail
#endif

#pragma once
#include "halo2/native_tu15_map_world_property_query_kernel.hpp"
#include "physics/runtime/native_tu15_host_fp_io.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include <bit>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_map_world_property_query_detail {
struct HostIo {
    template<class Function> void operator()(Function&& function) const {
        native_host_fp_io_detail::preserve(std::forward<Function>(function));
    }
};
template<class State,class Before> class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,HostIo> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,HostIo>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {
        at(pc);return word(address,width);
    }
    __m128 scalar(std::uint32_t pc,std::uint32_t address) {
        return _mm_set_ss(std::bit_cast<float>(read(pc,address)));
    }
    void store_byte(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        at(pc);
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(1);Base::io([&]{s_.memory.read(address,event.before);});}
        const std::array data{std::byte(value)};
        Base::io([&]{s_.memory.write(address,data);});
        if(s_.observe){event.written.assign(data.begin(),data.end());Base::io([&]{s_.observe(event);});}
    }
    std::uint32_t finish(std::uint32_t ret) {
        pop(ret);s_.gp[4]+=4u;return s_.gp[0];
    }
public:
    Frame(State& state,Before& before):Base{state,before,HostIo{}} {}
    Selection select() {
        auto& ecx=s_.gp[1];
        ecx=s_.flags.minus(ecx,0xa000693u,4);
        s_.xmm[1]=_mm_xor_ps(s_.xmm[1],s_.xmm[1]);s_.set(0,1u,1,0);
        if(s_.flags.zf)return Selection::flag;
        ecx=s_.flags.minus(ecx,0x2fffffeu,4);
        if(s_.flags.zf)return Selection::second_byte;
        ecx=s_.flags.increment(ecx,4,true);
        return s_.flags.zf?Selection::first_byte:Selection::unsupported;
    }
    std::uint32_t unsupported() {
        s_.set(0,s_.flags.logic(0u,1),1,0);return finish(0x1db81au);
    }
    void flag_value() {
        s_.gp[1]=read(0x1db841u,s_.gp[6]+4u);
        s_.flags.logic(s_.reg(1,1,8)&0x20u,1);
        s_.xmm[0]=scalar(0x1db847u,0x46a0d0u);
        if(s_.flags.zf)s_.xmm[0]=s_.xmm[1];
    }
    void byte_value(bool second) {
        s_.gp[1]=read(second?0x1db82fu:0x1db81du,s_.gp[6]+(second?0x1bu:0x1au),1);
        s_.xmm[0]=_mm_cvtsi32_ss(s_.xmm[0],int(s_.gp[1]));
        const auto scale=scalar(second?0x1db837u:0x1db825u,0x46a0c4u);
        asm volatile("mulss %1,%0":"+x"(s_.xmm[0]):"x"(scale));
    }
    std::uint32_t publish() {
        auto& gp=s_.gp;s_.flags.scalar_compare(s_.xmm[0],s_.xmm[1],false);
        Base::store(0x1db857u,gp[2],std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[0])),[]{});
        if(s_.flags.cf||s_.flags.zf){
            gp[2]=read(0x1db86bu,gp[4]+4u);gp[1]=s_.flags.logic(0u,4);
            store_byte(0x1db871u,gp[2],s_.reg(1,1,0));return finish(0x1db873u);
        }
        gp[2]=read(0x1db85du,gp[4]+4u);gp[1]=1u;
        store_byte(0x1db866u,gp[2],s_.reg(1,1,0));return finish(0x1db868u);
    }
};

template<class State,class Before> std::uint32_t run_private(State& state,Before before) {
    Frame view{state,before};return query(view);
}
} // namespace halo2::native_map_world_property_query_detail
#endif

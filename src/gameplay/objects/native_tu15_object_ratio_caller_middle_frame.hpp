#pragma once
#include "halo2/native_tu15_object_ratio_caller_middle_kernel.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_dispatch_frame.hpp"
#include "gameplay/objects/native_tu15_object_record_ratio_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_ratio_caller_detail {
template<class State,class Before> class MiddleFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::store;using Base::push;using Base::pop;using Base::io;
    Before& before_;
    native_raw_x87_detail::Scope<State>& fp_;

    std::uint32_t read(std::uint32_t pc,std::uint32_t address) {at(pc);return word(address);}
    bool above()const {return !s_.flags.zf&&s_.flags.sf==s_.flags.of;}
    void compare_key(std::uint32_t key) {s_.flags.minus(s_.gp[6],key,4);}
    void write_success(std::uint32_t pc,std::uint32_t address,std::byte value) {
        at(pc);
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(1);io([&]{s_.memory.read(address,event.before);});}
        io([&]{s_.memory.write(address,std::span(&value,1));});
        if(s_.observe){event.written.push_back(value);io([&]{s_.observe(event);});}
    }
    void restore_end(std::uint32_t ebx_pc,std::uint32_t ecx_pc,std::uint32_t ret_pc) {
        s_.gp[3]=pop(ebx_pc);s_.gp[1]=pop(ecx_pc);pop(ret_pc);s_.gp[4]+=16u;
    }
public:
    MiddleFrame(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},before_(before),fp_(fp) {}
    bool first_key() const {return s_.flags.zf;}
    void compare_upper_key() {compare_key(0x1400056du);}
    bool above_upper() const {return above();}
    bool equal_upper() const {return s_.flags.zf;}
    bool matches_record_key() {compare_key(0x11000575u);return s_.flags.zf;}
    bool matches_second_key() {compare_key(0x1200056au);return s_.flags.zf;}
    bool matches_mode_key() {compare_key(0x120006e4u);return s_.flags.zf;}

    bool has_record_index() {
        auto& g=s_.gp;
        g[1]=read(0x104b50u,g[3]+0x2c8u);
        g[0]=s_.flags.logic(0u,4);
        compare_key(0x0f000574u);
        s_.set(0,!s_.flags.zf,1,0);
        s_.flags.minus(g[0],g[1],4);
        return s_.flags.sf!=s_.flags.of;
    }
    void call_record_ratio() {
        auto& g=s_.gp;
        push(0x104b69u,g[0]);g[0]=g[7];
        push(0x104b6cu,0x104b71u);
        native_object_record_ratio_detail::Frame<State,Before> child{s_,before_,fp_};
        native_object_record_ratio_detail::query(child);
        s_.xmm[2]=s_.xmm[0];
    }
    void clamp_result() {
        s_.flags.scalar_compare(s_.xmm[3],s_.xmm[2],false);
        if(!s_.flags.cf&&!s_.flags.zf){s_.xmm[2]=s_.xmm[3];return;}
        s_.flags.scalar_compare(s_.xmm[2],s_.xmm[4],false);
        if(!s_.flags.cf&&!s_.flags.zf)s_.xmm[2]=s_.xmm[4];
    }
    void finish_result() {
        auto& g=s_.gp;
        s_.flags.scalar_compare(s_.xmm[2],s_.xmm[3],false);
        g[2]=read(0x104cfbu,g[4]+0x20u);
        store(0x104cffu,g[2],std::bit_cast<std::uint32_t>(_mm_cvtss_f32(s_.xmm[2])),[]{});
        if(!s_.flags.cf&&!s_.flags.zf){
            g[1]=read(0x104d05u,g[4]+0x24u);
            g[7]=pop(0x104d09u);g[6]=pop(0x104d0au);g[0]=1u;g[5]=pop(0x104d10u);
            write_success(0x104d11u,g[1],std::byte{1});
            restore_end(0x104d13u,0x104d14u,0x104d15u);
        }else{
            g[1]=read(0x104d18u,g[4]+0x24u);
            g[7]=pop(0x104d1cu);g[6]=pop(0x104d1du);g[0]=s_.flags.logic(0u,4);g[5]=pop(0x104d20u);
            write_success(0x104d21u,g[1],std::byte{});s_.set(0,1u,1,0);
            restore_end(0x104d25u,0x104d26u,0x104d27u);
        }
    }
    void publish_continuation(std::uint32_t pc) {at(pc);}
};

template<class State,class Before> void run_middle_record_path(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Setup<State,Before> setup{state,before,fp};setup.open_object();
    DispatchFrame<State,Before> entry{state,before,fp};
    const auto route=select_initial_route(entry);
    if(route!=Route::middle){entry.publish_continuation(route);return;}
    MiddleFrame<State,Before> middle{state,before,fp};
    const auto middle_route=select_middle_route(middle);
    if(middle_route!=MiddleRoute::record_ratio){middle.publish_continuation(static_cast<std::uint32_t>(middle_route));return;}
    run_record_branch(middle);
}
} // namespace halo2::native_object_ratio_caller_detail
#endif

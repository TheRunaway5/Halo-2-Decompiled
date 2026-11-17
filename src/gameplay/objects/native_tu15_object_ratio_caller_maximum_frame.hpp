#pragma once
#include "halo2/native_tu15_object_ratio_caller_maximum_kernel.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_extended_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_ratio_caller_detail {
template<class State,class Before> class MaximumFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;

    __m128 scalar(std::uint32_t pc,std::uint32_t address) {
        at(pc);return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(word(address))));
    }
    void sample(std::uint32_t compare,std::uint32_t reload,std::uint32_t address) {
        s_.flags.scalar_compare(s_.xmm[2],scalar(compare,address),false);
        if(s_.flags.cf||s_.flags.zf)s_.xmm[2]=scalar(reload,address);
    }
public:
    MaximumFrame(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}} {}
    void begin_groups() {
        auto& g=s_.gp;
        g[1]=s_.flags.right(g[2]-4u,2u,4,false);
        g[1]=s_.flags.increment(g[1],4,false);
        g[0]=g[5]+0x1f0u;g[6]=g[1]*4u;
    }
    void sample_group() {
        const auto address=s_.gp[0];
        sample(0x104790u,0x104796u,address-0x34u);
        sample(0x10479bu,0x1047a0u,address);
        sample(0x1047a4u,0x1047aau,address+0x34u);
        sample(0x1047afu,0x1047b5u,address+0x68u);
    }
    bool advance_group() {
        auto& g=s_.gp;
        g[0]=s_.flags.plus(g[0],0xd0u,4);g[1]=s_.flags.increment(g[1],4,true);
        return !s_.flags.zf;
    }
    bool has_remaining() {
        s_.flags.minus(s_.gp[6],s_.gp[2],4);return s_.flags.sf!=s_.flags.of;
    }
    void begin_remainder() {
        auto& g=s_.gp;
        g[0]=g[5]+0x1bcu+g[6]*0x34u;
        g[2]=s_.flags.minus(g[2],g[6],4);
    }
    void sample_remainder() {sample(0x1047d4u,0x1047d9u,s_.gp[0]);}
    bool advance_remainder() {
        auto& g=s_.gp;
        g[0]=s_.flags.plus(g[0],0x34u,4);g[2]=s_.flags.increment(g[2],4,true);
        return !s_.flags.zf;
    }
};

template<class State,class Before> void run_extended_with_maximum(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Setup<State,Before> setup{state,before,fp};setup.open_object();
    DispatchFrame<State,Before> entry{state,before,fp};
    const auto route=select_initial_route(entry);
    if(route!=Route::extended){entry.publish_continuation(route);return;}
    ExtendedFrame<State,Before> path{state,before,fp};
    const auto next=prepare_extended(path);
    MiddleFrame<State,Before> exit{state,before,fp};
    if(next==ExtendedStage::finish){exit.finish_result();return;}
    if(next!=ExtendedStage::scan_records){
        MaximumFrame<State,Before> maximum{state,before,fp};scan_initial_maximum(maximum,next);
    }
    scan_extended_records(path);exit.clamp_result();exit.finish_result();
}
} // namespace halo2::native_object_ratio_caller_detail
#endif

#pragma once
#include "halo2/native_tu15_object_ratio_caller_combat_kernel.hpp"
#include "gameplay/objects/native_tu15_object_definition_mode_query_frame.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_low_frame.hpp"
#include "gameplay/objects/native_tu15_object_table_ratio_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_ratio_caller_detail {
template<class State,class Before> class CombatFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    Before& before_;
    native_raw_x87_detail::Scope<State>& fp_;

    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {at(pc);return word(address,width);}
    __m128 scalar(std::uint32_t pc,std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(pc,address))));
    }
    bool above() const {return !s_.flags.cf&&!s_.flags.zf;}
    bool key(std::uint32_t symbol) {s_.flags.minus(s_.gp[6],symbol,4);return s_.flags.zf;}
    void lahf_test() {
        const auto ah=(s_.flags.bits()&0xd5u)|2u;
        s_.set(0,ah,1,8);s_.flags.logic(ah&0x44u,1);
    }
public:
    CombatFrame(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},before_(before),fp_(fp) {}

    bool allow_ratio(bool middle) {
        auto& g=s_.gp;
        g[0]=read(middle?0x104b13u:0x1049eeu,0x4f3218u);
        s_.flags.minus(read(middle?0x104b18u:0x1049f3u,g[0]+8u),2u,4);
        s_.xmm[2]=s_.xmm[3];
        if(!s_.flags.zf)return false;
        s_.flags.minus(read(middle?0x104b25u:0x104a00u,g[5]+0x17eu,2),0xffffu,2);
        return !s_.flags.zf;
    }
    bool has_definition_flag(bool middle) {
        s_.gp[1]=s_.gp[7];
        push(middle?0x104b35u:0x104a10u,middle?0x104b3au:0x104a15u);
        native_object_definition_mode_query_detail::Frame<State,Before> child{s_,before_,fp_};
        native_object_definition_mode_query_detail::query(child);
        s_.flags.logic(s_.reg(0,1,0),1);return !s_.flags.zf;
    }
    void call_mode_ratio(bool middle) {
        auto& g=s_.gp;
        push(middle?0x104b42u:0x104a1du,g[7]);
        push(middle?0x104b43u:0x104a1eu,middle?0x104b48u:0x104a23u);
        g[0]=read(0x1655f0u,0x4f3218u);
        s_.flags.minus(read(0x1655f5u,g[0]+0x180u),9u,4);
        if(s_.flags.zf){
            g[0]=read(0x1655feu,g[4]+4u);
            push(0x165602u,0x165607u);
            native_object_table_ratio_detail::Frame<State,Before> child{s_,before_,fp_};
            native_object_table_ratio_detail::query(child);
            pop(0x165607u);
        }else{
            s_.xmm[0]=_mm_setzero_ps();
            pop(0x16560du);
        }
        g[4]+=4u;
    }
    void copy_ratio() {s_.xmm[2]=s_.xmm[0];}
    void pick_low_ratio() {
        auto& x=s_.xmm;
        if(key(0x0f0006e5u)){
            s_.flags.scalar_compare(x[0],x[3],true);lahf_test();
            if(!s_.flags.pf)x[2]=scalar(0x104a38u,0x46a0d0u);
            return;
        }
        if(key(0x0e0006e6u)){
            s_.flags.scalar_compare(x[0],x[3],false);
            if(!above())return;
            x[1]=scalar(0x104a56u,0x46a0d0u);
            s_.flags.scalar_compare(x[1],x[0],false);
            if(above())x[2]=x[1];
            return;
        }
        if(!key(0x0d0006e7u))return;
        x[1]=scalar(0x104a7bu,0x46a0d0u);
        s_.flags.scalar_compare(x[0],x[1],true);lahf_test();
        if(!s_.flags.pf)x[2]=x[1];
    }
};

template<class State,class Before> void run_combat_path(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Setup<State,Before> setup{state,before,fp};setup.open_object();
    DispatchFrame<State,Before> entry{state,before,fp};
    MiddleFrame<State,Before> exit{state,before,fp};
    const auto route=select_initial_route(entry);
    if(route==Route::low){
        LowFrame<State,Before> low{state,before,fp};
        const auto next=select_low_route(low);
        if(next!=LowRoute::combat_ratio){low.publish_continuation(next);return;}
    }else if(route==Route::middle){
        const auto next=select_middle_route(exit);
        if(next!=MiddleRoute::mode_ratio){exit.publish_continuation(static_cast<std::uint32_t>(next));return;}
    }else{entry.publish_continuation(route);return;}
    CombatFrame<State,Before> combat{state,before,fp};
    if(run_combat_ratio(combat,route==Route::middle))exit.clamp_result();
    exit.finish_result();
}
} // namespace halo2::native_object_ratio_caller_detail
#endif

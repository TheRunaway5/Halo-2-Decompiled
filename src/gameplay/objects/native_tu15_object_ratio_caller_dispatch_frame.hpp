#pragma once
#include "halo2/native_tu15_object_ratio_caller_dispatch_kernel.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_setup_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_ratio_caller_detail {
template<class State,class Before> class DispatchFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::store;

    bool signed_at_or_below()const {return s_.flags.zf||s_.flags.sf!=s_.flags.of;}
    void compare_key(std::uint32_t key) {s_.flags.minus(s_.gp[6],key,4);}
public:
    DispatchFrame(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}} {}

    void compare_low_key() {compare_key(0x10000573u);}
    void publish_definition() {
        auto& g=s_.gp;
        at(0x104706u);g[3]=word(g[1]+g[0]+8u);
        store(0x10470au,g[4]+0x10u,g[3],[]{});
        s_.xmm[2]=s_.xmm[3];
    }
    bool at_or_below_low() const {return signed_at_or_below();}
    bool at_or_below_middle() {compare_key(0x1500056eu);return signed_at_or_below();}
    bool at_or_below_high() {compare_key(0x1700056cu);return signed_at_or_below();}
    bool matches_exact() {compare_key(0x1700056fu);return s_.flags.zf;}
    bool matches_extended() {compare_key(0x1d0007bau);return s_.flags.zf;}
    void publish_continuation(Route route) {at(static_cast<std::uint32_t>(route));}
};

template<class State,class Before> Route run_initial_dispatch(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Setup<State,Before> setup{state,before,fp};setup.open_object();
    DispatchFrame<State,Before> view{state,before,fp};
    const auto route=select_initial_route(view);view.publish_continuation(route);return route;
}
} // namespace halo2::native_object_ratio_caller_detail
#endif

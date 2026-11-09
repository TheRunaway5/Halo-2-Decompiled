#pragma once
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"

namespace halo2::native_sse_mmx_detail {
// Only an x87/MMX-neutral numerical body may borrow this owner. The source's
// eight opaque physical aliases and known mask stay in retained State; neither
// numeric interpretation nor temporary clearing of active_mmx is permitted.
template<class State>bool supported(const State&state) noexcept {
    if constexpr(!requires{state.active_mmx;})return false;
    else if(!state.active_mmx||state.fp.count||state.fp.top)return false;
    auto image=native_raw_x87_detail::capture();image.word(4,state.fp.word());
    return native_raw_x87_detail::controls_supported(image);
}
class Scope {
    using Image=native_raw_x87_detail::Image;
    Image host_;
public:
    template<class State>explicit Scope(const State&state):host_(native_raw_x87_detail::capture()) {
        if(!supported(state))throw std::runtime_error("SSE opaque-MMX owner requires masked TOP0/depth0 state");
        auto source=host_;source.word(4,state.fp.word());source.word(8,0xffffu);
        // C++ providers borrow an empty host x87 register file under source
        // controls/status. The actual source MMX file remains opaque State.
        native_raw_x87_detail::restore(source);
    }
    Scope(const Scope&)=delete;Scope&operator=(const Scope&)=delete;
    ~Scope() noexcept {
        const auto source=native_raw_x87_detail::capture();auto returned=host_;
        returned.word(0,source.word(0));
        returned.word(4,std::uint16_t((source.word(4)&~0x3800u)|(host_.word(4)&0x3800u)));
        returned.mxcsr=source.mxcsr;
        // State.save samples current source controls/exception status. Only
        // the caller's payloads, tags, and physical TOP are restored here.
        native_raw_x87_detail::restore(returned);
    }
    template<class Callback>void io(Callback&&callback) {
        static_assert(std::is_void_v<std::invoke_result_t<Callback>>);
        const auto source=native_raw_x87_detail::capture();
        try {std::forward<Callback>(callback)();}
        catch(...) {native_raw_x87_detail::restore(source);throw;}
        native_raw_x87_detail::restore(source);
    }
};
} // namespace halo2::native_sse_mmx_detail

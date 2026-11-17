#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"

namespace halo2::native_integer_fp_detail {
// An integer-only owner keeps the complete source register file in State.
// No x87/MMX arithmetic, payload interpretation or clearing of active_mmx is
// permitted. Unlike RawScope, this owner never imports numeric source lanes.
template<class State>bool supported(const State&state) noexcept {
    if(state.fp.count>8u||state.fp.top>7u)return false;
    if constexpr(requires{state.active_mmx;})
        if(state.active_mmx&&(state.fp.count||state.fp.top))return false;
    auto image=native_raw_x87_detail::capture();image.word(4,state.fp.word());
    return native_raw_x87_detail::controls_supported(image);
}
template<class State>class Scope {
    using Image=native_raw_x87_detail::Image;
    State&state_;Image host_;bool live_{true};
    void return_host(Image source) noexcept {
        auto returned=host_;returned.word(0,source.word(0));
        returned.word(4,std::uint16_t((source.word(4)&~0x3800u)|(host_.word(4)&0x3800u)));
        returned.mxcsr=source.mxcsr;native_raw_x87_detail::restore(returned);
    }
    void adopt() {
        if(!supported(state_)) {
            live_=false;return_host(native_raw_x87_detail::capture());
            throw std::runtime_error("Integer FP continuation requires valid masked retained state");
        }
        // State.load owns the returned controls and exception bits. Publish
        // its C bits in the parked host image, with an empty temporary file.
        auto source=native_raw_x87_detail::capture();
        source.word(4,std::uint16_t(state_.fp.word()&~0x3800u));source.word(8,0xffffu);
        native_raw_x87_detail::restore(source);
    }
public:
    explicit Scope(State&state):state_(state),host_(native_raw_x87_detail::capture()) {
        if(!supported(state_))throw std::runtime_error("Integer FP input requires valid masked retained state");
        adopt();
    }
    Scope(const Scope&)=delete;Scope&operator=(const Scope&)=delete;
    ~Scope() noexcept {
        if(!live_)return;
        auto source=native_raw_x87_detail::capture();source.word(4,state_.fp.word());
        return_host(source);
    }
    template<class Callback>void io(Callback&&callback) {
        static_assert(std::is_void_v<std::invoke_result_t<Callback>>);
        const auto source=native_raw_x87_detail::capture();
        try{std::forward<Callback>(callback)();}
        catch(...){native_raw_x87_detail::restore(source);throw;}
        native_raw_x87_detail::restore(source);
    }
    template<class Callback>void child(Callback&&callback) {
        static_assert(std::is_void_v<std::invoke_result_t<Callback>>);
        try{std::forward<Callback>(callback)();}
        catch(...) {
            const auto original=std::current_exception();
            try{adopt();}catch(...){std::rethrow_exception(original);}
            std::rethrow_exception(original);
        }
        adopt();
    }
};
} // namespace halo2::native_integer_fp_detail
#else
namespace halo2::native_integer_fp_detail {
template<class State>bool supported(const State&) noexcept{return false;}
}
#endif

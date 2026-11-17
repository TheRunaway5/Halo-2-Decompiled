#pragma once

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_model_first_selector_frame.hpp"

namespace halo2::native_codec_model_secondary_data_detail {
enum class Boundary { returned, secondary_data };

using native_codec_model_first_selector_detail::Noop;
using native_codec_model_first_selector_detail::Io;

template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>> {
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;

public:
    Frame(State& state,Before& before,Scope& scope):Base{state,before,{&scope}}{}

    bool has_secondary_data() {
        auto& gp=s_.gp;
        at(0x283947u);
        gp[0]=word(gp[6]+4u);
        at(0x28394au);
        const auto length=word(gp[0]+12u);
        s_.flags.minus(length,gp[3],4u);
        return !s_.flags.zf;
    }

    void finish_parent() {
        auto& gp=s_.gp;
        gp[5]=Base::pop(0x283a3cu);
        gp[7]=Base::pop(0x283a3du);
        gp[3]=Base::pop(0x283a3eu);
        (void)Base::pop(0x283a3fu);
        gp[4]+=40u;
    }

    Boundary finish_without_secondary_data() {
        if(has_secondary_data())return Boundary::secondary_data;
        finish_parent();
        return Boundary::returned;
    }
};

template<class State,class Before=Noop>Boundary run_private(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};
    Frame view{state,before,scope};
    return view.finish_without_secondary_data();
}
} // namespace halo2::native_codec_model_secondary_data_detail
#endif

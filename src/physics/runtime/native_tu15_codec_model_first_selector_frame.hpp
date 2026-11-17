#pragma once

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_dispatch_frame.hpp"
#include "physics/runtime/native_tu15_codec_model_selector_setup_frame.hpp"

namespace halo2::native_codec_model_first_selector_detail {
enum class Boundary { returned, bypassed, tail_entry, selector_returned };

struct Stop {
    Boundary kind;
    std::uint32_t instruction;
};

using native_codec_model_selector_setup_detail::Noop;
using native_codec_model_selector_setup_detail::Io;

template<class State,class Before>
Stop select_after_push(State& state,Before& before,native_integer_fp_detail::Scope<State>& scope) {
    namespace dispatch=native_codec_dispatch_detail;
    dispatch::Frame format{state,before,scope,0u};
    const auto field=dispatch::format(format);
    if(!field.target)return {Boundary::selector_returned,field.instruction};

    for(unsigned profile=0;profile<dispatch::field_entries.size();++profile) {
        if(dispatch::field_entries[profile]!=field.target)continue;
        dispatch::Frame decoder{state,before,scope,profile};
        const auto tail=dispatch::field(decoder);
        if(!tail.target)return {Boundary::selector_returned,tail.instruction};
        return {Boundary::tail_entry,tail.target};
    }
    throw std::logic_error("Unexpected TU1.5 selector field entry");
}

template<class State,class Before>class FirstSelectorCall:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>> {
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::before_;

public:
    FirstSelectorCall(State& state,Before& before,Scope& scope):Base{state,before,{&scope}}{}

    Stop select(Scope& scope) {
        Base::push(0x283942u,0x283947u);
        return select_after_push(s_,before_,scope);
    }
};

template<class State,class Before=Noop>Stop run_private_prefix(State& state,Before before={}) {
    namespace initialize=native_codec_model_initialize_detail;
    namespace mode=native_codec_model_mode_detail;
    namespace setup=native_codec_model_selector_setup_detail;
    native_integer_fp_detail::Scope scope{state};

    initialize::Frame initial{state,before,scope};
    const auto branch=initialize::initialize(initial);
    if(branch==initialize::Boundary::returned)return {Boundary::returned,0x283a3fu};

    mode::Frame modes{state,before,scope};
    mode::compose_mode(modes,branch);
    setup::Frame selector{state,before,scope};
    if(setup::prepare_selector(selector)==setup::Boundary::bypassed)return {Boundary::bypassed,0x283947u};

    FirstSelectorCall call{state,before,scope};
    return call.select(scope);
}
} // namespace halo2::native_codec_model_first_selector_detail
#endif

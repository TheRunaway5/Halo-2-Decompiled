#pragma once
#include "physics/runtime/native_tu15_codec_format_four_math.hpp"
#include "physics/runtime/native_tu15_codec_format_four_source_frame.hpp"
#include "gameplay/weapons/native_tu15_model_marker_lookup_admission.hpp"
#include <cstdlib>

namespace halo2::native_codec_format_four_owner_detail {
struct Continuation {std::uint32_t pc;const void* owner;};
struct Activity {std::uint64_t calls{},completed{},continuations{},children{};};
inline Activity& activity(){thread_local Activity value;return value;}
inline bool reference() {
    static const bool enabled=[]{const auto* value=std::getenv("HALO2_REFERENCE_CODEC_FORMAT_FOUR_DECODE");return value&&*value;}();
    return enabled;
}

template<bool Literal,class State>bool admitted(const State& state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(!Literal)if(state.services.instruction)return false;
    if(reference()||state.image!=SourcePhysicsContactImage::TitleUpdate15||
       !native_integer_fp_detail::supported(state)||
       !native_model_marker_lookup_detail::protocol(state.services,state.observe))return false;
    if constexpr(requires{state.selected_observation_pcs;})if(!state.selected_observation_pcs)return false;
    return state.services.instruction?state.literal_service_sites&&state.instruction_cursor==&state.services.instruction_cursor:
        !state.instruction_cursor&&!state.literal_service_sites;
#else
    (void)state;return false;
#endif
}

template<bool Literal,class State,class Call>std::uint32_t bridge(State& state,Call call) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!admitted<Literal>(state))return 0u;
    ++activity().calls;
    const std::byte token{};
    auto child=[&](auto& current,std::uint32_t target,std::uint32_t pc){++activity().children;call(current,target,pc);};
    auto checkpoint=[&](auto& current,std::uint32_t next) {
        if(!admitted<Literal>(current))throw Continuation{next,&token};
    };
    try {
        native_codec_format_four_source_detail::run_private_source<Literal>(state,child,checkpoint);
        ++activity().completed;return 1u;
    } catch(const Continuation& resume) {
        if(resume.owner!=&token)throw;
        ++activity().continuations;return resume.pc;
    }
#else
    (void)state;(void)call;return 0u;
#endif
}
} // namespace halo2::native_codec_format_four_owner_detail

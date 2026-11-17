#pragma once
#include "gameplay/weapons/native_tu15_model_marker_lookup_admission.hpp"
#include "physics/runtime/native_tu15_codec_row_copy_source_frame.hpp"
#include "physics/runtime/native_tu15_codec_scale_row_source_frame.hpp"
#include "physics/runtime/native_tu15_integer_fp_scope.hpp"
#include <cstdlib>

namespace halo2::native_codec_copy_owner_detail {
enum class Entry { quaternion, translation, scale };
struct Activity { std::uint64_t calls{}, completed{}, continuations{}; };

template<Entry Profile>Activity& activity() {
    thread_local Activity value{};
    return value;
}

inline bool reference() {
    static const bool enabled = [] {
        const auto* value = std::getenv("HALO2_REFERENCE_CODEC_COPY_ROWS");
        return value && *value;
    }();
    return enabled;
}

template<bool Literal,class State>bool admitted(const State& s) {
    if constexpr(!Literal)if(s.services.instruction)return false;
    if(reference() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       !native_integer_fp_detail::supported(s) ||
       !native_model_marker_lookup_detail::protocol(s.services,s.observe))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    return s.services.instruction ?
        s.literal_service_sites && s.instruction_cursor==&s.services.instruction_cursor :
        !s.instruction_cursor && !s.literal_service_sites;
}

template<Entry Profile,bool Literal,class State>std::uint32_t bridge(State& s) {
    if(!admitted<Literal>(s))return 0u;
    auto& count=activity<Profile>();
    ++count.calls;
    // This view is integer-only. It retains the authored provider publication
    // order and leaves source FP metadata and host numeric state in place.
    const auto remains_native=[](const auto& current,std::uint32_t) {return admitted<Literal>(current);};
    const auto next=[&] {
        if constexpr(Profile==Entry::scale)return native_codec_scale_row_detail::run_source(s,remains_native);
        else {
            constexpr auto row=Profile==Entry::quaternion ? native_codec_row_copy_detail::Row::quaternion :
                native_codec_row_copy_detail::Row::translation;
            return native_codec_row_copy_detail::run_source<row>(s,remains_native);
        }
    }();
    if(next)++count.continuations;
    else ++count.completed;
    return next ? next : 1u;
}
} // namespace halo2::native_codec_copy_owner_detail

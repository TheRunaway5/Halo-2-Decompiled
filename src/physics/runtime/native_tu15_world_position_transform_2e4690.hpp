#pragma once

#include "physics/runtime/native_tu15_geometry_leaf_frame.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"

namespace halo2::native_world_position_transform_2e4690_detail {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// World providers own their original I/O and may modify the live FP
// environment. The physical oracle's aligned, parked transport is separate.
template<class State> class WorldTransport {
protected:
    State &s_;
    std::uint32_t &cursor_;

    void at(std::uint32_t pc) { cursor_ = pc; }
    std::uint32_t word(std::uint32_t address) {
        return physics_step_detail::r(s_.memory,address);
    }
    __m128 vector(std::uint32_t pc,std::uint32_t address) {
        at(pc);
        return physics_step_detail::v(s_.memory,address);
    }
    void vector_store(std::uint32_t pc,std::uint32_t address,__m128 value) {
        at(pc);
        physics_step_detail::vw(s_.memory,s_.observe,pc,address,value);
    }
    void push(std::uint32_t pc,std::uint32_t value) {
        at(pc);
        s_.push(pc,value);
    }
    std::uint32_t pop(std::uint32_t pc) {
        at(pc);
        return s_.pop();
    }

public:
    WorldTransport(State &state,std::uint32_t &cursor) : s_(state), cursor_(cursor) {}
};
#endif

template<class State> bool bridge(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto *setting = std::getenv("HALO2_REFERENCE_WORLD_POSITION_TRANSFORM_2E4690");
        return setting && *setting;
    }();
    // Only World seals its const services table. Public or remapped
    // callbacks can replace their cursor or ask for all 26 instruction PCs.
    const auto *cursor = s.immutable_cursor.cursor;
    if (reference || !s.selected_observation_pcs || s.image != SourcePhysicsContactImage::TitleUpdate15 ||
        !cursor || !cursor->position || (cursor->first_trace && *cursor->first_trace) ||
        (cursor->second_trace && *cursor->second_trace) || (s.observe && !cursor->observation_only_stores) ||
        (_mm_getcsr() & 0x1f80u) != 0x1f80u)
        return false;

    // The sealed literal service sites are 38846E, 21F590 and 33B595.
    using namespace native_geometry_leaf_detail;
    Frame<WorldTransport<State>> frame{Kind::position,false,s,*cursor->position};
    frame.begin();
    transformed_position(frame);
    frame.finish();
    return true;
#else
    (void)s;
    return false;
#endif
}
} // namespace halo2::native_world_position_transform_2e4690_detail

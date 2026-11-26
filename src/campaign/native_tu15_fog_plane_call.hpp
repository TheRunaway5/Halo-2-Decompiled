#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_fog_plane_call_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;

inline constexpr std::uint32_t site = 0x133d8au;
inline constexpr std::uint32_t continuation = 0x133d8fu;
inline constexpr std::uint32_t vector_angle = 0x11f7e0u;

template<class State, class Dispatch> struct VerifiedCall : Port<State> {
    using Base = Port<State>;
    Dispatch &dispatch;
    VerifiedCall(State &s, std::uint32_t *cursor, Trace<State> *trace, Dispatch &route)
        : Base(s, cursor, trace, {}), dispatch(route) {}
    void run() {
        try {
            this->push(site, continuation);
        } catch (...) {
            // PUSH decrements ESP even if its preimage read, store or
            // observer fails. The valid original child has not run yet.
            this->s.gp = this->gp;
            throw;
        }
        // The genuine dispatcher owns all changes and faults in the child.
        // Do not restore the saved register packet or CALL PC on its exit.
        this->enter(vector_angle, [&](State &s) { dispatch(s, vector_angle, site, false); });
        this->reached(0);
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Campaign fog vector-angle call"};
    return value;
}

template<class State, class Reference, class Dispatch>
bool bridge(State &s, Reference reference, Dispatch dispatch) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_FOG_PLANE_CALL");
    static const bool verify = flag("HALO2_VERIFY_FOG_PLANE_CALL");
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return false;
    }
    if (verify)
        return native_small_queries_detail::run_owner(s, site, site, force_reference, true, coverage(), summary(), reference,
            [&](State &v, std::uint32_t *cursor, Trace<State> *trace) { return VerifiedCall<State,Dispatch>{v, cursor, trace, dispatch}; });
    std::uint32_t *cursor{};
    if (!native_object_tick_detail::admitted(s, site, site, cursor, force_reference))
        return false;
    if (cursor)
        *cursor = site;
    s.push(site, continuation);
    dispatch(s, vector_angle, site, false);
    return true;
}

} // namespace halo2::native_fog_plane_call_detail

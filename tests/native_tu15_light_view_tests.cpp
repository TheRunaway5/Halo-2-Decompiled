#include "pstencil/native_tu15_light_view.hpp"
#include "native_tu15_shadow_frame_harness.hpp"

namespace copies {
using namespace physics_world_runtime_detail;
using WorldState = InstrumentedState;
#include "light_view_bodies.inc"
}

struct Owner {
    static constexpr bool core_only = false;
    static constexpr const char* name = "light view";
    static constexpr const char* reference_test_variable = "HALO2_LIGHT_VIEW_REFERENCE_TEST";
    static constexpr const char* census_test_variable = "HALO2_LIGHT_VIEW_CENSUS_TEST";

    static void retained(InstrumentedState& state) { copies::retained::run_134c00(state); }
    static void bound(InstrumentedState& state) { copies::bound::run_134c00(state); }
    static void prepared(InstrumentedState& state) { copies::prepared::run_134c00(state); }
    static bool reference() { return native_light_visibility_detail::reference(); }

    template<class State> static bool policy_admitted(const State& state) {
        return native_light_visibility_detail::admitted(state, 0x134c00, 0x134cf0);
    }
    template<class State, class Call, class Before>
    static bool native(State& state, Call& call, Before& before) {
        auto dispatch = [&](auto& child, auto target, auto site) { call(child, target, site, before); };
        return native_light_view_detail::bridge(state, dispatch, before);
    }
    template<class State, class Call> static bool production(State& state, Call& call) {
        native_light_visibility_detail::Noop before;
        return native(state, call, before);
    }
    template<class State, class Before>
    static bool child(State&, std::uint32_t, std::uint32_t, Before&) { return false; }
};

int main(int argc, char** argv) { return shadow_frame_harness::run<Owner>(argc, argv); }

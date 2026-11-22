#include "pstencil/native_tu15_light_point_transform.hpp"
#include "native_tu15_shadow_frame_harness.hpp"

namespace copies {
using namespace physics_world_runtime_detail;
using WorldState = InstrumentedState;
#include "light_point_transform_bodies.inc"
}

struct Owner {
    static constexpr bool core_only = false;
    static constexpr const char* name = "light point transform";
    static constexpr const char* reference_test_variable = "HALO2_LIGHT_POINT_TRANSFORM_REFERENCE_TEST";
    static constexpr const char* census_test_variable = "HALO2_LIGHT_POINT_TRANSFORM_CENSUS_TEST";

    static void retained(InstrumentedState& state) { copies::retained::run_1454d0(state); }
    static void bound(InstrumentedState& state) { copies::bound::run_1454d0(state); }
    static void prepared(InstrumentedState& state) { copies::prepared::run_1454d0(state); }
    static bool reference() { return native_light_visibility_detail::reference(); }

    template<class State> static bool policy_admitted(const State& state) {
        return native_light_visibility_detail::admitted(state, 0x1454d0, 0x145558);
    }
    template<class State, class Call, class Before>
    static bool native(State& state, Call&, Before& before) {
        return native_light_point_transform_detail::bridge(state, before);
    }
    template<class State, class Call> static bool production(State& state, Call&) {
        return native_light_point_transform_detail::bridge(state);
    }
    template<class State, class Before>
    static bool child(State&, std::uint32_t, std::uint32_t, Before&) { return false; }
};

int main(int argc, char** argv) { return shadow_frame_harness::run<Owner>(argc, argv); }

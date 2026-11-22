#include "pstencil/native_tu15_light_admission.hpp"
#include "native_tu15_shadow_frame_harness.hpp"

namespace copies {
using namespace physics_world_runtime_detail;
using WorldState = InstrumentedState;
#include "light_admission_bodies.inc"
}

struct Owner {
    static constexpr bool core_only = false;
    static constexpr const char* name = "light admission";
    static constexpr const char* reference_test_variable = "HALO2_LIGHT_ADMISSION_REFERENCE_TEST";
    static constexpr const char* census_test_variable = "HALO2_LIGHT_ADMISSION_CENSUS_TEST";

    static void retained(InstrumentedState& state) { copies::retained::run_31570(state); }
    static void bound(InstrumentedState& state) { copies::bound::run_31570(state); }
    static void prepared(InstrumentedState& state) { copies::prepared::run_31570(state); }
    static bool reference() { return native_light_visibility_detail::reference(); }

    static std::uint32_t undefined_flags(std::uint32_t pc) {
        // SHL ECX,4 leaves AF undefined through the tag load, call push and
        // 31500 entry.
        // 31500 sets arithmetic flags before consuming any such input.
        return pc == 0x315c2 || pc == 0x315c8 || pc == 0x31500 ? 0x10 : 0;
    }

    template<class State> static bool policy_admitted(const State& state) {
        return native_light_visibility_detail::admitted(state, 0x31570, 0x3164d);
    }
    template<class State, class Call, class Before>
    static bool native(State& state, Call& call, Before& before) {
        auto dispatch = [&](auto& child, auto target, auto site) { call(child, target, site, before); };
        return native_light_admission_detail::bridge(state, dispatch, before);
    }
    template<class State, class Call> static bool production(State& state, Call& call) {
        native_light_visibility_detail::Noop before;
        return native(state, call, before);
    }
    template<class State, class Before>
    static bool child(State&, std::uint32_t, std::uint32_t, Before&) { return false; }
};

int main(int argc, char** argv) { return shadow_frame_harness::run<Owner>(argc, argv); }

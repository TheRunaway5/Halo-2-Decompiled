#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "physics/runtime/native_tu15_normalize_vector_bridge.hpp"
#include "rendering/effects/native_tu15_particle_system_contact_basis.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace owner = halo2::native_particle_system_contact_basis_detail;

namespace children {
#include "system_contact_basis_children.inc"
} // namespace children

namespace {
void run_30bd0(WorldState &state) {
    log_call(entry_hash(state, owner::normalize));
    children::run_30bd0(state);
}

void run_11f9c0(WorldState &state) {
    log_call(entry_hash(state, owner::perpendicular));
    children::run_11f9c0(state);
}
} // namespace

namespace original {
using ::run_11f9c0;
using ::run_30bd0;
#include "system_contact_basis.inc"
} // namespace original

namespace {
bool native_basis(WorldState &state) {
    return owner::bridge(state, original::native_particle_system_contact_basis_reference_1778d0,
        [](WorldState &value, auto target) {
            if constexpr (decltype(target)::value == owner::normalize) run_30bd0(value);
            if constexpr (decltype(target)::value == owner::perpendicular) run_11f9c0(value);
        });
}

unsigned compare(const Case &test, unsigned at, Totals &totals) {
    return fixture::compare_geometry(test, at, totals, native_basis,
        original::native_particle_system_contact_basis_reference_1778d0_live,
        [](auto site) { return site >= owner::first && site <= owner::last; });
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    fixture::use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x177a2fu);
    Totals totals;
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = fixture::make(random, index, owner::velocity);
        const auto accesses = compare(test, 0u, totals);
        if (index < 32u) {
            for (unsigned at = 1u; at <= accesses; ++at) compare(test, at, totals);
        }
    }
    if (!census_mode) {
        check_refusals(fixture::make(random, count, owner::velocity), native_basis);
        check_birth_observer_refusals(fixture::make(random, count, owner::velocity), native_basis);
    }
    report(count, totals, 1u, 1u, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

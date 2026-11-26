#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_system_velocity_basis.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace owner = halo2::native_particle_system_velocity_basis_detail;
namespace world = halo2::native_world_leaves_detail;

namespace original {
#include "system_velocity_basis.inc"
} // namespace original

namespace {
Case make_case(std::mt19937 &random, unsigned index) {
    auto test = fixture::make(random, index, owner::velocity);
    const auto base = test.abi.gp[world::ebx] + owner::velocity;
    if (index % 16u == 0u) {
        test.memory.put(owner::negative_one, 0xbf800000u);
        test.memory.put(base, 0x35800000u);
        test.memory.put(base + 4u, 0u);
        test.memory.put(base + 8u, 0x80000000u);
    }
    if (index % 23u < 2u) {
        test.abi.x87_depth = index % 23u == 0u ? 7u : 8u;
        for (unsigned lane = 0u; lane < test.abi.x87_depth; ++lane)
            test.abi.x87[lane] = static_cast<long double>(lane * 2u + 1u) / 8.0L;
    }
    if (index % 13u == 0u) test.abi.x87_status |= 0x200u;
    return test;
}

bool native_velocity(WorldState &state) {
    return owner::bridge_velocity(state, original::native_particle_system_velocity_basis_reference_1778d0);
}

bool native_lane(WorldState &state) {
    return owner::bridge_lane(state, original::native_particle_system_basis_final_lane_reference_1778d0);
}

unsigned compare(const Case &test, unsigned at, Totals &totals) {
    const bool lane_only = test.policy % 8u == 3u;
    const auto kernel = [lane_only](WorldState &state) {
        return (lane_only || native_velocity(state)) && native_lane(state);
    };
    const auto source = [lane_only](WorldState &state) {
        if (!lane_only) original::native_particle_system_velocity_basis_reference_1778d0_live(state);
        original::native_particle_system_basis_final_lane_reference_1778d0_live(state);
    };
    return fixture::compare_geometry(test, at, totals, kernel, source,
        [](auto site) { return site >= owner::first && site <= owner::final_store; });
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    fixture::use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x177b7cu);
    Totals totals;
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random, index);
        const auto accesses = compare(test, 0u, totals);
        if (index < 48u) {
            for (unsigned at = 1u; at <= accesses; ++at) compare(test, at, totals);
        }
    }
    if (!census_mode) {
        check_refusals(make_case(random, count), native_velocity);
        check_birth_observer_refusals(make_case(random, count), native_velocity);
        check_refusals(make_case(random, count), native_lane);
        check_birth_observer_refusals(make_case(random, count), native_lane);
    }
    report(count, totals, owner::summary().coverage.load(), owner::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_contact_response.hpp"
#include "rendering/effects/native_tu15_particle_property.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace kernel = halo2::native_particle_contact_response_detail;

#include "particle_contact_response_reference.inc"

namespace {
constexpr std::uint32_t frame = 0x28002300u, particle = 0x28004300u, descriptors = 0x28006300u, curve = 0x28008300u;
constexpr std::array<std::uint32_t, 12> edge_words{
    0u, 0x80000000u, 0x7fc00017u, 0x7f800019u, 0xffc00031u, 0x7f800000u,
    0xff800000u, 0x00000001u, 0x807fffffu, 0x7f7fffffu, 0x3f800000u, 0x3f200000u};

Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, frame);
    test.policy = index;
    test.abi.fs_base = random();
    test.abi.gp[1] = 0u;
    test.abi.gp[5] = index % 17u ? particle : frame + 0x90u;
    test.abi.gp[6] = descriptors;
    for (std::uint32_t address = frame - 0x80u; address <= frame + 0x144u; address += 4u)
        test.memory.put(address, edge_words[(index + address / 4u) % edge_words.size()]);
    const auto datum = test.abi.gp[5];
    for (std::uint32_t offset = 0u; offset < 0x40u; offset += 4u)
        test.memory.put(datum + offset, edge_words[(index + offset / 4u + 5u) % edge_words.size()]);
    test.memory.put(datum + offsetof(kernel::walk::ParticleDatum, flags), index & 0xffffu, 2);
    test.memory.put(frame + 0x10u, index % 4u);
    test.memory.put(frame + 0x14u, 0u);
    if (index % 16u == 0u) {
        test.memory.put(datum + kernel::velocity, 0x7fc00019u);
        test.memory.put(datum + kernel::velocity + 4u, 0x3f000000u);
        test.memory.put(datum + kernel::velocity + 8u, 0x3f800000u);
        test.memory.put(frame + 0xd8u, 0xffc00429u);
        test.memory.put(frame + 0xdcu, 0u);
        test.memory.put(frame + 0xe0u, 0x3f800000u);
    }
    for (const auto address : {descriptors, descriptors + 0x14u}) {
        test.memory.put(address, 0u, 2);
        test.memory.put(address + 2u, 1u, 2);
        test.memory.put(address + 6u, 0xffffu, 2);
        test.memory.put(address + 12u, curve);
    }
    test.memory.put(curve, 0u);
    test.memory.put(curve + 4u, 0x3ed00000u);
    test.memory.put(curve + 8u, 0x3f300000u);
    test.memory.put(kernel::walk::location::globals::one, 0x3f800000u);
    test.memory.put(kernel::contact_offset, index % 3u ? 0x3e800000u : 0x7fc00479u);
    test.memory.put(0x474d1cu, 0u);
    return test;
}

bool bridge(WorldState &state) {
    return kernel::bridge_response(state, prepared::native_particle_contact_response_reference_2c2d90,
                                    children::response_local);
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    fixture::use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x2c30b8u);
    Totals totals;
    const auto source = original::native_particle_contact_response_reference_2c2d90_live;
    const auto watched = [](std::uint32_t site) { return site >= kernel::first && site <= kernel::last; };
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random, index);
        const auto accesses = fixture::compare_geometry(test, 0u, totals, bridge, source, watched);
        if (index < 64u)
            for (unsigned at = 1u; at <= accesses; ++at)
                fixture::compare_geometry(test, at, totals, bridge, source, watched);
    }
    if (!census_mode) {
        const auto control = make_case(random, count);
        check_refusals(control, bridge);
        check_birth_observer_refusals(control, bridge);
    }
    report(count, totals, 0u, 0u, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

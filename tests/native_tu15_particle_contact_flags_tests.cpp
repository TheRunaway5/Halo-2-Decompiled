#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_contact_flags.hpp"
#include "rendering/effects/native_tu15_particle_property.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace kernel = halo2::native_particle_contact_flags_detail;
using kernel::Resume;

#include "particle_contact_flags_reference.inc"

namespace {
constexpr std::uint32_t frame = 0x28001200u, particle = 0x28003400u, system_record = 0x28005100u;

Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, frame);
    test.policy = index;
    test.abi.gp[5] = particle;
    test.abi.fs_base = random();
    for (std::uint32_t address = frame - 0x80u; address <= frame + 0x148u; address += 4u)
        test.memory.put(address, random());
    for (std::uint32_t offset = 0u; offset < 0x40u; offset += 4u)
        test.memory.put(particle + offset, random());
    test.memory.put(particle + 2u, index % 3u ? random() | 2u : 0xffffu, 2);
    test.memory.put(frame + kernel::cached_particle, index % 2u ? particle : 0u);
    test.memory.put(frame + kernel::cached_mask, index % 3u ? 0u : random());
    test.memory.put(frame + kernel::cached_system, system_record);
    test.memory.put(frame + 0x9cu, 0u);
    test.memory.put(frame + 0x138u, system_record);
    test.memory.put(frame + 0x148u, index % 64u);
    test.memory.put(system_record + 0x10u, index % 5u ? index % 16u : 0u);
    test.memory.put(0x46a160u, 0x37800000u);
    test.memory.put(0x252430u, 0x252119u);
    test.memory.put(0x252434u, 0x25212au);
    test.memory.put(0x252438u, 0x25213bu);
    test.memory.put(0x25243cu, 0x25213bu);
    return test;
}

Resume bridge(WorldState &state) {
    return kernel::bridge_flags(state, prepared::native_particle_contact_flags_reference_2c2d90,
                                children::call_local);
}

unsigned compare(const Case &test, unsigned at, Totals &totals, std::uint64_t &coverage) {
    Resume mine{}, theirs{};
    const auto native = [&](WorldState &state) {
        mine = bridge(state);
        return mine != Resume::retained;
    };
    const auto source = [&](WorldState &state) {
        theirs = original::native_particle_contact_flags_reference_2c2d90_live(state);
    };
    const auto watched = [](std::uint32_t site) { return site >= kernel::first && site <= kernel::last; };
    const auto accesses = fixture::compare_geometry(test, at, totals, native, source, watched);
    if (mine != theirs) throw std::runtime_error("Particle contact flags resume differential mismatch");
    coverage |= std::uint64_t{1} << static_cast<unsigned>(mine);
    return accesses;
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    fixture::use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x2c2ee0u);
    Totals totals;
    std::uint64_t coverage{};
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random, index);
        const auto accesses = compare(test, 0u, totals, coverage);
        if (index < 64u)
            for (unsigned at = 1u; at <= accesses; ++at) compare(test, at, totals, coverage);
    }
    if (!census_mode) {
        const auto control = make_case(random, count);
        const auto attempt = [](WorldState &state) { return bridge(state) != Resume::retained; };
        check_refusals(control, attempt);
        check_birth_observer_refusals(control, attempt);
    }
    if (coverage != 7u) throw std::runtime_error("Particle contact flags fixture missed a route");
    report(count, totals, coverage, 3u, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

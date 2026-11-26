#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_contact_query.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace kernel = halo2::native_particle_contact_query_detail;
using kernel::Resume;

namespace child {
// Fixture controls for the owned cross-unit boundary. The production hook
// uses Effect Frame's dispatch with the real 16C620 owner and service bank.
void dispatch(WorldState &s, std::uint32_t target, std::uint32_t at, bool tail = false) {
    if (target != kernel::query || at != kernel::call || tail)
        throw std::logic_error("Unexpected particle contact sweep target");
    log_call(entry_hash(s, target));
    const auto sp = s.gp[4], frame = sp + 28u;
    const auto mask = r(s.memory, sp + 4u, 4);
    const auto origin = r(s.memory, sp + 8u, 4);
    const auto direction = r(s.memory, sp + 12u, 4);
    const auto ignored_first = r(s.memory, sp + 16u, 4);
    const auto ignored_second = r(s.memory, sp + 20u, 4);
    const auto result = r(s.memory, sp + 24u, 4);
    if (mask != s.gp[0] || origin != frame + 0x20u || direction != frame + 0x44u ||
        ignored_first != ignored_second || ignored_first != s.gp[3] || result != frame + 0xb0u)
        throw std::logic_error("Particle contact sweep arguments changed");
    for (unsigned i = 0; i < 12u; i += 4u) {
        const auto start = r(s.memory, origin + i, 4);
        const auto delta = r(s.memory, direction + i, 4);
        log_call((std::uint64_t(start) << 32u) | delta);
    }
    constexpr std::array fractions{0u, 0x3e800000u, 0x3f000000u, 0x3f800000u, 0x7fc02201u, 0x7f802104u};
    const auto selector = policy % fractions.size();
    s.gp[0] = s.flags.logic(selector ? 0x8b771101u : 0x77339900u, 4);
    s.gp[1] ^= 0xff157393u;
    s.gp[2] ^= 0xfedca012u;
    s.xmm[1] = _mm_set_ss(0.5f);
    w(s.memory, s.observe, kernel::query, result + 4u, 4, fractions[selector]);
    if (s.pop() != 0x2c3090u) throw std::logic_error("Particle contact sweep return address changed");
    s.gp[4] += 24u;
    log_call(entry_hash(s, target));
}
} // namespace child

#include "particle_contact_query_reference.inc"

namespace {
constexpr std::uint32_t frame = 0x28002300u, particle = 0x28004300u;
constexpr std::array<std::uint32_t, 12> edge_words{
    0u, 0x80000000u, 0x7fc00017u, 0x7f800019u, 0xffc00031u, 0x7f800000u,
    0xff800000u, 0x00000001u, 0x807fffffu, 0x7f7fffffu, 0x3f800000u, 0x3f200000u};

Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, frame);
    test.policy = index;
    test.abi.fs_base = random();
    test.abi.gp[5] = index % 17u ? particle : frame + 4u;
    for (std::uint32_t address = frame - 32u; address <= frame + 0x144u; address += 4u)
        test.memory.put(address, random());
    test.memory.put(frame + 0x144u, edge_words[index % edge_words.size()]);
    const auto datum = test.abi.gp[5];
    test.memory.put(datum + offsetof(kernel::walk::ParticleDatum, flags), random(), 2);
    for (unsigned i = 0u; i < 12u; i += 4u) {
        test.memory.put(datum + kernel::position + i, edge_words[(index + i + 3u) % edge_words.size()]);
        test.memory.put(datum + kernel::velocity + i, edge_words[(index + i + 2u) % edge_words.size()]);
    }
    test.memory.put(kernel::collision_scale, index % 3u ? 0x3f000000u : 0xffc00063u);
    test.memory.put(kernel::collision_limit, index % 13u ? 0x3f000000u : 0x7f807343u);
    return test;
}

Resume bridge(WorldState &state) {
    return kernel::bridge_query(state, prepared::native_particle_contact_query_reference_2c2d90,
                                [](WorldState &value, std::uint32_t target, std::uint32_t site) {
                                    child::dispatch(value, target, site);
                                });
}

unsigned compare(const Case &test, unsigned at, Totals &totals, std::uint64_t &coverage) {
    Resume mine{}, theirs{};
    const auto native = [&](WorldState &state) {
        mine = bridge(state);
        return mine != Resume::retained;
    };
    const auto source = [&](WorldState &state) {
        theirs = original::native_particle_contact_query_reference_2c2d90_live(state);
    };
    const auto watched = [](std::uint32_t site) { return site >= kernel::first && site <= kernel::last; };
    const auto accesses = fixture::compare_geometry(test, at, totals, native, source, watched);
    if (mine != theirs) throw std::runtime_error("Particle contact sweep resume differential mismatch");
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
    std::mt19937 random(0x2c2fd4u);
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
    if (coverage != 7u) throw std::runtime_error("Particle contact sweep fixture missed a route");
    report(count, totals, coverage, 3u, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_contact_walk.hpp"
#include "rendering/effects/native_tu15_particle_property.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace kernel = halo2::native_particle_contact_walk_detail;
using kernel::Resume;
using kernel::Start;

#include "particle_contact_walk_reference.inc"

namespace {
constexpr std::uint32_t entry_stack = 0x28001300u;
constexpr std::uint32_t local_frame = entry_stack - kernel::local_size - 16u;
constexpr std::uint32_t system_record = 0x28003300u, system_tag = 0x28005300u, definition = 0x28007300u;
constexpr std::uint32_t configuration = 0x28009300u, fixed_inputs = 0x2800b300u, curve = 0x2800d300u;
constexpr std::uint32_t tag_table = 0x28011000u, pool = 0x28013000u, slots = 0xffe00000u;
constexpr std::uint32_t none = 0xffffffffu;
constexpr std::array<std::uint16_t, 5> particle_slots{0xfffeu, 0u, 0xffffu, 0x1001u, 0x8020u};

enum class Entry : unsigned { setup, combined, particle, advance, epilogue };
Entry entry(unsigned index) { return static_cast<Entry>(index % 5u); }

Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    const auto invoke = entry(index);
    random_abi(random, test, invoke == Entry::setup || invoke == Entry::combined ? entry_stack : local_frame);
    test.policy = index;
    test.abi.fs_base = random();
    for (std::uint32_t addr = local_frame - 0x80u; addr != entry_stack + 28u; addr += 4u) {
        test.memory.put(addr, random());
    }
    const auto handle = [](unsigned loc) {
        return 0x6f920000u | particle_slots[loc];
    };
    const auto sequence = index / 5u;
    const unsigned active = sequence % (particle_slots.size() + 1u);
    const auto first = active ? handle(0u) : none;
    test.memory.put(entry_stack + 4u, configuration);
    test.memory.put(entry_stack + 8u, system_record);
    test.memory.put(entry_stack + 12u, first);
    test.memory.put(local_frame + kernel::next_slot, invoke == Entry::particle ? handle(0u) : first);

    test.memory.put(configuration + 8u, fixed_inputs);
    test.memory.put(configuration + 12u, sequence % 4u);
    test.memory.put(configuration + 16u, sequence % 3u ? 0u : 0x10u);
    test.memory.put(system_record + 0xeu, 0u, 2);
    test.memory.put(system_record + 0x10u, 1u);
    test.memory.put(system_record + 4u, 0x3e400000u);
    test.memory.put(0x4f0414u, tag_table);
    test.memory.put(tag_table + 16u, 0x62736474u);
    test.memory.put(tag_table + 24u, system_tag);
    test.memory.put(tag_table + 32u, 0u);
    test.memory.put(system_tag + 0x18u, definition);
    test.memory.put(definition + 4u, 2u);
    test.memory.put(0x252440u, 0x252175u);

    for (std::uint32_t offset : {4u, 0x18u}) {
        test.memory.put(fixed_inputs + offset + 0u, 0u, 2);
        test.memory.put(fixed_inputs + offset + 2u, 1u, 2);
        test.memory.put(fixed_inputs + offset + 6u, 0xffffu, 2);
        test.memory.put(fixed_inputs + offset + 12u, curve);
    }
    test.memory.put(curve, 0u);
    test.memory.put(curve + 4u, 0x3f000000u);
    test.memory.put(curve + 8u, 0x3fc00000u);
    test.memory.put(kernel::location::globals::one, 0x3f800000u);
    test.memory.put(kernel::location::globals::particles, pool);
    test.memory.put(pool + 0x44u, slots);
    constexpr std::array flags{0u, 0x1001u, 8u, 0x2000u, 0xe064u, 4u, 0x6000u};
    constexpr std::array ages{0u, 0x3f400000u, 0x3f800000u, 0x3fc00000u, 0x7fc009b5u, 0x7f801122u,
                              0x00000001u, 0xbf800000u, 0x80000000u};
    for (unsigned i = 0u; i < particle_slots.size(); ++i) {
        const auto address = slots + particle_slots[i] * 64u;
        test.memory.put(address + 2u, flags[(sequence + i) % flags.size()], 2);
        test.memory.put(address + 4u, i + 1u < active ? handle(i + 1u) : none);
        test.memory.put(address + 8u, ages[(sequence + i * 3u) % ages.size()]);
    }
    return test;
}

Resume run_setup(WorldState &state) {
    return kernel::bridge_setup(state, prepared::native_particle_contact_setup_reference_2c2d90,
                                children::call_local);
}

Resume run_walk(WorldState &state, Start start) {
    return kernel::bridge_walk(state, start, prepared::native_particle_contact_walk_reference_2c2d90);
}

unsigned compare(const Case &test, unsigned at, Totals &totals, std::uint64_t &coverage) {
    const auto invoke = entry(test.policy);
    Resume mine{}, theirs{};
    const auto invoke_route = [&](WorldState &state, auto setup, auto walk) {
        switch (invoke) {
        case Entry::setup: return setup(state);
        case Entry::combined: {
            const auto next = setup(state);
            if (next == Resume::retained) return next;
            return walk(state, next == Resume::particle ? Start::particle : Start::epilogue);
        }
        case Entry::particle: return walk(state, Start::particle);
        case Entry::advance: return walk(state, Start::advance);
        case Entry::epilogue: return walk(state, Start::epilogue);
        }
        throw std::logic_error("Unknown contact walk fixture");
    };
    const auto native = [&](WorldState &state) {
        mine = invoke_route(state, run_setup, run_walk);
        return mine != Resume::retained;
    };
    const auto source = [&](WorldState &state) {
        theirs = invoke_route(state, original::native_particle_contact_setup_reference_2c2d90_live,
                              original::native_particle_contact_walk_reference_2c2d90_live);
    };
    const auto watched = [](std::uint32_t site) { return site >= kernel::first && site <= kernel::last; };
    const auto accesses = fixture::compare_geometry(test, at, totals, native, source, watched);
    if (mine != theirs) throw std::runtime_error("Particle contact walk resume differential mismatch");
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
    std::mt19937 random(0x2c2d90u);
    Totals totals;
    std::uint64_t coverage{};
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random, index);
        const auto accesses = compare(test, 0u, totals, coverage);
        if (index < 90u)
            for (unsigned at = 1u; at <= accesses; ++at) compare(test, at, totals, coverage);
    }
    const auto setup = [](WorldState &state) { return run_setup(state) != Resume::retained; };
    const auto walk = [](WorldState &state) { return run_walk(state, Start::particle) != Resume::retained; };
    if (!census_mode) {
        const auto control = make_case(random, count);
        check_refusals(control, setup);
        check_birth_observer_refusals(control, setup);
        check_refusals(control, walk);
        check_birth_observer_refusals(control, walk);
    }
    if (coverage != 0x1fu) throw std::runtime_error("Particle contact walk fixture missed a route");
    report(count, totals, coverage, 5u, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

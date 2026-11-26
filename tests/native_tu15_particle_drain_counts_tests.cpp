#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_drain_counts.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace kernel = halo2::native_particle_drain_counts_detail;
namespace types = halo2::native_effects::location_update;
namespace world = halo2::native_world_leaves_detail;

#include "particle_drain_counts_reference.inc"

namespace {
constexpr std::uint32_t stack = 0x20008a00u;
constexpr std::uint32_t system_address = 0x32002108u;
constexpr std::uint32_t location_array = 0x31001300u, emitter_array = 0x31003700u;
constexpr std::array<std::uint16_t, 5> location_slots{0xfffeu, 0u, 0xffffu, 0x1001u, 0x8020u};
constexpr std::uint32_t none = 0xffffffffu;

std::uint32_t handle(std::mt19937 &random, std::uint16_t slot) {
    return ((0x1000u + random() % 0xe000u) << 16u) | slot;
}

Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, stack);
    test.policy = index;
    test.abi.fs_base = random();
    for (unsigned offset = 0u; offset < 0x28u; offset += 4u) {
        test.memory.put(stack - 0x20u + offset, random());
    }

    const std::uint32_t location_data = (index % 7u == 0u) ? 0xffe00000u : 0x33003800u;
    const std::uint32_t emitter_data = (index % 11u == 0u) ? 0xffd00000u : 0x34004700u;
    test.memory.put(kernel::system::location_array, location_array);
    test.memory.put(types::globals::emitters, emitter_array);
    test.memory.put(location_array + kernel::array_data, location_data);
    test.memory.put(emitter_array + kernel::array_data, emitter_data);

    const unsigned locations = (index / 2u) % (location_slots.size() + 1u);
    test.memory.put(system_address + kernel::system::first_location,
                    locations ? handle(random, location_slots.front()) : none);
    for (unsigned loc = 0u; loc < location_slots.size(); ++loc) {
        const auto address = std::uint32_t(location_data + location_slots[loc] * sizeof(types::LocationDatum));
        if (loc == 0u) test.abi.gp[world::eax] = (index % 2u) ? address : system_address;
        const unsigned emitters = (index / 3u + loc) % 7u;
        const auto emitter_slot = [loc](unsigned number) {
            return std::uint16_t(0xffffu - 7u * loc - number);
        };
        test.memory.put(address + offsetof(types::LocationDatum, first_emitter),
                        emitters ? handle(random, emitter_slot(0u)) : none);
        test.memory.put(address + kernel::system::next_location,
                        loc + 1u < locations ? handle(random, location_slots[loc + 1u]) : none);
        for (unsigned e = 0u; e < emitters; ++e) {
            const auto address = std::uint32_t(emitter_data + emitter_slot(e) * sizeof(types::EmitterDatum));
            const auto count = index % 15u ? random() & 0xffffu : 0xffffu;
            test.memory.put(address + offsetof(types::EmitterDatum, particle_count), count, 2);
            test.memory.put(address + offsetof(types::EmitterDatum, first_particle), 0xdeadbeefu);
            test.memory.put(address + offsetof(types::EmitterDatum, next_emitter),
                            e + 1u < emitters ? handle(random, emitter_slot(e + 1u)) : none);
        }
    }
    return test;
}

bool native_locations(WorldState &state) {
    return kernel::bridge_locations(state, prepared::native_particle_drain_counts_reference_178240,
                                    prepared::run_252060);
}

bool native_emitters(WorldState &state) {
    return kernel::bridge_emitters(state, prepared::native_particle_drain_counts_reference_252060);
}

unsigned compare(const Case &test, unsigned at, Totals &totals, bool reference) {
    const bool emitter = (test.policy % 2u) != 0u;
    const auto native = [emitter](WorldState &state) {
        if (emitter) prepared::run_252060(state);
        else prepared::run_178240(state);
        return true;
    };
    const auto source = emitter ? original::run_252060 : original::run_178240;
    const auto watched = [reference](std::uint32_t site) {
        return !reference && ((site >= kernel::system::drain && site <= kernel::last_locations) ||
                              (site >= kernel::emitter_count && site <= kernel::last_emitters));
    };
    return fixture::compare_geometry(test, at, totals, native, source, watched);
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    fixture::use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x178240u);
    Totals totals;
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random, index);
        const auto accesses = compare(test, 0u, totals, mode == "reference");
        if (index < 54u) {
            for (unsigned at = 1u; at <= accesses; ++at) compare(test, at, totals, mode == "reference");
        }
    }
    if (mode != "census" && mode != "reference") {
        const auto control = make_case(random, count);
        check_refusals(control, native_locations);
        check_birth_observer_refusals(control, native_locations);
        check_refusals(control, native_emitters);
        check_birth_observer_refusals(control, native_emitters);
    }
    report(count, totals, 0u, 0u, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

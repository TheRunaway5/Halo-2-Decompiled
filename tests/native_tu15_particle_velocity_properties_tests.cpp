#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_property.hpp"
#include "rendering/effects/native_tu15_particle_velocity_properties.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace kernel = halo2::native_particle_velocity_properties_detail;

namespace children {
std::uint32_t estimate(std::uint32_t input) {
    log_call(mix(0x252d0au, input));
    if (policy % 17u == 8u)
        throw std::runtime_error("Required original contact RSQRTSS owner unavailable");
    return (input & 0x80000000u) | 0x3f100000u | (input & 0x3ffffu);
}

void dispatch(WorldState &s, std::uint32_t target, std::uint32_t at, bool tail) {
    if (target != 0x13de70u || at != 0x250004u || tail)
        throw std::logic_error("Unexpected authored curve child");
    log_call(entry_hash(s, target));
    s.xmm[0] = _mm_set_ss(static_cast<float>(policy & 7u) * 0.25f);
    if (s.pop() != 0x250009u) throw std::logic_error("Wrong curve child return address");
    s.gp[4] += 8u;
}
} // namespace children

#include "particle_velocity_properties_reference.inc"

namespace {
constexpr std::uint32_t entry_stack = 0x25002300u, phase_frame = entry_stack - 0x28u;
constexpr std::uint32_t particle = 0x25004300u, direction = 0x25006300u, input = 0x25008300u;
constexpr std::uint32_t authored = 0x2500a300u, curve = 0x2500c300u;
constexpr std::array words{0u, 0x80000000u, 0x3f800000u, 0xc0000000u, 0x7fc00412u, 0x7f800015u,
                           0xffc00462u, 0x7f800000u, 0x00000001u, 0x00800000u, 0x7f7fffffu, 0xbf400000u};

Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, index % 3u ? phase_frame : entry_stack);
    test.policy = index;
    test.abi.fs_base = random();
    test.abi.gp[0] = input;
    test.abi.gp[3] = input;
    test.abi.gp[5] = direction;
    test.abi.gp[7] = particle;
    for (std::uint32_t address = phase_frame - 0x60u; address <= entry_stack + 0xcu; address += 4u)
        test.memory.put(address, random());
    test.memory.put(entry_stack + 4u, authored);
    test.memory.put(authored + 0x78u, index % 9u);
    test.memory.put(input, 0x3e800000u);
    test.memory.put(input + 4u, 0x3f000000u);
    for (auto offset : {0x18u, 0x28u, 0x38u}) {
        test.memory.put(authored + offset, 0u, 2);
        test.memory.put(authored + offset + 2u, 1u, 2);
        test.memory.put(authored + offset + 6u, 0xffffu, 2);
        test.memory.put(authored + offset + 12u, curve);
    }
    test.memory.put(curve, 0u);
    test.memory.put(curve + 4u, index % 11u == 0u ? 0u : 0x3e800000u);
    test.memory.put(curve + 8u, index % 11u == 0u ? 0u : 0x3fc00000u);
    for (unsigned axis = 0; axis < 3u; ++axis)
        test.memory.put(direction + 4u * axis, index % 13u ? words[(index + axis) % words.size()] : 0u);
    test.memory.put(kernel::minimum_lifetime, 0x3a83126fu);
    test.memory.put(kernel::rotation_scale, index % 7u ? 0x3f000000u : 0x7fc03712u);
    test.memory.put(kernel::one, 0x3f800000u);
    test.memory.put(kernel::zero, 0u);
    test.memory.put(particle + 0xcu, 0u);
    test.memory.put(particle + 0x24u, 0u);
    test.memory.put(particle + 0x34u, 0u);
    return test;
}

bool prepare(WorldState &s) {
    return kernel::bridge_prefix(s, source::native_particle_velocity_properties_prefix_reference_252480,
                                 children::curve_call);
}

bool finish(WorldState &s, bool store) {
    return kernel::bridge_suffix(s, store,
        [](auto &value, bool first, auto &estimate) {
            source::native_particle_velocity_properties_suffix_reference_252480(value, first, estimate);
        },
        children::curve_call, [](WorldState &, std::uint32_t input) { return children::estimate(input); });
}

unsigned compare(const Case &test, unsigned access, Totals &totals) {
    const auto section = test.policy % 3u;
    const auto native = [section](WorldState &s) { return section ? finish(s, section == 1u) : prepare(s); };
    const auto original = [section](WorldState &s) {
        if (section) {
            auto estimate = children::estimate;
            source::native_particle_velocity_properties_suffix_reference_252480_live(s, section == 1u, estimate);
        } else {
            source::native_particle_velocity_properties_prefix_reference_252480_live(s);
        }
    };
    const auto sites = [](std::uint32_t pc) {
        return (pc >= kernel::entry && pc < kernel::choose_mode) ||
               (pc >= kernel::store_position_z && pc <= kernel::last);
    };
    return fixture::compare_geometry(test, access, totals, native, original, sites);
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    fixture::use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x252480u);
    Totals totals;
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random, index);
        const auto accesses = compare(test, 0u, totals);
        if (index < 63u)
            for (unsigned at = 1u; at <= accesses; ++at) compare(test, at, totals);
    }
    if (!census_mode) {
        const auto control = make_case(random, count);
        const auto tail = [](WorldState &s) { return finish(s, true); };
        check_refusals(control, prepare);
        check_refusals(control, tail);
        check_birth_observer_refusals(control, prepare);
        check_birth_observer_refusals(control, tail);
    }
    report(count, totals, 0u, 0u, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

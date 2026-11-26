#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_property.hpp"
#include "rendering/effects/native_tu15_particle_velocity_direction.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace kernel = halo2::native_particle_velocity_direction_detail;

namespace children {
constexpr std::uint32_t random_service = 0x148de0u, authored_curve = 0x13de70u;
constexpr std::array vector_words{0u, 0x80000000u, 0x3f800000u, 0xc0000000u, 0x7fc00412u, 0x7f800015u,
                                  0xffc00462u, 0x7f800000u, 0x00000001u, 0x00800000u, 0xbf400000u};

void dispatch(WorldState &s, std::uint32_t target, std::uint32_t at, bool tail = false) {
    if (tail) throw std::logic_error("Unexpected particle direction tail");
    log_call(entry_hash(s, target));
    if (target == authored_curve && at == 0x250004u) {
        s.xmm[0] = _mm_set_ss(static_cast<float>(policy & 7u) * 0.25f);
        if (s.pop() != 0x250009u) throw std::logic_error("Wrong curve child return address");
        s.gp[4] += 8u;
    } else if (target == random_service && (at == 0x25263eu || at == 0x25267fu)) {
        const auto address = s.gp[0], rng = s.gp[2];
        const auto seed = r(s.memory, rng, 4);
        w(s.memory, s.observe, target, rng, 4, seed * 1664525u + 1013904223u);
        for (unsigned axis = 0; axis < 3; ++axis)
            w(s.memory, s.observe, target, address + 4u * axis, 4, vector_words[(policy + axis) % vector_words.size()]);
        s.xmm[1] = _mm_set_ss(0.5f);
        s.gp[1] = s.flags.logic(0x1976eeabu, 4);
        if (s.pop() != (at == 0x25263eu ? 0x252643u : 0x252684u))
            throw std::logic_error("Wrong random direction return address");
    } else {
        throw std::logic_error("Unexpected particle direction child");
    }
    log_call(entry_hash(s, target));
}
} // namespace children

#include "particle_velocity_direction_reference.inc"

namespace {
constexpr std::uint32_t frame = 0x250022d8u, particle = 0x25004300u, input = 0x25008300u;
constexpr std::uint32_t authored = 0x2500a300u, curve = 0x2500c300u, rng = 0x2500e300u;

Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, frame);
    test.policy = index;
    test.abi.fs_base = random();
    test.abi.gp[3] = input;
    test.abi.gp[5] = authored;
    test.abi.gp[7] = particle;
    for (std::uint32_t address = frame - 0x60u; address <= frame + 0x30u; address += 4u)
        test.memory.put(address, random());
    test.memory.put(input, 0x3e800000u);
    test.memory.put(input + 4u, 0x3f000000u);
    test.memory.put(authored + 0x7cu, 0u, 2);
    test.memory.put(authored + 0x7eu, 1u, 2);
    test.memory.put(authored + 0x82u, 0xffffu, 2);
    test.memory.put(authored + 0x88u, curve);
    test.memory.put(curve, 0u);
    test.memory.put(curve + 4u, 0x3e800000u);
    test.memory.put(curve + 8u, 0x3fc00000u);
    test.memory.put(0x46a0d0u, 0x3f800000u);
    test.memory.put(kernel::random_state, rng);
    test.memory.put(rng + 4u, random());
    for (unsigned axis = 0; axis < 3; ++axis) {
        test.memory.put(particle + kernel::position + 4u * axis, random());
        test.memory.put(particle + kernel::velocity + 4u * axis, random());
    }
    return test;
}

template<bool Inward>
bool bridge(WorldState &s) {
    const auto replay = Inward ? source::native_particle_velocity_direction_inward_reference_252480
                               : source::native_particle_velocity_direction_outward_reference_252480;
    return kernel::bridge<Inward>(s,replay,children::curve_call,
        [](auto &state,std::uint32_t target,std::uint32_t at) { children::dispatch(state,target,at); });
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    fixture::use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x252628u);
    Totals totals;
    const auto sites = [](std::uint32_t pc) { return pc >= kernel::outward && pc <= kernel::inward_exit; };
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random, index);
        const auto native = index & 1u ? bridge<true> : bridge<false>;
        const auto original = (index & 1u) ? source::native_particle_velocity_direction_inward_reference_252480_live
                                           : source::native_particle_velocity_direction_outward_reference_252480_live;
        const auto accesses = fixture::compare_geometry(test, 0u, totals, native, original, sites);
        if (index < 64u)
            for (unsigned at = 1u; at <= accesses; ++at)
                fixture::compare_geometry(test, at, totals, native, original, sites);
    }
    if (!census_mode) {
        const auto control = make_case(random, count);
        check_refusals(control, bridge<true>);
        check_refusals(control, bridge<false>);
        check_birth_observer_refusals(control, bridge<true>);
        check_birth_observer_refusals(control, bridge<false>);
    }
    report(count, totals, 0u, 0u, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

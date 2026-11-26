#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_property.hpp"
#include "rendering/effects/native_tu15_particle_velocity_angles.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace kernel = halo2::native_particle_velocity_angles_detail;

namespace children {
void dispatch(WorldState &s, std::uint32_t target, std::uint32_t at, bool tail) {
    if (target != 0x13de70u || at != 0x250004u || tail)
        throw std::logic_error("Unexpected authored angle curve child");
    log_call(entry_hash(s, target));
    constexpr std::array curve_words{0u, 0x3f400000u, 0x3f800000u, 0x40000000u};
    s.xmm[0] = _mm_set_ss(std::bit_cast<float>(curve_words[(policy / 2u) % curve_words.size()]));
    if (s.pop() != 0x250009u) throw std::logic_error("Wrong curve child return address");
    s.gp[4] += 8u;
    log_call(entry_hash(s, target));
}
} // namespace children

#include "particle_velocity_angles_reference.inc"

namespace {
constexpr std::uint32_t frame = 0x25002300u, particle = 0x25004300u, input = 0x25006300u;
constexpr std::uint32_t authored = 0x25008300u, curve = 0x2500a300u, seed = 0x2500c300u;

Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, frame);
    test.policy = index;
    test.abi.fs_base = random();
    test.abi.gp[3] = input;
    test.abi.gp[5] = authored;
    test.abi.gp[7] = particle;
    for (std::uint32_t addr = frame - 0x60u; addr <= frame + 0x38u; addr += 4u)
        test.memory.put(addr, random());
    test.memory.put(input, 0x3f000000u);
    test.memory.put(input + 4u, 0x3f400000u);
    for (auto offset : {0x7cu, 0x8cu}) {
        test.memory.put(authored + offset, 0u, 2);
        test.memory.put(authored + offset + 2u, 1u, 2);
        test.memory.put(authored + offset + 6u, 0xffffu, 2);
        test.memory.put(authored + offset + 12u, curve);
    }
    test.memory.put(curve, 0u);
    test.memory.put(curve + 4u, 0x3e800000u);
    test.memory.put(curve + 8u, 0x3fc00000u);
    test.memory.put(0x46a0d0u, 0x3f800000u);
    test.memory.put(kernel::random_state, seed);
    test.memory.put(seed + 4u, random());
    test.memory.put(kernel::unsigned_adjustment, 0x4f800000u);
    test.memory.put(kernel::random_scale, 0x37800000u);
    test.memory.put(kernel::angle_scale, index % 17u ? 0x40c90fdbu : 0x7f800000u);
    test.memory.put(kernel::spread_scale, 0x3f800000u);
    for (unsigned offset = 0u; offset < 0x40u; offset += 4u)
        test.memory.put(particle + offset, random());
    if (index % 19u < 2u) {
        test.abi.x87_depth = (index % 19u == 0u ? 7u : 6u);
        for (unsigned lane = 0u; lane < test.abi.x87_depth; ++lane)
            test.abi.x87[lane] = (static_cast<long double>(lane) + 0.25L) * -1.0L;
    }
    return test;
}

template<bool Planar>
bool bridge(WorldState &s) {
    const auto replay = Planar ? source::native_particle_velocity_angles_planar_reference_252480
                               : source::native_particle_velocity_angles_spatial_reference_252480;
    return kernel::bridge<Planar>(s,replay,children::curve_call,
        [](long double value,bool cosine,std::uint16_t &status) {
            return cosine ? source::native_fcos(value,status) : source::native_fsin(value,status);
        }, children::native_single_spill);
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    fixture::use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x2524d4u);
    Totals totals;
    const auto sites = [](std::uint32_t pc) { return pc >= kernel::spatial && pc <= kernel::planar_exit; };
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random, index);
        const auto native = index & 1u ? bridge<true> : bridge<false>;
        const auto original = (index & 1u) ? source::native_particle_velocity_angles_planar_reference_252480_live
                                           : source::native_particle_velocity_angles_spatial_reference_252480_live;
        const auto accesses = fixture::compare_geometry(test, 0u, totals, native, original, sites);
        if (index < 64u)
            for (unsigned at = 1u; at <= accesses; ++at) fixture::compare_geometry(test, at, totals, native, original, sites);
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

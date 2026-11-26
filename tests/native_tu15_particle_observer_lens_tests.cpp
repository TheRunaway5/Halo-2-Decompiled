#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_observer_lens.hpp"

#include "particle_observer_lens_reference.inc"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace lens = halo2::native_particle_observer_lens_detail;
namespace observer = particle_observer_lens_source;

namespace {
constexpr std::uint32_t frame = 0x2a003300u, packet = 0x2a004300u, axis = 0x44ac40u;
constexpr std::uint32_t return_address = 0x172200u;
constexpr std::array float_words{0u, 0x80000000u, 0x3f000000u, 0xbf000000u, 0x3f800000u, 0xbf800000u,
                                 0x7fc09713u, 0xffc08133u, 0x7f804225u, 0x0000000fu, 0x80800000u, 0x7f7fffffu,
                                 0x7f800000u, 0xff800000u};

Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, frame);
    test.policy = index;
    test.abi.fs_base = random();
    test.abi.gp[6] = index % 29u ? packet : frame - 0x5cu;
    auto &memory = test.memory;
    for (std::uint32_t address = frame - 0x100u; address <= frame + 0x54u; address += 4u)
        memory.put(address, random());
    for (unsigned offset = 0; offset <= 0x50u; offset += 4u)
        memory.put(test.abi.gp[6] + offset, random());
    constexpr std::array authored{0x3f800000u, 0u, 0u, 0u, 0u, 0x3f800000u,
                                   0x3fb2b8c2u, 0x3faaaaabu, 0u, 0u};
    for (unsigned lane = 0; lane < authored.size(); ++lane) {
        const auto finite = u(static_cast<float>(int(random() % 2001u) - 1000) / 127.0f);
        const auto special = float_words[(index + lane) % float_words.size()];
        const auto value = index % 4u == 0u ? authored[lane] : index % 4u == 1u ? special : finite;
        memory.put(test.abi.gp[6] + lens::forward + 4u * lane, value);
    }
    memory.put(frame,return_address);
    const auto axis_address = index % 23u ? axis : test.abi.gp[6] + lens::forward;
    memory.put(lens::reference_axis, axis_address);
    memory.put(axis_address, 0u);
    memory.put(axis_address + 4u, index % 6u ? 0u : 0x7fc01111u);
    memory.put(axis_address + 8u, 0x3f800000u);
    memory.put(lens::lens_divisor, index % 7u ? 0x3fb2b8c2u : float_words[index % float_words.size()]);
    memory.put(0x46a0ccu, 0x3f000000u);
    memory.put(0x46a0d0u, 0x3f800000u);
    memory.put(0x46a1d0u, 0u);
    memory.put(0x46a1d4u, 0x3ff00000u);
    memory.put(0x46a0ecu, 0x38d1b717u);
    if (index % 19u == 0u) {
        test.abi.x87_depth = (index % 38u == 0u ? 7u : 6u);
        for (unsigned lane = 0u; lane < test.abi.x87_depth; ++lane)
            test.abi.x87[lane] = -(static_cast<long double>(lane) + 0.25L);
    }
    return test;
}

bool native(WorldState &s) {
    const auto trig = [](lens::Trig op, long double value, std::uint16_t &status) {
        if (op == lens::Trig::sine) return observer::native_fsin(value,status);
        if (op == lens::Trig::cosine) return observer::native_fcos(value,status);
        return observer::native_fptan(value,status);
    };
    return lens::bridge(s,observer::native_particle_observer_lens_reference_174bf0,
                        trig,observer::native_fpatan,observer::native_single_spill);
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    fixture::use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x174bf0u);
    Totals totals;
    unsigned normal_returns = 0u;
    const auto sites = [](std::uint32_t pc) { return pc >= lens::entry && pc <= lens::last; };
    const auto source = observer::native_particle_observer_lens_reference_174bf0;
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random, index);
        const auto faults_before = totals.faults;
        const auto accesses = fixture::compare_geometry(test,0u,totals,native,source,sites);
        normal_returns += totals.faults == faults_before;
        if (index < 64u)
            for (unsigned at = 1u; at <= accesses; ++at)
                fixture::compare_geometry(test,at,totals,native,source,sites);
    }
    if (!census_mode) {
        const auto control = make_case(random, count);
        check_refusals(control,native);
        check_birth_observer_refusals(control,native);
    }
    std::fprintf(stderr, "Particle observer lens fixture returns: %u\n", normal_returns);
    report(count, totals, 0u, 0u, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

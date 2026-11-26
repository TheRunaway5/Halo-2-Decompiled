#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_location_prepare.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace kernel = halo2::native_particle_location_prepare_detail;

namespace children {
constexpr std::uint32_t allocation = 0x6f9a0007u;
constexpr std::uint32_t no_location = 0xffffffffu;

static void run_251930(WorldState &);
static void run_2520a0(WorldState &);

void run_252100(WorldState &s) {
    log_call(entry_hash(s, 0x252100u));
    s.xmm[0] = _mm_set_ss(static_cast<float>(s.gp[0] & 0x1fu) * 0.125f);
    if (s.pop() != 0x176a91u) throw std::logic_error("Wrong enumerator continuation");
}

void run_251c80(WorldState &s) {
    log_call(entry_hash(s, 0x251c80u));
    for (unsigned offset = 4u; offset <= 20u; offset += 4u)
        log_call(r(s.memory, s.gp[4] + offset, 4));
    const auto location = r(s.memory, s.gp[4] + 4u, 4);
    if (location != s.gp[6]) throw std::logic_error("Wrong emitter location argument");
    w(s.memory, s.observe, 0x251c80u, location + 0xcu, 4, 0x7c170011u | policy);
    s.set(0, 0xc07f0020u, 4, 0);
    s.set(2, s.flags.logic(0x279df0u, 4), 4, 0);
    if (s.pop() != 0x178225u) throw std::logic_error("Wrong emitter continuation");
    s.gp[4] += 20u;
}

void dispatch(WorldState &s, std::uint32_t target, std::uint32_t site, bool tail = false) {
    if (tail) throw std::logic_error("Unexpected particle location preparation tail");
    log_call(entry_hash(s, target));
    if (target == 0x16e6a0u && site == 0x251937u) {
        s.gp[0] = policy % 6u == 2u ? no_location : allocation;
        if (s.pop() != 0x25193cu) throw std::logic_error("Wrong datum allocation continuation");
    } else if (target == 0x251930u && site == 0x17813eu) run_251930(s);
    else if (target == 0x2520a0u && site == 0x17816bu) run_2520a0(s);
    else if (target == 0x251c80u && site == 0x178220u) run_251c80(s);
    else throw std::logic_error("Unexpected particle location preparation route");
}
} // namespace children

#include "particle_location_prepare_reference.inc"

namespace {
constexpr std::uint32_t frame = 0x28002300u, system_record = 0x28004300u, request = 0x28006300u, pose = 0x28008300u;
constexpr std::uint32_t pool = 0x2800a300u, slots = 0x2800c300u, coordinate = 0x2800e300u, seed = 0x28010300u;
constexpr std::uint32_t existing = 0x6f9a0042u;
constexpr std::array words{0u, 0x80000000u, 0x3f800000u, 0xbf800000u, 0x7fc00047u, 0x7f800018u, 0xffc00019u,
                           0x7f800000u, 0xff800000u, 0x00000001u, 0x00800000u, 0x7f7fffffu};

Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, frame);
    test.policy = index;
    test.abi.fs_base = random();
    test.abi.gp[3] = system_record;
    for (std::uint32_t address = frame - 0xd0u; address <= frame + 0x1cu; address += 4u)
        test.memory.put(address, random());
    test.memory.put(frame + 4u, request);
    test.memory.put(frame + 8u, pose);
    test.memory.put(frame + 12u, index ^ 0x3c90u);
    test.memory.put(system_record + 0xcu, random(), 2);
    test.memory.put(system_record + 0x8u, words[(index + 4u) % words.size()]);
    test.memory.put(system_record + 0x30u, 0xffffffffu);
    test.memory.put(system_record + 0x34u, 0xffffffffu);
    test.memory.put(request, words[index % words.size()]);
    test.memory.put(request + 4u, words[(index + 2u) % words.size()]);
    test.memory.put(request + 8u, index % 3u ? 0xffffffffu : existing);
    test.memory.put(kernel::system::location_array, pool);
    test.memory.put(pool + kernel::array_data, slots);
    for (auto handle : {children::allocation, existing}) {
        const auto location = slots + (handle & 0xffffu) * 0x34u;
        test.memory.put(location, handle >> 16u, 2);
        test.memory.put(location + 0xcu, 0xffffffffu);
    }
    for (unsigned axis = 0u; axis < 3u; ++axis) {
        test.memory.put(pose + 0x28u + 4u * axis, words[(index + axis) % words.size()]);
        test.memory.put(coordinate + 4u * axis, words[(index + axis + 1u) % words.size()]);
    }
    test.memory.put(0x474ce8u, coordinate);
    test.memory.put(0x4f3cd8u, seed);
    test.memory.put(seed + 4u, random());
    test.memory.put(0x4c6854u, random());
    test.memory.put(0x46a0ccu, 0x3f000000u);
    test.memory.put(0x46a0d0u, 0x3f800000u);
    return test;
}

unsigned compare(const Case &test, unsigned at, Totals &totals, bool frame_owner, bool reference) {
    const auto prepared = frame_owner ? frame_copy::run_178110 : death_copy::run_178110;
    const auto source = frame_owner ? frame_copy::native_particle_location_prepare_reference_178110_live
                                    : death_copy::native_particle_location_prepare_reference_178110_live;
    const auto native = [prepared](WorldState &s) { prepared(s); return true; };
    const auto watched = [reference](std::uint32_t site) {
        return !reference && site >= kernel::first && site <= kernel::last;
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
    std::mt19937 random(0x178110u);
    Totals totals;
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random, index);
        for (bool frame_owner : {false, true}) {
            const auto accesses = compare(test, 0u, totals, frame_owner, mode == "reference");
            if (index < 48u)
                for (unsigned at = 1u; at <= accesses; ++at)
                    compare(test, at, totals, frame_owner, mode == "reference");
        }
    }
    if (!census_mode && mode != "reference") {
        const auto control = make_case(random, count);
        check_refusals(control, death_copy::attempt_native);
        check_refusals(control, frame_copy::attempt_native);
        check_birth_observer_refusals(control, death_copy::attempt_native);
        check_birth_observer_refusals(control, frame_copy::attempt_native);
    }
    report(count, totals, 0u, 0u, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

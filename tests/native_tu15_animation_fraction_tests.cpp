#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_animation_fraction.hpp"

using namespace frame_misc_harness;
namespace fraction = halo2::native_animation_fraction_detail;
namespace {
constexpr std::uint32_t state = 0x10000000u;
constexpr std::uint32_t later_state = 0x10000100u;
constexpr std::uint32_t instances = 0x10100000u;
constexpr std::uint32_t animation = 0x10200000u;
constexpr std::uint32_t scratch = 0x10300000u;

void dispatch_child(WorldState &s, std::uint32_t target, std::uint32_t site, bool tail) {
    const bool lookup = target == 0x1e3150u && site == 0x1ce8f1u && !tail;
    const bool clamp = target == 0x1ce810u && tail && (site == 0x1ce928u || site == 0x1ce948u);
    if (!lookup && !clamp) throw std::runtime_error("Unexpected animation fraction dispatch");
    auto hash = mix(entry_hash(s, target), site);
    hash = mix(hash, r(s.memory, s.gp[4], 4));
    hash = mix(hash, r(s.memory, s.gp[4] + 4u, 4));
    log_call(hash);
    w(s.memory, s.observe, site, scratch, 4, std::uint32_t(hash));

    std::mt19937_64 random(hash);
    s.gp[0] = lookup ? animation : std::uint32_t(random());
    s.gp[1] = std::uint32_t(random());
    s.gp[2] = std::uint32_t(random());
    s.flags.load(std::uint32_t(random()) & 0x8d5u);
    std::array<std::uint32_t, 4> lanes;
    for (auto &lane : lanes) lane = std::uint32_t(random());
    std::memcpy(&s.xmm[0], lanes.data(), 16);
    s.gp[4] += 8u;
    if (lookup && (policy & 2u)) {
        s.gp[6] = later_state;
        s.gp[4] += 4u;
    }
}
} // namespace

namespace original {
std::uint32_t selected(std::uint32_t address, SourcePhysicsContactImage) { return address; }
void dispatch(WorldState &s, std::uint32_t target, std::uint32_t site, bool tail) {
    dispatch_child(s, target, site, tail);
}
#include "body.inc"
#include "reference.inc"
} // namespace original

namespace {
Case author(std::mt19937 &random, unsigned n) {
    Case result;
    auto &memory = result.memory;
    const auto stack = 0x20010020u + 4u * std::uint32_t(random() % 32);
    random_abi(random, result, stack);
    result.abi.gp[6] = state;

    const std::uint32_t tag_index = std::uint32_t(random() % 512);
    memory.put(state, (std::uint32_t(random()) & 0xffff0000u) | tag_index);
    memory.put(state + 4u, n % 7 == 0 ? 0xffff0000u : std::uint32_t(random() % 32) << 16);
    memory.put(0x4f0414u, instances);
    memory.put(instances + 16u * tag_index + 8u, std::uint32_t(random()));
    const std::int16_t count = n % 11 == 0 ? -3 : std::int16_t(1 + random() % 64);
    memory.put(animation + 0x14u, std::uint16_t(count), 2);
    memory.put(0x14u, std::uint16_t(count), 2);
    memory.put(scratch, 0);
    for (std::uint32_t address = stack - 0x10u; address < stack + 0x10u; address += 4)
        memory.put(address, std::uint32_t(random()));

    std::mt19937_64 wide(random());
    const auto margin = n == 1 ? 0x7f800001u
        : n % 19 == 0 ? special_float(wide) : std::bit_cast<std::uint32_t>(0.001f);
    memory.put(0x46a0ecu, margin);
    constexpr float positions[]{-2.0f, 0.0f, 0.25f, 0.5f, 1.0f, 1.25f};
    // When both MULSS inputs are NaNs, the quieted margin owns the payload.
    const auto frame = n == 1 ? 0xffc00000u : n % 5 == 0 ? special_float(wide)
        : std::bit_cast<std::uint32_t>(positions[n % std::size(positions)]);
    memory.put(stack + 4u, frame);
    memory.put(stack + 8u, frame);

    if (n % 13 == 0) {
        const std::uint32_t missing[]{state, instances + 16u * tag_index + 8u, animation, 0x46a0ecu, stack};
        memory.drop(missing[random() % std::size(missing)]);
    }
    return result;
}

bool native(WorldState &s) {
    return fraction::bridge(s, original::native_frame_misc_reference_1ce8d0, dispatch_child);
}
} // namespace

int main(int argc, char **argv) try {
    const unsigned cases = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode, "HALO2_VERIFY_ANIMATION_FRACTION");
    std::mt19937 random(0x1ce8d0u + (census_mode ? 1u : 0u) + (mode == "verify" ? 2u : 0u));
    Totals totals;
    for (unsigned n = 0; n < cases; ++n) {
        if (!compare(n, author(random, n), native, original::run_1ce8d0, totals)) return 1;
    }
    if (!census_mode) check_refusals(author(random, 1), native);
    report(cases, totals, fraction::coverage().load(), unsigned(fraction::branch_count), mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

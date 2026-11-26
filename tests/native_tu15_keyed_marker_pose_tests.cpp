#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_keyed_marker_pose.hpp"

using namespace frame_misc_harness;
namespace pose = halo2::native_keyed_marker_pose_detail;
namespace {
constexpr std::uint32_t object_array = 0x10000000u;
constexpr std::uint32_t header_rows = 0x10100000u;
constexpr std::uint32_t unit = 0x10301040u;
constexpr std::uint32_t scratch = 0x10500000u;

void blend_owner(WorldState &s) {
    constexpr std::uint32_t target = 0x1e1620u;
    auto hash = mix(entry_hash(s, target), r(s.memory, s.gp[4], 4));
    hash = mix(hash, r(s.memory, s.gp[0], 4));
    const auto fraction = r(s.memory, s.gp[0] + 4u, 4);
    hash = mix(hash, fraction);
    log_call(hash);
    w(s.memory, s.observe, target, scratch, 4, std::uint32_t(hash));

    const std::array<std::uint32_t, 4> lanes{fraction, std::uint32_t(hash),
        std::uint32_t(hash >> 32u), std::uint32_t(hash ^ (hash >> 33u))};
    std::memcpy(&s.xmm[0], lanes.data(), 16);
    s.gp[0] = std::uint32_t(hash);
    s.gp[1] = std::uint32_t(hash >> 32u);
    s.gp[2] = std::uint32_t(hash ^ (hash >> 33u));
    if ((policy & 2u) && (hash & 7u) == 0) {
        s.gp[3] ^= std::uint32_t(hash);
        s.gp[5] ^= std::uint32_t(hash >> 32u);
        s.gp[6] ^= std::uint32_t(hash);
        s.gp[7] = std::uint32_t(hash >> 16u);
    }
    s.flags.load(std::uint32_t(hash) & 0x8d5u);
    s.gp[4] += (policy & 1u) ? 8u : 4u;
}

void marker_owner(WorldState &s) {
    constexpr std::uint32_t target = 0x116060u;
    auto hash = entry_hash(s, target);
    for (unsigned slot = 0; slot <= 8; ++slot)
        hash = mix(hash, r(s.memory, s.gp[4] + 4u * slot, 4));
    const auto iterator = r(s.memory, s.gp[4] + 0x18u, 4);
    for (unsigned word = 0; word < 5; ++word)
        hash = mix(hash, r(s.memory, iterator + 4u * word, 4));
    log_call(hash);
    w(s.memory, s.observe, target, scratch + 4u, 4, std::uint32_t(hash));
    s.gp[0] = std::uint32_t(hash);
    s.gp[1] = std::uint32_t(hash >> 32u);
    s.flags.load(std::uint32_t(hash) & 0x8d5u);
    s.gp[4] += (policy & 2u) ? 40u : 36u;
}
} // namespace

namespace original {
void run_1e1620(WorldState &s) { blend_owner(s); }
void run_116060(WorldState &s) { marker_owner(s); }
#include "body.inc"
#include "reference.inc"
} // namespace original

namespace {
Case author(std::mt19937 &random, unsigned n) {
    Case result;
    auto &memory = result.memory;
    const auto stack = 0x20010070u + 4u * std::uint32_t(random() % 64);
    random_abi(random, result, stack);
    if (n % 19 == 0) result.abi.x87_depth = 7;
    const auto index = std::uint32_t(random() % 512);
    result.abi.gp[3] = (result.abi.gp[3] & 0xffff0000u) | index;

    const auto state_offset = n % 2 == 0 ? 0x400 : -0x110;
    const auto channels_offset = n % 3 == 0 ? 0x580 : -0x300;
    const auto state = unit + std::uint32_t(state_offset);
    const auto transition = unit + std::uint32_t(channels_offset) + 0x80u;
    const auto shape = n % 16;
    memory.put(0x4ecbd0u, object_array);
    memory.put(object_array + 0x44u, header_rows);
    memory.put(header_rows + 12u * index + 8u, unit);
    memory.put(unit + 0x33eu, std::uint16_t(channels_offset), 2);
    memory.put(unit + 0x12au, std::uint16_t(state_offset), 2);
    memory.put(state + 0x68u, shape == 0 ? 0xffffffffu : std::uint32_t(random()));
    memory.put(state, shape == 1 ? 0xffffffffu : std::uint32_t(random()));
    memory.put(state + 6u, shape == 2 ? 0xffffu : std::uint32_t(random() % 128), 2);
    memory.put(transition, std::uint32_t(random()) | 0x0100u);
    if (shape == 4) memory.put(transition + 1u, 0u, 1);

    std::mt19937_64 wide(random());
    const auto bits = [](float value) { return std::bit_cast<std::uint32_t>(value); };
    const auto fraction = shape == 5 ? bits(0.125f)
        : shape == 6 ? bits(-0.25f)
        : shape == 7 ? bits(-0.5f)
        : shape == 8 ? 0x7f800001u
        : n % 3 == 0 ? special_float(wide)
        : bits(float(int(random() % 100) - 50) / 10.0f);
    memory.put(transition + 4u, fraction);
    memory.put(0x46a0ecu, n % 17 == 0 ? special_float(wide) : bits(0.25f));
    memory.put(scratch, 0u);
    for (std::uint32_t address = stack - 0x50u; address < stack + 0x20u; address += 4u)
        memory.put(address, std::uint32_t(random()));
    memory.put(stack + 4u, shape == 3 ? 0xffffffffu : std::uint32_t(random()));
    if (shape == 15) {
        const std::uint32_t missing[]{0x4ecbd0u, object_array, header_rows + 12u * index, unit, state,
            transition, 0x46a0ecu, stack, scratch};
        memory.drop(missing[random() % std::size(missing)]);
    }
    return result;
}

bool native(WorldState &s) {
    return pose::bridge(s, original::native_frame_misc_reference_116280, blend_owner, marker_owner);
}
} // namespace

int main(int argc, char **argv) try {
    const unsigned cases = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode, "HALO2_VERIFY_KEYED_MARKER_POSE");
    std::mt19937 random(0x116280u + (census_mode ? 1u : 0u) + (mode == "verify" ? 2u : 0u));
    Totals totals;
    for (unsigned n = 0; n < cases; ++n) {
        if (!compare(n, author(random, n), native, original::run_116280, totals)) return 1;
    }
    if (!census_mode) check_refusals(author(random, 1), native);
    report(cases, totals, pose::coverage().load(), unsigned(pose::branch_count), mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

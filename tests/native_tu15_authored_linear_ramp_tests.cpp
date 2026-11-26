#include "native_tu15_frame_misc_harness.hpp"
#include "core/native_tu15_authored_linear_ramp.hpp"

using namespace frame_misc_harness;
namespace authored = halo2::native_authored_linear_ramp;

namespace original {
[[maybe_unused]] std::uint32_t selected(std::uint32_t address, SourcePhysicsContactImage) { return address; }
#include "cases.inc"
} // namespace original

namespace {
constexpr const char *reference_env = "HALO2_REFERENCE_AUTHORED_LINEAR_RAMP";
constexpr const char *verify_env = "HALO2_VERIFY_AUTHORED_LINEAR_RAMP";
constexpr std::uint32_t unit_address = 0x46a0d0u;
constexpr std::uint32_t one = 0x3f800000u;
constexpr unsigned shape_count = 18;

bool force_reference{};
std::uint64_t fallback_calls{};

auto reference_for(const WorldState &s) {
    return s.gp[1] == 4u ? original::native_frame_misc_reference_13de70_linear
                          : original::native_frame_misc_reference_13de70_ramp;
}

void reference(WorldState &s) { reference_for(s)(s); }

bool native(WorldState &s) {
    const auto kind = s.gp[1] == 4u ? authored::Kind::linear : authored::Kind::ramp;
    if (authored::bridge(s, kind, reference_for(s))) return true;
    if (!force_reference) return false;

    auto *cursor = s.instruction_cursor ? s.instruction_cursor->position : nullptr;
    auto &services = const_cast<SourcePhysicsWorldRuntimeServices &>(s.services);
    auto expected = std::uint32_t(kind);
    services.instruction = [cursor, expected](std::uint32_t pc) mutable {
        if (expected && pc != expected) throw std::runtime_error("Authored case resumed at a different instruction");
        expected = 0;
        if (cursor) *cursor = pc;
    };
    ++fallback_calls;
    reference(s);
    return true;
}

Case fixture(std::mt19937 &random, unsigned n) {
    Case result;
    const bool linear = (n & 1u) == 0;
    const auto shape = n / 2u % shape_count;
    const auto sp = 0x20010000u + (shape == 9 ? 0xff0u : shape == 10 ? 0xfdcu : 0x100u);
    auto data = 0x10300000u + (shape == 6 ? 0xff0u : shape == 7 ? 0xff8u
        : shape == 11 ? 0xfc0u : shape == 12 ? 0xf80u : 0x120u);
    // The first result store aliases a read from the second layer.
    if (shape == 13) data = sp + 0xcu - (linear ? 0x28u : 0x84u);
    const auto blob = shape == 8 ? 0x10100020u : data - 0x14u;
    random_abi(random, result, sp);
    result.abi.gp[1] = linear ? 4u : 5u;
    result.abi.gp[3] = blob;
    result.abi.gp[6] = data;
    result.abi.xmm[5] = {};

    auto &memory = result.memory;
    const auto blend_enabled = shape == 0 ? 0u : shape < 16 ? 1u : std::uint32_t(random() & 1u);
    memory.put(blob + 1u, (std::uint32_t(random()) & 0xfeu) | blend_enabled, 1);
    memory.put(unit_address, one);
    memory.put(sp + 0xcu, 0xabc00defu);
    memory.put(sp + 0x20u, shape == 2 ? 0xbf800000u : shape == 3 ? 0x40000000u : 0x3f000000u);
    memory.put(sp + 0x24u, 0x3e800000u);
    for (unsigned offset = 0x10; offset <= (linear ? 0x2cu : 0x9cu); offset += 4u)
        memory.put(data + offset, linear ? 0x3f400000u : 0u);
    if (!linear) {
        for (unsigned layer : {0u, 0x50u}) {
            for (unsigned slope : {0x34u, 0x38u, 0x3cu})
                memory.put(data + layer + slope, one);
            memory.put(data + layer + 0x40u, 0x3e000000u);
            memory.put(data + layer + 0x44u, 0x3e800000u);
            memory.put(data + layer + 0x48u, 0x3f000000u);
            memory.put(data + layer + 0x4cu, 0x3f400000u);
        }
    }
    if (shape == 4) {
        // Distinct payloads distinguish both operands of a scalar multiply or add.
        memory.put(sp + 0x20u, 0x7f800001u);
        memory.put(data + (linear ? 0x10u : 0x24u), 0xffc00010u);
        memory.put(data + (linear ? 0x14u : 0x34u), 0x7fc00020u);
    }
    if (shape == 5) memory.put(unit_address, 0x7fc00100u);
    if (shape >= 15) {
        std::mt19937_64 wide(random());
        for (unsigned offset = 0x10u; offset <= (linear ? 0x2cu : 0x9cu); offset += 4u)
            memory.put(data + offset, random() % 3u ? special_float(wide) : std::uint32_t(random()));
        memory.put(sp + 0x20u, special_float(wide));
        memory.put(sp + 0x24u, special_float(wide));
        if (shape == 17) result.abi.xmm[5][0] = special_float(wide);
    }
    if (shape == 6 || shape == 7) memory.drop(data + (shape == 6 ? 0x18u : 0x10u));
    if (shape == 8) memory.drop(blob + 1u);
    if (shape == 9) memory.drop(sp + 0xcu);
    if (shape == 10) memory.drop(sp + 0x24u);
    if (shape == 11 || shape == 12) memory.drop(data + (shape == 11 ? 0x40u : 0x80u));
    if (shape == 14) memory.drop(unit_address);
    return result;
}

bool same_fault_abi(unsigned n, const Case &c) {
    auto left = c.memory.clone();
    const auto baseline = execute(left, c, false, native, reference);
    if (baseline.fault.empty()) return true;
    auto right = c.memory.clone();
    const auto saved_fallbacks = fallback_calls;
    const auto candidate = execute(right, c, true, native, reference);
    fallback_calls = saved_fallbacks;
    if (same_abi(candidate.abi, baseline.abi)) return true;
    std::fprintf(stderr, "case %u differs in registers at fault %x\n", n, baseline.cursor);
    return false;
}
} // namespace

int main(int argc, char **argv) try {
    const unsigned cases = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode, verify_env);
    force_reference = mode == "reference";
    if (force_reference) setenv(reference_env, "1", 1);
    std::mt19937 random(0x13e146u + (census_mode ? 1u : 0u) + (mode == "verify" ? 2u : 0u));
    Totals totals;
    for (unsigned n = 0; n < cases; ++n) {
        const auto sample = fixture(random, n);
        if (!compare(n, sample, native, reference, totals) || !same_fault_abi(n, sample)) return 1;
    }
    if (!census_mode && !force_reference) check_refusals(fixture(random, 0), native);
    const auto coverage = authored::coverage().load();
    if (force_reference && (fallback_calls != cases || coverage))
        throw std::runtime_error("Force-reference did not execute the original authored cases");
    if (!force_reference && cases >= shape_count * 2u && coverage != (1ull << authored::branch_count) - 1u)
        throw std::runtime_error("Authored case branch coverage is incomplete");
    report(cases, totals, coverage, authored::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

#include "native_tu15_frame_misc_harness.hpp"
#include "core/native_tu15_authored_cubic.hpp"

using namespace frame_misc_harness;
namespace authored = halo2::native_authored_cubic;

namespace original {
[[maybe_unused]] std::uint32_t selected(std::uint32_t address, SourcePhysicsContactImage) { return address; }
#include "cases.inc"
} // namespace original

namespace {
constexpr const char *reference_env = "HALO2_REFERENCE_AUTHORED_CUBIC";
constexpr const char *verify_env = "HALO2_VERIFY_AUTHORED_CUBIC";
constexpr unsigned shape_count = 14;
constexpr std::uint32_t unit_address = 0x46a0d0u;
using authored::Continuation;
using authored::Entry;

Entry active_entry() {
    constexpr std::array entries{Entry::base, Entry::blend, Entry::refresh, Entry::input};
    return entries[policy];
}

Continuation baseline_route{}, native_route{};
std::uint64_t fallbacks{};
bool force_reference{};

auto reference_begin(Entry entry) {
    return entry == Entry::input ? original::native_frame_misc_reference_13de70_cubic_input
                                 : original::native_frame_misc_reference_13de70_cubic_base;
}

auto reference_blend(Entry entry) {
    return entry == Entry::refresh ? original::native_frame_misc_reference_13de70_cubic_refresh
                                   : original::native_frame_misc_reference_13de70_cubic_blend;
}

void reference(WorldState &state) {
    const auto entry = active_entry();
    if (entry == Entry::base || entry == Entry::input) baseline_route = reference_begin(entry)(state);
    else reference_blend(entry)(state);
}

bool native(WorldState &state) {
    const auto entry = active_entry();
    if (entry == Entry::base || entry == Entry::input) {
        native_route = authored::begin(state, entry, reference_begin(entry));
        if (native_route != Continuation::retained) return true;
    } else if (authored::blend(state, entry, reference_blend(entry))) return true;
    if (!force_reference) return false;

    auto *cursor = state.instruction_cursor ? state.instruction_cursor->position : nullptr;
    auto &services = const_cast<SourcePhysicsWorldRuntimeServices &>(state.services);
    services.instruction = [cursor, expected = std::uint32_t(entry)](std::uint32_t pc) mutable {
        if (expected && pc != expected) throw std::runtime_error("Cubic resumed at the wrong original instruction");
        expected = 0;
        if (cursor) *cursor = pc;
    };
    ++fallbacks;
    if (entry == Entry::base || entry == Entry::input) native_route = reference_begin(entry)(state);
    else reference_blend(entry)(state);
    return true;
}

Case fixture(std::mt19937 &random, unsigned n) {
    Case result;
    const auto shape = n / 4u % shape_count;
    const auto sp = 0x20200000u + (shape == 10 ? 0xff0u : shape == 11 ? 0xfe0u : 0x100u);
    const auto blob = shape == 4 ? sp + 0xcu : 0x10300000u + (shape == 5 ? 0xfffu
        : shape == 6 ? 0xfc4u : shape == 7 ? 0xf94u : 0x120u);
    random_abi(random, result, sp);
    result.policy = n % 4u;
    result.abi.gp[0] = (result.abi.gp[0] & ~0xffu) | (shape == 2 ? 10u : 7u);
    result.abi.gp[3] = blob;
    result.abi.gp[5] = shape == 9 ? sp + 8u : blob + 0x64u;
    auto &memory = result.memory;
    constexpr std::uint32_t half = 0x3f000000u;
    constexpr std::uint32_t one = 0x3f800000u;
    result.abi.xmm[0][0] = half;
    result.abi.xmm[1][0] = 0x3e800000u;
    result.abi.xmm[2][0] = 0x3e000000u;
    memory.put(sp + 0xcu, half);
    memory.put(sp + 0x20u, half);
    memory.put(sp + 0x24u, 0x3e800000u);
    memory.put(unit_address, one);
    memory.put(blob, shape == 2 ? 10u : 7u, 1);
    memory.put(blob + 1u, shape == 0 ? 0x80u : 0x81u, 1);
    for (unsigned offset : {0x34u, 0x38u, 0x3cu, 0x40u, 0x64u, 0x68u, 0x6cu, 0x70u})
        memory.put(blob + offset, half);
    if (shape == 3) {
        result.abi.xmm[0][0] = 0x7f800001u;
        result.abi.xmm[1][0] = 0xffc00002u;
        memory.put(blob + 0x38u, 0xffc00003u);
        memory.put(blob + 0x68u, 0xffc00004u);
    }
    if (shape == 4) {
        // The base result changes the kind byte read before the second layer.
        result.abi.xmm[0][0] = 0;
        memory.put(blob + 0x40u, 0x3f80000au);
    }
    if (shape == 5) memory.drop(blob);
    if (shape == 6) memory.drop(blob + 0x40u);
    if (shape == 7) memory.drop(blob + 0x6cu);
    if (shape == 8) memory.drop(unit_address);
    if (shape == 10) memory.drop(sp + 0xcu);
    if (shape == 11) memory.drop(sp + 0x24u);
    if (shape >= 12) {
        std::mt19937_64 wide(random());
        for (auto &feature : result.abi.xmm)
            feature[0] = special_float(wide);
        for (unsigned offset : {0x34u, 0x38u, 0x3cu, 0x40u, 0x64u, 0x68u, 0x6cu, 0x70u})
            memory.put(blob + offset, random() & 1u ? special_float(wide) : std::uint32_t(random()));
        memory.put(sp + 0x24u, special_float(wide));
        memory.put(sp + 0x20u, special_float(wide));
        memory.put(sp + 0xcu, special_float(wide));
        if (shape == 13) memory.put(unit_address, special_float(wide));
    }
    return result;
}

bool same_fault_abi(unsigned n, const Case &c) {
    auto left = c.memory.clone();
    const auto baseline = execute(left, c, false, native, reference);
    if (baseline.fault.empty()) return true;
    auto right = c.memory.clone();
    const auto saved_fallbacks = fallbacks;
    const auto candidate = execute(right, c, true, native, reference);
    fallbacks = saved_fallbacks;
    if (same_abi(candidate.abi, baseline.abi)) return true;
    std::fprintf(stderr, "case %u differs in cubic registers at fault %x\n", n, baseline.cursor);
    return false;
}
} // namespace

int main(int argc, char **argv) try {
    const unsigned cases = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode, verify_env);
    force_reference = mode == "reference";
    if (force_reference) setenv(reference_env, "1", 1);
    std::mt19937 random(0x13e3e8u + (census_mode ? 1u : 0u) + (mode == "verify" ? 2u : 0u));
    Totals totals;
    for (unsigned n = 0; n < cases; ++n) {
        const auto sample = fixture(random, n);
        native_route = baseline_route = Continuation::retained;
        if (!compare(n, sample, native, reference, totals) || !same_fault_abi(n, sample)) return 1;
        if (native_route != baseline_route) {
            std::fprintf(stderr, "case %u cubic handoff %x vs %x\n", n,
                std::uint32_t(native_route), std::uint32_t(baseline_route));
            return 1;
        }
    }
    if (!census_mode && !force_reference) check_refusals(fixture(random, 0), native);
    const auto coverage = authored::coverage().load();
    if (force_reference && (fallbacks != cases || coverage))
        throw std::runtime_error("Force-reference did not execute the original cubic cases");
    if (!force_reference && cases >= shape_count * 4u && coverage != (1ull << authored::branch_count) - 1u)
        throw std::runtime_error("Cubic branch coverage is incomplete");
    report(cases, totals, coverage, authored::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

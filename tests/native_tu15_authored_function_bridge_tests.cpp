#include "native_tu15_frame_misc_harness.hpp"
#include "core/native_tu15_authored_function_bridge.hpp"
#include <cmath>

using namespace frame_misc_harness;
namespace authored = halo2::native_authored_function_detail;
namespace {
constexpr std::uint32_t points = 0x10500000u;
constexpr std::uint32_t scratch = 0x10600000u;
constexpr std::array<std::uint32_t, 11> source_targets{
    0x13deabu, 0x13debcu, 0x13dee5u, 0x13e035u, 0x13e0edu, 0x13e146u,
    0x13e5e2u, 0x13e2a2u, 0x13e5e2u, 0x13df3fu, 0x13e2a2u
};
struct RouteEvidence {
    std::uint64_t completed{}, resumed{}, full_reference{}, retained_callbacks{};
};
RouteEvidence routes;
void record_native_completion() { ++routes.completed; }

void curve_owner(WorldState &state, std::uint32_t target, unsigned arguments) {
    auto hash = entry_hash(state, target);
    for (unsigned index = 0; index <= arguments; ++index)
        hash = mix(hash, r(state.memory, state.gp[4] + 4u * index, 4));
    log_call(hash);
    w(state.memory, state.observe, target, scratch, 4, std::uint32_t(hash));
    const auto result = float(hash % 7u) * 0.125f;
    state.xmm[0] = _mm_set_ss(result);
    state.gp[0] = std::uint32_t(hash);
    state.gp[1] = std::uint32_t(hash >> 32u);
    state.gp[2] = std::uint32_t(hash ^ (hash >> 32u));
    state.flags.load(std::uint32_t(hash) & 0x8d5u);
    state.gp[4] += 4u * (arguments + 1u);
}

// The frame harness rejects literal callbacks during native code. Once a
// residual case begins, allow its original callbacks and check the exact
// first instruction. Its subsequent effects still compare against the full
// translated body.
void begin_retained_case(WorldState &state, std::uint32_t expected) {
    if (expected == 0x13de70u) ++routes.full_reference;
    else ++routes.resumed;
    auto *cursor = state.instruction_cursor ? state.instruction_cursor->position : nullptr;
    auto &services = const_cast<SourcePhysicsContactRuntimeServices &>(state.services);
    services.instruction = [cursor, expected](std::uint32_t pc) mutable {
        if (expected && expected != pc)
            throw std::runtime_error("Authored function resumed at the wrong original case");
        expected = 0;
        ++routes.retained_callbacks;
        if (cursor) *cursor = pc;
    };
}
} // namespace

namespace original {
[[maybe_unused]] std::uint32_t selected(std::uint32_t address, SourcePhysicsContactImage) { return address; }
std::uint32_t canonical_branch(std::uint32_t target, SourcePhysicsContactImage) { return target; }
void run_17f8a0(WorldState &state) { curve_owner(state, 0x17f8a0u, 0); }
void run_17f790(WorldState &state) { curve_owner(state, 0x17f790u, 1); }

void dispatch(WorldState &state, std::uint32_t target, std::uint32_t site, bool tail) {
    if (tail) throw std::runtime_error("Unexpected authored function tail call");
    auto hash = mix(entry_hash(state, target), site);
    hash = mix(hash, r(state.memory, state.gp[4], 4));
    if (target == 0x13e9a0u) {
        const auto layer = r(state.memory, state.gp[4] + 4u, 4);
        const auto index = r(state.memory, state.gp[4] + 8u, 4);
        if (layer > 1 || index > 3) throw std::runtime_error("Unexpected authored control point");
        log_call(mix(mix(hash, layer), index));
        state.gp[0] = points + layer * 32u + index * 8u;
        state.gp[4] += 12u;
    } else if (target == 0x37cde8u) {
        std::array<std::byte, 10> bytes;
        for (unsigned lane = 0; lane < 2; ++lane) {
            std::memcpy(bytes.data(), &state.fp.st(lane), bytes.size());
            for (auto byte : bytes) hash = mix(hash, std::to_integer<unsigned>(byte));
        }
        log_call(hash);
        const auto exponent = static_cast<double>(state.fp.st(0));
        const auto base = static_cast<double>(state.fp.st(1));
        const auto value = std::pow(base, exponent);
        state.fp.pop();
        state.fp.st(0) = static_cast<long double>(value);
        state.gp[4] += 4u;
    } else {
        throw std::runtime_error("Unexpected authored function child");
    }
    w(state.memory, state.observe, site, scratch + 4u, 4, std::uint32_t(hash));
}

std::uint32_t source_estimate(WorldState &state, std::uint32_t value) {
    if (!state.services.reciprocal_sqrt)
        throw std::runtime_error("Required original contact RSQRTSS owner unavailable");
    return state.services.reciprocal_sqrt(value);
}

[[maybe_unused]] std::uint32_t native_single_spill(long double value, std::uint16_t &status) {
    float result;
    std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1"
        : "=m"(result), "=m"(actual) : "m"(value) : "st");
    status = (status & ~0x200u) | (actual & 0x200u);
    return std::bit_cast<std::uint32_t>(result);
}
#include "body.inc"
#include "reference.inc"
#include "candidate.inc"
} // namespace original

namespace {
constexpr std::uint32_t one = 0x3f800000u;
Case author(std::mt19937 &random, unsigned n) {
    Case result;
    auto &memory = result.memory;
    const unsigned shape = n % 44u;
    const auto stack = 0x20010000u + (shape == 26 ? 0x14u : shape == 27 ? 0x18u
        : shape == 28 ? 0xffcu : shape == 29 ? 0x1000u : 0x50u + 4u * std::uint32_t(random() % 128));
    const auto reference = shape == 22 ? 0x10000ffcu : shape == 40 ? stack - 0x18u : 0x10000100u;
    const auto blob = shape == 24 ? 0x10300fffu : shape == 41 ? stack - 0x10u : 0x10300100u;
    random_abi(random, result, stack);
    result.abi.gp[7] = reference;
    result.reciprocal_sqrt = [](std::uint32_t value) {
        return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(value)))));
    };

    std::uint32_t kind = shape < 11 ? shape : 0u;
    if (shape == 14 || shape == 24 || shape == 35) kind = 1;
    else if (shape == 15) kind = 11;
    else if (shape == 16) kind = 255;
    else if (shape == 29) kind = 8;
    else if (shape >= 32 && shape <= 34) kind = 9;
    else if (shape >= 36) kind = std::uint32_t(random() % 11);
    const auto flags = shape == 14 || shape == 25 ? 0u : shape == 24 || shape == 35
        ? 1u : std::uint32_t(random() & 1u);

    memory.put(reference, shape == 12 ? 0u : shape == 13 ? 0x80000000u : 192u);
    memory.put(reference + 4u, shape == 11 ? 0u : blob);
    memory.put(blob, kind, 1);
    memory.put(blob + 1u, flags, 1);
    memory.put(blob + 2u, std::uint32_t(random() % 8), 1);
    memory.put(blob + 3u, std::uint32_t(random() % 8), 1);
    constexpr std::array coeffs{0x3ec00000u, 0x3f000000u, 0x3f400000u, one};
    for (unsigned index = 0; index < 40; ++index)
        memory.put(blob + 0x14u + index * 4u, coeffs[random() % coeffs.size()]);
    for (unsigned index = 0; index < source_targets.size(); ++index)
        memory.put(0x13e5f4u + index * 4u, source_targets[index]);
    if (shape == 17) memory.put(0x13e5f4u, source_targets[4]);
    if (shape == 18) memory.put(0x13e5f4u, source_targets[6]);
    if (shape == 19) memory.put(0x13e5f4u, 0x11111111u);

    memory.put(0x46a0d0u, one);
    memory.put(0x46a0e8u, 0u);
    memory.put(0x46a0ecu, 0x38d1b717u);
    for (unsigned layer = 0; layer < 2; ++layer) {
        const auto at = points + 32u * layer;
        memory.put(at + 0x00u, 0u);
        memory.put(at + 0x04u, 0u);
        memory.put(at + 0x08u, 0x3f000000u);
        memory.put(at + 0x0cu, 0u);
        memory.put(at + 0x10u, 0u);
        memory.put(at + 0x14u, 0u);
        memory.put(at + 0x18u, one);
        memory.put(at + 0x1cu, one);
    }
    memory.put(scratch, 0u);
    for (auto at = stack - 0x80u; at < stack + 0x20u; at += 4u)
        memory.put(at, std::uint32_t(random()));
    std::mt19937_64 wide(random());
    const auto first = shape == 20 ? 0x40200000u : shape == 21 ? 0xbf800000u
        : shape == 30 ? 0x3f000000u : shape == 31 ? 0x80000000u
        : n % 3 == 0 ? special_float(wide) : std::uint32_t(random());
    memory.put(stack + 4u, first);
    memory.put(stack + 8u, n % 4 == 0 ? special_float(wide) : 0x3e800000u);
    if (shape == 23) memory.drop(reference);
    if (shape == 22) memory.drop(reference);
    if (shape == 24) memory.drop(blob + 1u);
    if (shape == 25) memory.drop(0x13e5f4u);
    if (shape == 26 || shape == 27) memory.drop(stack - 0x1cu);
    if (shape == 28) memory.drop(stack + 4u);
    if (shape == 29) memory.drop(stack);
    if (shape == 30) memory.drop(0x46a0d0u);
    if (shape == 33) memory.drop(blob);
    if (shape == 34) memory.drop(scratch);
    // The native prefix must do the real pushes and seed write before reads
    // that alias its own stack frame. A speculative classifier would see
    // the previous values.
    if (shape == 40) {
        result.abi.gp[3] = blob;
        result.abi.gp[5] = 192u;
    }
    return result;
}

bool native(WorldState &state) { return original::run_candidate_13de70(state); }

bool same_fault_abi(unsigned n, const Case &fixture) {
    auto baseline_memory = fixture.memory.clone();
    auto native_memory = fixture.memory.clone();
    const auto baseline = execute(baseline_memory, fixture, false, native, original::run_13de70);
    if (baseline.fault.empty()) return true;
    const auto totals = routes;
    const auto result = execute(native_memory, fixture, true, native, original::run_13de70);
    routes = totals;
    if (same_abi(result.abi, baseline.abi)) return true;
    std::fprintf(stderr, "case %u diverged in registers at fault %x\n", n, baseline.cursor);
    return false;
}
} // namespace

int main(int argc, char **argv) try {
    const unsigned cases = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode, "HALO2_VERIFY_AUTHORED_FUNCTION_BOUNDED");
    if (mode == "reference") setenv("HALO2_REFERENCE_AUTHORED_FUNCTION_BOUNDED", "1", 1);
    std::mt19937 random(0x13de70u + (census_mode ? 1u : 0u) + (mode == "verify" ? 2u : 0u));
    Totals totals;
    for (unsigned n = 0; n < cases; ++n) {
        const auto fixture = author(random, n);
        if (!compare(n, fixture, native, original::run_13de70, totals) ||
            !same_fault_abi(n, fixture))
            return 1;
    }
    if (!census_mode) check_refusals(author(random, 1), [](WorldState &state) {
        return authored::bridge(state, original::native_frame_misc_reference_13de70, original::canonical_branch)
            != halo2::native_authored_function::retained_entry;
    });
    if (mode == "reference") {
        if (routes.completed || routes.resumed || routes.full_reference != cases)
            throw std::runtime_error("Force-reference did not run every original entry");
    } else if (cases >= 44 && (!routes.completed || !routes.resumed || routes.full_reference ||
        routes.retained_callbacks <= routes.resumed)) {
        throw std::runtime_error("Residual translated work was missing from the callback accounting");
    }
    std::printf("{\"cases\":%u,\"compared\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,\"children\":%llu,"
        "\"coverage\":%llu,\"branches\":%u,\"completed_natively\":%llu,\"resumed_cases\":%llu,"
        "\"full_reference_calls\":%llu,\"retained_callbacks\":%llu,\"mode\":\"%s\"}\n",
        cases, static_cast<unsigned long long>(totals.compared), static_cast<unsigned long long>(totals.faults),
        static_cast<unsigned long long>(totals.stores), static_cast<unsigned long long>(totals.children),
        static_cast<unsigned long long>(authored::coverage().load()), unsigned(authored::branch_count),
        static_cast<unsigned long long>(routes.completed), static_cast<unsigned long long>(routes.resumed),
        static_cast<unsigned long long>(routes.full_reference), static_cast<unsigned long long>(routes.retained_callbacks),
        mode.c_str());
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

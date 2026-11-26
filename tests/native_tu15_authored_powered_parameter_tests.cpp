#include "native_tu15_frame_misc_harness.hpp"
#include "core/native_tu15_authored_powered_parameter.hpp"

using namespace frame_misc_harness;
namespace authored = halo2::native_authored_powered_parameter;

namespace {
constexpr std::uint32_t points = 0x10300100u;
constexpr std::uint32_t reference_argument = 0x10400000u;
constexpr std::uint32_t child_scratch = 0x10500000u;
constexpr std::uint32_t one_address = 0x46a0d0u;
constexpr std::uint32_t zero_address = 0x46a0e8u;
constexpr std::uint32_t epsilon_address = 0x46a0ecu;
constexpr const char *reference_env = "HALO2_REFERENCE_AUTHORED_POWERED_PARAMETER";
constexpr const char *verify_env = "HALO2_VERIFY_AUTHORED_POWERED_PARAMETER";
constexpr unsigned shapes = 20;
std::uint64_t fallback_calls{}, estimates{};
bool force_reference{};

void required_owner(SourcePhysicsContactMemory &memory, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsContactRuntimeAbi &abi, const SourcePhysicsContactRecordObserver &observe) {
    const auto sp = abi.gp[4];
    auto hash = mix(call.target, call.instruction);
    for (auto word : abi.gp) hash = mix(hash, word);
    for (const auto &reg : abi.xmm)
        for (auto lane : reg) hash = mix(hash, lane);
    hash = mix(mix(hash, abi.eflags), abi.x87_status);
    hash = mix(hash, r(memory, sp, 4));
    if (abi.floating_point) {
        hash = mix(hash, abi.floating_point->mxcsr);
        hash = mix(hash, abi.floating_point->x87_control_word);
    }
    for (unsigned index = 0; index < abi.x87_depth; ++index) {
        std::array<std::byte, 10> lane;
        std::memcpy(lane.data(), &abi.x87[index], lane.size());
        for (auto byte : lane) hash = mix(hash, std::to_integer<unsigned>(byte));
    }

    if (call.tail) throw std::runtime_error("Unexpected powered parameter tail route");
    if (call.target == authored::control_point_owner) {
        const auto layer = r(memory, sp + 4u, 4);
        const auto index = r(memory, sp + 8u, 4);
        if (layer > 1 || index > 3 || abi.gp[0] != reference_argument)
            throw std::runtime_error("Unexpected control point ABI");
        log_call(mix(mix(hash, layer), index));
        abi.gp[0] = points + 32u * layer + 8u * index;
        abi.gp[1] = std::uint32_t(hash);
        abi.gp[2] = std::uint32_t(hash >> 32u);
        if (policy / 2u == 13u && index == 2u) {
            abi.eflags ^= 0x41u;
            throw std::runtime_error("Control point service fault");
        }
        abi.gp[4] += 12u;
    } else if (call.target == authored::power_owner) {
        log_call(hash);
        if (abi.x87_depth < 2u) throw std::runtime_error("Unexpected CRT power ABI");
        abi.gp[0] = std::uint32_t(hash);
        abi.gp[2] = std::uint32_t(hash >> 32u);
        abi.eflags ^= 0x45u;
        if (policy / 2u == 14u) throw std::runtime_error("CRT power service fault");
        for (unsigned index = 1; index + 1u < abi.x87_depth; ++index) abi.x87[index] = abi.x87[index + 1u];
        --abi.x87_depth;
        const auto top = ((unsigned(abi.x87_status) >> 11u) + 1u) & 7u;
        abi.x87_status = std::uint16_t((abi.x87_status & ~0x383fu) | (top << 11u) | 0x200u | 0x20u);
        const auto shape = policy / 2u;
        abi.x87[0] = shape == 4u ? 1.125L : shape == 5u ? -0.125L : shape == 19u ? 0.5L : 0.3141592653589793L;
        if (shape == 15u) abi.x87[0] = std::numeric_limits<long double>::quiet_NaN();
        if (shape == 19u) abi.floating_point->x87_control_word ^= 0x400u;
        abi.gp[4] += 4u;
    } else throw std::runtime_error("Unexpected powered parameter owner");
    w(memory, observe, call.instruction, child_scratch, 4, std::uint32_t(hash));
}
} // namespace

namespace original {
[[maybe_unused]] std::uint32_t selected(std::uint32_t address, SourcePhysicsContactImage) { return address; }

void dispatch(WorldState &s, std::uint32_t target, std::uint32_t site, bool tail) { s.required(site, target, tail); }

std::uint32_t source_estimate(WorldState &s, std::uint32_t value) {
    if (!s.services.reciprocal_sqrt)
        throw std::runtime_error("Required original contact RSQRTSS owner unavailable");
    return s.services.reciprocal_sqrt(value);
}

[[maybe_unused]] std::uint32_t native_single_spill(long double value, std::uint16_t &status) {
    float result;
    std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1"
        : "=m"(result), "=m"(actual) : "m"(value) : "st");
    status = (status & ~0x200u) | (actual & 0x200u);
    return std::bit_cast<std::uint32_t>(result);
}
#include "powered.inc"
} // namespace original

namespace {
void reference(WorldState &s) {
    if (policy & 1u) original::reference_blend(s);
    else original::reference_base(s);
}

bool native(WorldState &s) {
    const auto layer = policy & 1u ? authored::Layer::blend : authored::Layer::base;
    auto replay = [layer](auto &state, auto child, auto estimate) {
        if (layer == authored::Layer::base) original::verify_base(state, child, estimate);
        else original::verify_blend(state, child, estimate);
    };
    auto dispatch = [](WorldState &state, std::uint32_t target, std::uint32_t site) {
        original::dispatch(state, target, site, false);
    };
    if (authored::bridge(s, layer, original::spill_mode, replay, dispatch)) return true;
    if (!force_reference) return false;

    auto *cursor = s.instruction_cursor ? s.instruction_cursor->position : nullptr;
    auto &services = const_cast<SourcePhysicsWorldRuntimeServices &>(s.services);
    const auto site = layer == authored::Layer::base ? authored::base_sites.entry : authored::blend_sites.entry;
    services.instruction = [cursor, expected = site](std::uint32_t pc) mutable {
        if (expected && pc != expected) throw std::runtime_error("Powered parameter resumed at the wrong original PC");
        expected = 0;
        if (cursor) *cursor = pc;
    };
    ++fallback_calls;
    reference(s);
    return true;
}

Case fixture(std::mt19937 &random, unsigned n) {
    Case result;
    const auto shape = n / 2u % shapes;
    const auto layer = n & 1u;
    const auto sp = 0x20200000u + (shape == 9u ? 0x1008u : shape == 10u ? 0xfe0u : 0x100u);
    random_abi(random, result, sp);
    result.policy = shape * 2u + layer;
    result.abi.gp[7] = reference_argument;
    result.abi.xmm[0][0] = 0x3f000000u;
    result.abi.x87_status |= 0x200u;
    if (shape == 16u) result.abi.x87_depth = 7u;
    if (shape == 17u) result.abi.x87_depth = 8u;

    auto &memory = result.memory;
    for (auto at = sp - 0x20u; at < sp + 0x28u; at += 4u) memory.put(at, std::uint32_t(random()));
    memory.put(sp + 0x20u, 0x3e800000u);
    memory.put(reference_argument, 256u);
    memory.put(child_scratch, 0u);
    memory.put(one_address, 0x3f800000u);
    memory.put(zero_address, 0u);
    memory.put(epsilon_address, 0x38d1b717u);
    const auto row = points + layer * 32u;
    for (unsigned point = 0; point < 4u; ++point) {
        memory.put(row + point * 8u, 0u);
        memory.put(row + point * 8u + 4u, 0u);
    }
    memory.put(row, shape == 1u ? 0u : 0x3f800000u);
    memory.put(row + 16u, shape == 0u ? 0u : shape == 2u ? 0x3f800000u : 0x40000000u);
    if (shape == 6u) memory.put(row + 16u, 0x7fc0002au);
    if (shape == 7u) {
        // Distinct NaN payloads in both axes catch a swapped scalar add.
        memory.put(row + 16u, 0x7f80002au);
        memory.put(row + 20u, 0xffc0005au);
    }
    if (shape == 8u) memory.drop(points);
    if (shape == 9u) memory.drop(sp - 12u);
    if (shape == 10u) memory.drop(sp + 0x20u);
    if (shape == 11u) memory.drop(epsilon_address);
    if (shape != 12u) result.reciprocal_sqrt = [](std::uint32_t input) {
        ++estimates;
        if (policy / 2u == 18u) throw std::runtime_error("RSQRTSS service fault");
        return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(input)))));
    };
    return result;
}

bool same_fault_abi(unsigned n, const Case &c) {
    auto left = c.memory.clone();
    const auto baseline = execute(left, c, false, native, reference, required_owner);
    if (baseline.fault.empty()) return true;
    auto right = c.memory.clone();
    const auto saved_fallbacks = fallback_calls;
    const auto result = execute(right, c, true, native, reference, required_owner);
    fallback_calls = saved_fallbacks;
    if (same_abi(result.abi, baseline.abi)) return true;
    std::fprintf(stderr, "case %u powered parameter registers differ at fault %x\n", n, baseline.cursor);
    return false;
}

struct FaultMemory : Memory {
    std::uint32_t address;
    FaultMemory(const Memory &value, std::uint32_t bad_address) : Memory(value.clone()), address(bad_address) {}
    void read(std::uint32_t at, std::span<std::byte> bytes) const override {
        if (at == address) throw std::runtime_error("Injected powered parameter read fault");
        Memory::read(at, bytes);
    }
};

void check_spill_convention(std::mt19937 &random) {
    // Immediately after each rounded spill, a fault exposes C1 before the
    // next successful FLD clears it. The two retained units differ here.
    for (unsigned layer = 0; layer < 2u; ++layer) {
        for (bool after_power : {false, true}) {
            auto sample = fixture(random, 38u + layer);
            sample.abi.floating_point->x87_control_word = 0xb7fu;
            sample.memory.put(points + 32u * layer, 0x3fb00000u);
            sample.reciprocal_sqrt = [](std::uint32_t) { return 0x3f123457u; };
            const auto fault_address = after_power ? zero_address : sample.abi.gp[4] + 0x20u;
            const auto &sites = layer == 0u ? authored::base_sites : authored::blend_sites;
            const auto pc = after_power ? sites.power.load_zero : sites.root.load_parameter;
            const bool lens = original::spill_mode == authored::Spill::lens;
            const auto expected_c1 = lens != after_power;
            FaultMemory left(sample.memory, fault_address), right(sample.memory, fault_address);
            const auto baseline = execute(left, sample, false, native, reference, required_owner);
            const auto candidate = execute(right, sample, true, native, reference, required_owner);
            if (baseline.cursor != pc || candidate.cursor != pc || baseline.fault.empty() ||
                baseline.fault != candidate.fault || bool(baseline.abi.x87_status & 0x200u) != expected_c1 ||
                !same_abi(baseline.abi, candidate.abi) || baseline.mxcsr != candidate.mxcsr || !left.same(right) ||
                baseline.stores != candidate.stores || baseline.calls != candidate.calls) {
                std::fprintf(stderr, "C1 at %x: source %u, native %u, expected %u\n", pc,
                    unsigned(bool(baseline.abi.x87_status & 0x200u)), unsigned(bool(candidate.abi.x87_status & 0x200u)),
                    unsigned(expected_c1));
                throw std::runtime_error("Powered parameter spill convention differs from the original");
            }
        }
    }
}
} // namespace

int main(int argc, char **argv) try {
    const unsigned cases = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode, verify_env);
    force_reference = mode == "reference";
    if (force_reference) setenv(reference_env, "1", 1);
    std::mt19937 random(0x13e2b0u + (census_mode ? 1u : 0u) + (mode == "verify" ? 2u : 0u));
    Totals totals;
    for (unsigned n = 0; n < cases; ++n) {
        const auto sample = fixture(random, n);
        const auto before = estimates;
        if (!compare(n, sample, native, reference, totals, false, required_owner)) return 1;
        if (estimates != before && estimates != before + 2u)
            throw std::runtime_error("A reciprocal estimate was duplicated or omitted during replay");
        if (!same_fault_abi(n, sample)) return 1;
    }
    if (!census_mode && !force_reference) {
        check_refusals(fixture(random, 0), native);
        check_spill_convention(random);
    }
    const auto coverage = authored::coverage().load();
    if (force_reference && (fallback_calls != cases || coverage))
        throw std::runtime_error("Force-reference did not execute each original powered parameter case");
    if (!force_reference && cases >= shapes * 2u && coverage != (1ull << (authored::outcomes * 2u)) - 1u)
        throw std::runtime_error("Powered parameter branch coverage is incomplete");
    report(cases, totals, coverage, authored::outcomes * 2u, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

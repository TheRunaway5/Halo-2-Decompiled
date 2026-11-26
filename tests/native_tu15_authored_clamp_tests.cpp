#include "native_tu15_frame_misc_harness.hpp"
#include "core/native_tu15_authored_function_bridge.hpp"

using namespace frame_misc_harness;
namespace authored = halo2::native_authored_function_detail;

namespace original {
[[maybe_unused]] std::uint32_t selected(std::uint32_t address, SourcePhysicsContactImage) { return address; }
#include "clamp.inc"
} // namespace original

namespace {
constexpr const char *reference_env = "HALO2_REFERENCE_AUTHORED_CLAMP";
constexpr const char *verify_env = "HALO2_VERIFY_AUTHORED_CLAMP";
constexpr std::uint32_t unit = 0x46a0d0u;
constexpr std::uint32_t entry = 0x13e0d2u;
constexpr unsigned shapes = 17;
std::uint64_t fallback_calls{};
bool force_reference{};

bool native(WorldState &s) {
    if (authored::bridge_clamp(s, original::native_frame_misc_reference_13de70_clamp)) return true;
    if (!force_reference) return false;
    auto *cursor = s.instruction_cursor ? s.instruction_cursor->position : nullptr;
    auto &services = const_cast<SourcePhysicsWorldRuntimeServices &>(s.services);
    services.instruction = [cursor, first = true](std::uint32_t pc) mutable {
        if (first && pc != entry) throw std::runtime_error("Shared clamp resumed at the wrong instruction");
        first = false;
        if (cursor) *cursor = pc;
    };
    ++fallback_calls;
    original::native_frame_misc_reference_13de70_clamp(s);
    return true;
}

Case fixture(std::mt19937 &random, unsigned n) {
    const auto shape = n % shapes;
    const auto sp = 0x20010000u + (shape == 6 ? 0xff8u : shape == 7 ? 0xffcu
        : shape == 8 ? 0x1000u : shape == 9 ? 0xfe4u : shape == 10 ? 0x1003u : 0x100u);
    Case result;
    random_abi(random, result, sp);
    auto &memory = result.memory;
    for (unsigned offset : {0u, 4u, 8u, 0x1cu})
        memory.put(sp + offset, std::uint32_t(random()));
    constexpr std::array<std::uint32_t, 6> values{0xbf800000u, 0x40000000u, 0x3f000000u,
        0x80000000u, 0x7f800001u, 0xffc00001u};
    const bool randomized = shape >= 14;
    std::mt19937_64 wide(random());
    auto value = randomized ? special_float(wide) : values[shape % values.size()];
    if (shape >= 6 && shape <= 10) value = shape == 8 ? 0x3f000000u : 0xbf800000u;
    result.abi.xmm[5] = {};
    if (shape == 12 || shape == 16) {
        // The old clamp copies all four floor lanes on its lower exit.
        for (auto &lane : result.abi.xmm[5]) lane = std::uint32_t(random());
        result.abi.xmm[5][0] = shape == 12 ? 0x3f000000u : special_float(wide);
        if (shape == 12) value = 0u;
    }
    memory.put(sp + 0xcu, value);
    memory.put(unit, shape == 13 ? 0x7f800005u : 0x3f800000u);
    if (shape == 6 || shape == 7) memory.drop(sp + 4u);
    if (shape == 8 || shape == 10) memory.drop(sp);
    if (shape == 9) memory.drop(sp + 0x1cu);
    if (shape == 11) memory.drop(unit);
    return result;
}

bool same_fault_abi(unsigned n, const Case &c) {
    const auto reference = original::native_frame_misc_reference_13de70_clamp;
    auto a = c.memory.clone();
    const auto baseline = execute(a, c, false, native, reference);
    if (baseline.fault.empty()) return true;
    auto b = c.memory.clone();
    const auto saved_fallbacks = fallback_calls;
    const auto candidate = execute(b, c, true, native, reference);
    fallback_calls = saved_fallbacks;
    if (same_abi(candidate.abi, baseline.abi)) return true;
    std::fprintf(stderr, "case %u differs in clamp registers at fault %x\n", n, baseline.cursor);
    return false;
}

struct FaultMemory : Memory {
    std::uint32_t address;
    mutable unsigned reads_to_deny;

    FaultMemory(const Memory &memory, std::uint32_t fault_address, unsigned occurrence)
        : Memory(memory.clone()), address(fault_address), reads_to_deny(occurrence) {}

    void read(std::uint32_t at, std::span<std::byte> out) const override {
        if (at == address && --reads_to_deny == 0u) throw std::runtime_error("Injected clamp read fault");
        Memory::read(at, out);
    }
};

void check_ordered_read_faults(std::mt19937 &random) {
    struct FaultPoint {
        unsigned shape;
        std::uint32_t stack_offset;
        unsigned occurrence;
        std::uint32_t pc;
    };
    constexpr std::array faults{
        FaultPoint{0, 0xcu, 1, 0x13e0d2u}, FaultPoint{0, 0u, 1, 0x13e0e1u},
        FaultPoint{0, 4u, 1, 0x13e0e2u}, FaultPoint{0, 8u, 1, 0x13e0e6u},
        FaultPoint{0, 0x1cu, 1, 0x13e0eau}, FaultPoint{1, 0u, 1, 0x13e5e8u},
        FaultPoint{1, 4u, 1, 0x13e5e9u}, FaultPoint{1, 8u, 1, 0x13e5eau},
        FaultPoint{1, 0x1cu, 1, 0x13e5eeu}, FaultPoint{2, 0xcu, 2, 0x13e5e2u},
        FaultPoint{2, 0x46a0d0u, 1, 0x13e5d5u},
    };
    const auto reference = original::native_frame_misc_reference_13de70_clamp;
    for (const auto [shape, offset, occurrence, pc] : faults) {
        const auto sample = fixture(random, shape);
        const auto address = offset == unit ? unit : sample.abi.gp[4] + offset;
        FaultMemory left(sample.memory, address, occurrence), right(sample.memory, address, occurrence);
        const auto original_result = execute(left, sample, false, native, reference);
        const auto native_result = execute(right, sample, true, native, reference);
        if (original_result.cursor != pc || native_result.cursor != pc || original_result.fault.empty() ||
            original_result.fault != native_result.fault || native_result.mxcsr != original_result.mxcsr ||
            !same_abi(original_result.abi, native_result.abi) || !left.same(right)) {
            std::fprintf(stderr, "Clamp fault differs at source PC %x\n", pc);
            throw std::runtime_error("Clamp read order differs from the original");
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
    std::mt19937 random(entry + (census_mode ? 1u : 0u) + (mode == "verify" ? 2u : 0u));
    Totals totals;
    for (unsigned n = 0; n < cases; ++n) {
        const auto sample = fixture(random, n);
        if (!compare(n, sample, native, original::native_frame_misc_reference_13de70_clamp, totals) ||
            !same_fault_abi(n, sample)) return 1;
    }
    if (!census_mode && !force_reference) {
        check_refusals(fixture(random, 0), native);
        check_ordered_read_faults(random);
    }
    const auto coverage = authored::clamp_coverage().load();
    const auto required = (1ull << authored::low_clamp) | (1ull << authored::high_clamp) | (1ull << authored::within_clamp);
    if (force_reference && (fallback_calls != cases || coverage))
        throw std::runtime_error("Force-reference did not execute the original shared clamp");
    if (!force_reference && cases >= shapes && coverage != required)
        throw std::runtime_error("Shared-clamp branch coverage is incomplete");
    report(cases, totals, coverage, 3, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

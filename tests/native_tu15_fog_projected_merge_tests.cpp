#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/native_tu15_fog_projected_merge.hpp"

using namespace frame_misc_harness;
namespace fog = halo2::native_fog_projected_merge_detail;

#include "fog_projected_merge_body.inc"

namespace {
void reference(WorldState &s) { log_call(std::uint32_t(native_fog_projected_merge_reference_131ac0(s))); }
bool native(WorldState &s) {
    const auto next = fog::bridge(s, native_fog_projected_merge_reference_131ac0);
    if (next == fog::Next::fallback)
        return false;
    log_call(std::uint32_t(next));
    return true;
}
struct FaultMemory : Memory {
    mutable unsigned accesses{};
    unsigned fail_at{};
    explicit FaultMemory(const Memory &source, unsigned fail = 0) : fail_at(fail) { pages = source.clone().pages; }
    void access() const {
        if (++accesses == fail_at)
            throw std::runtime_error("Injected projected fog access fault");
    }
    void read(std::uint32_t at, std::span<std::byte> out) const override {
        access();
        Memory::read(at, out);
    }
    void write(std::uint32_t at, std::span<const std::byte> in) override {
        access();
        Memory::write(at, in);
    }
};

constexpr std::uint32_t half = 0x3f000000u, one = 0x3f800000u, two = 0x40000000u;
Case fixture(std::mt19937 &rng, std::mt19937_64 &floats, unsigned row) {
    Case c;
    const auto stack = row % 13 == 7 ? 0xfffffff0u : 0x601ff4u + row % 13;
    const auto region = row % 17 == 12 ? stack + 1u : 0x710ff0u + row % 19;
    const auto patchy = row % 11 == 8 ? stack + 0x35u : 0x720ff8u + row % 9;
    random_abi(rng, c, stack);
    c.abi.gp[6] = region;
    const auto data = [&] {
        return row < 10 ? one : row & 1 ? std::uint32_t(rng()) : special_float(floats);
    };
    for (unsigned offset = 0; offset <= 0x148u; offset += 4)
        c.memory.put(stack + offset, data());
    c.memory.put(0x46a0d0u, row < 10 || row % 7 ? one : data());
    c.memory.put(stack + 0xb8u, row == 1 ? two : row < 10 ? half : data());
    c.memory.put(stack + 0x108u, row == 2 ? 0u : row < 10 || row % 3 ? one : data());
    c.memory.put(stack + 0x128u, row == 3 ? 0u : row < 10 || row % 5 ? one : data());
    c.memory.put(stack + 0x148u, row == 4 ? 0u : row < 10 || row % 7 ? one : data());
    c.memory.put(region + 0x60u, row == 5 ? 0x80000000u : row % 5 ? 1u : data());
    c.memory.put(region + 0x64u, patchy);
    c.memory.put(patchy + 0x4cu, row == 6 ? 0xffffffffu : row % 3 && row > 10 ? data() : 0u);
    c.memory.put(patchy + 0x1cu, row == 7 ? 0x80000000u : row < 10 || row % 11 ? one : data());
    if (row == 9) {
        c.memory.put(stack + 0xb8u, 0x7f80001au);
        c.memory.put(stack + 0xf0u, 0xffc01234u);
        c.memory.put(stack + 0x34u, 0x7fc04567u);
    }
    return c;
}

bool compare_exact(unsigned row, const Case &c, Totals &totals, unsigned site = 0) {
    FaultMemory expected_bank(c.memory, site), actual_bank(c.memory, site);
    const auto expected = execute(expected_bank, c, false, native, reference);
    const auto actual = execute(actual_bank, c, true, native, reference);
    if ((!actual.admitted && actual.fault.empty()) || (site && expected.fault.empty()) ||
        actual.fault != expected.fault || actual.mxcsr != expected.mxcsr ||
        (!census_mode && actual.cursor != expected.cursor) || actual.stores != expected.stores ||
        actual.calls != expected.calls || !same_abi(actual.abi, expected.abi) || !actual_bank.same(expected_bank) ||
        actual_bank.accesses != expected_bank.accesses) {
        std::fprintf(stderr, "Projected fog row %u, fault %u, cursors %x/%x, ABI %u, errors '%s'/'%s'\n",
            row, site, actual.cursor, expected.cursor, unsigned(same_abi(actual.abi, expected.abi)),
            actual.fault.c_str(), expected.fault.c_str());
        return false;
    }
    ++totals.compared;
    totals.faults += !expected.fault.empty();
    totals.stores += expected.stores.size();
    totals.children += expected.calls.size();
    return true;
}
} // namespace

int main(int argc, char **argv) try {
    const std::string mode = argc > 1 ? argv[1] : "literal";
    const auto count = argc > 2 ? unsigned(std::stoul(argv[2])) : 12000u;
    set_mode(mode, "HALO2_VERIFY_FOG_PROJECTED_MERGE");
    std::mt19937 rng{0x132188u};
    std::mt19937_64 floats{0x132490u};
    const auto dense = fixture(rng, floats, 0);
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_FOG_PROJECTED_MERGE", "1", 1);
        auto bank = dense.memory.clone();
        const auto result = execute(bank, dense, true, native, reference);
        if (result.admitted || !result.fault.empty() || !result.stores.empty() || !result.calls.empty() ||
            !same_abi(result.abi, dense.abi) || result.cursor)
            throw std::runtime_error("Forced reference touched the projected fog state");
        std::puts("{\"forced_reference_refusals\":1}");
        return 0;
    }
    if (!census_mode)
        check_refusals(dense, native);
    Totals totals;
    for (unsigned row = 0; row < count; ++row)
        if (!compare_exact(row, fixture(rng, floats, row), totals))
            return 1;
    unsigned injected_faults{};
    for (const auto row : {0u, 5u, 6u, 7u}) {
        const auto sample = fixture(rng, floats, row);
        FaultMemory memory(sample.memory);
        const auto original = execute(memory, sample, false, native, reference);
        const auto expected = row == 0 ? fog::Next::patchy_fog : fog::Next::bloom;
        if (!original.fault.empty() || original.stores.size() != 25 ||
            original.calls != std::vector<std::uint64_t>{std::uint32_t(expected)})
            throw std::runtime_error("Dense projected fog row missed its original continuation");
        for (unsigned site = 1; site <= memory.accesses; ++site) {
            if (!compare_exact(row, sample, totals, site))
                return 1;
            ++injected_faults;
        }
    }
    const auto coverage = fog::coverage().load();
    if (coverage != 4095)
        throw std::runtime_error("Projected fog missed an outcome");
    std::printf("{\"mode\":\"%s\",\"compared\":%llu,\"faults\":%llu,\"injected_memory_faults\":%u,"
        "\"ordered_stores\":%llu,\"continuations_checked\":%llu,\"coverage\":%llu}\n",
        mode.c_str(), static_cast<unsigned long long>(totals.compared), static_cast<unsigned long long>(totals.faults),
        injected_faults, static_cast<unsigned long long>(totals.stores), static_cast<unsigned long long>(totals.children),
        static_cast<unsigned long long>(coverage));
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

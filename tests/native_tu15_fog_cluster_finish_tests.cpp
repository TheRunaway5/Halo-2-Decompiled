#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/native_tu15_fog_cluster_finish.hpp"

using namespace frame_misc_harness;
namespace fog = halo2::native_fog_cluster_finish_detail;

#include "fog_cluster_finish_body.inc"

namespace {
fog::Entry entry = fog::Entry::bloom;
void reference(WorldState &s) { log_call(std::uint32_t(native_fog_cluster_finish_reference_131ac0(s, entry))); }
bool native(WorldState &s) {
    const auto next = fog::bridge(s, entry, native_fog_cluster_finish_reference_131ac0);
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
            throw std::runtime_error("Injected fog cluster memory fault");
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
constexpr std::uint32_t one = 0x3f800000u, two = 0x40000000u, half = 0x3f000000u;
constexpr std::array entries{fog::Entry::bloom, fog::Entry::recorded, fog::Entry::uncovered};
Case fixture(std::mt19937 &rng, std::mt19937_64 &floats, unsigned row) {
    Case c;
    const auto stack = row % 23 == 8 ? 0xfffffff5u : 0x601000u + row % 11;
    const auto profile = row % 7 == 4 ? stack - 0x88u : 0x700f21u + row % 13;
    const auto record = row % 11 == 5 ? stack + 0xe4u : 0x710ff8u + row % 3;
    const auto cluster = 0x720f01u;
    random_abi(rng, c, stack);
    c.abi.gp[3] = profile;
    c.abi.gp[5] = record;
    c.abi.gp[6] = cluster;
    const auto data = [&] {
        return row < 12 ? one : row & 1 ? std::uint32_t(rng()) : special_float(floats);
    };
    for (unsigned offset = 0x54; offset <= 0x68; offset += 4)
        c.memory.put(stack + offset, data());
    for (unsigned offset = 0xe0; offset <= 0xec; offset += 4)
        c.memory.put(profile + offset, data());
    for (unsigned offset : {0xfffffffcu, 0u, 4u, 8u})
        c.memory.put(record + offset, data());
    c.memory.put(cluster + 0x6fu, row < 12 ? 0x80u : std::uint32_t(rng()), 1);
    c.memory.put(stack + 0xb0u, std::uint32_t(rng()));
    c.memory.put(stack + 0xb4u, row < 12 ? (row % 4) + 1 : std::uint32_t(rng()));
    c.memory.put(stack + 0xe8u, data());
    c.memory.put(stack + 0x483cu, std::uint32_t(rng()));
    c.memory.put(stack + 0x13u, row < 12 ? row % 2 : std::uint32_t(rng()), 1);
    c.memory.put(0x556e10u, row < 12 || row % 5 ? one : data());
    c.abi.xmm[3][0] = row < 12 || row % 3 ? one : special_float(floats);
    c.abi.xmm[4][0] = row < 12 ? half : row % 5 ? half : special_float(floats);
    c.abi.xmm[7][0] = row < 12 || row % 7 ? 0u : special_float(floats);
    if (row == 0)
        c.abi.xmm[4][0] = two;
    if (row == 3)
        c.memory.put(profile + 0xe0u, 0u);
    if (row == 4)
        c.memory.put(record - 4u, two);
    if (row == 6)
        c.memory.put(profile + 0xecu, 0u);
    if (row == 9) {
        c.abi.xmm[4][0] = 0xffc01234u;
        c.memory.put(profile + 0xecu, 0x7f800001u);
        c.memory.put(stack + 0x60u, 0x7fc00187u);
    }
    if (row >= 12 && row % 11 == 0)
        c.memory.put(stack + 0xb4u, 1);
    return c;
}
} // namespace

int main(int argc, char **argv) try {
    const std::string mode = argc > 1 ? argv[1] : "literal";
    const auto count = argc > 2 ? unsigned(std::stoul(argv[2])) : 12000u;
    set_mode(mode, "HALO2_VERIFY_FOG_CLUSTER_FINISH");
    std::mt19937 rng{0x131fc7u};
    std::mt19937_64 floats{0x1320e0u};
    auto dense = fixture(rng, floats, 0);
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_FOG_CLUSTER_FINISH", "1", 1);
        const auto result = execute(dense.memory, dense, true, native, reference);
        if (result.admitted || !result.fault.empty() || !result.stores.empty() || !result.calls.empty() ||
            !same_abi(result.abi, dense.abi) || result.cursor)
            throw std::runtime_error("Forced reference touched the fog cluster state");
        std::puts("{\"forced_reference_refusals\":1}");
        return 0;
    }
    if (!census_mode)
        check_refusals(dense, native);
    Totals totals;
    for (unsigned row = 0; row < count; ++row) {
        entry = entries[row % entries.size()];
        if (!compare(row, fixture(rng, floats, row), native, reference, totals))
            return 1;
    }
    entry = fog::Entry::bloom;
    FaultMemory memory(dense.memory);
    const auto original = execute(memory, dense, false, native, reference);
    if (!original.fault.empty() || original.stores.size() != 11 ||
        original.calls != std::vector<std::uint64_t>{std::uint32_t(fog::Next::resolve)})
        throw std::runtime_error("Dense fog cluster row missed an access path");
    for (unsigned site = 1; site <= memory.accesses; ++site) {
        FaultMemory expected_bank(dense.memory, site), actual_bank(dense.memory, site);
        const auto expected = execute(expected_bank, dense, false, native, reference);
        const auto actual = execute(actual_bank, dense, true, native, reference);
        if (expected.fault.empty() || actual.fault != expected.fault ||
            actual.mxcsr != expected.mxcsr || (!census_mode && actual.cursor != expected.cursor) ||
            actual.stores != expected.stores || actual.calls != expected.calls ||
            !same_abi(actual.abi, expected.abi) || !actual_bank.same(expected_bank)) {
            std::fprintf(stderr, "Fault %u cursor %x/%x, ABI %u, errors '%s'/'%s'\n",
                site, actual.cursor, expected.cursor, unsigned(same_abi(actual.abi, expected.abi)),
                actual.fault.c_str(), expected.fault.c_str());
            return 1;
        }
        ++totals.compared;
        ++totals.faults;
        totals.stores += actual.stores.size();
    }
    const auto coverage = fog::coverage().load();
    if (coverage != 32767)
        throw std::runtime_error("Fog cluster completion missed an outcome");
    std::printf("{\"mode\":\"%s\",\"compared\":%llu,\"memory_faults\":%llu,"
        "\"ordered_stores\":%llu,\"continuations_checked\":%llu,\"coverage\":%llu}\n",
        mode.c_str(), static_cast<unsigned long long>(totals.compared), static_cast<unsigned long long>(totals.faults),
        static_cast<unsigned long long>(totals.stores), static_cast<unsigned long long>(totals.children),
        static_cast<unsigned long long>(coverage));
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

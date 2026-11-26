#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/native_tu15_fog_accumulation.hpp"

using namespace frame_misc_harness;
namespace fog = halo2::native_fog_accumulation_detail;

#include "fog_accumulation_body.inc"

namespace {
void reference(WorldState &s) { log_call(std::uint32_t(native_fog_accumulation_reference_131ac0(s))); }
bool native(WorldState &s) {
    const auto next = fog::bridge(s, native_fog_accumulation_reference_131ac0);
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
            throw std::runtime_error("Injected fog memory fault");
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
Case fixture(std::mt19937 &rng, std::mt19937_64 &fp_rng, unsigned row) {
    Case c;
    const auto stack = row % 13 == 8 ? 0xffffffcdu : 0x601000u + row % 11;
    const auto profile = row == 4 ? stack - 0x18u : row % 7 == 4 ? stack + 8 : 0x700fb0u + row % 13;
    random_abi(rng, c, stack);
    const auto data = [&] {
        return row < 8 ? one : row & 1 ? std::uint32_t(rng()) : special_float(fp_rng);
    };
    for (std::uint32_t at = 0; at <= 0x8cu; at += 4)
        c.memory.put(stack + at, data());
    for (std::uint32_t at = 0; at <= 0xd4u; at += 4)
        c.memory.put(profile + at, data());
    c.abi.gp[3] = profile;
    c.abi.xmm[3][0] = row < 8 ? one : row % 3 ? one : special_float(fp_rng);
    c.abi.xmm[4][0] = row < 8 ? half : row % 7 ? half : special_float(fp_rng);
    c.abi.xmm[7][0] = row < 8 || row % 19 ? 0u : special_float(fp_rng);
    if (row == 0)
        c.abi.xmm[4][0] = 0u;
    if (row == 1)
        c.abi.xmm[4][0] = two;
    if (row == 2)
        c.memory.put(profile + 0xa4u, 0u);
    if (row == 3)
        c.abi.gp[0] = c.memory.get(profile + 0xd4u);
    // The ordered attenuation store can alias the second candidate read.
    if (row == 4)
        c.abi.xmm[4][0] = one;
    if (row == 5) {
        c.abi.xmm[4][0] = 0x7f800001u;
        c.abi.xmm[7][0] = 0xffc01234u;
    }
    if (row == 6) {
        c.abi.xmm[3][0] = 0xffc01234u;
        c.memory.put(profile + 0x4u, 0x7f800010u);
        c.memory.put(stack + 0x34u, 0xffc00021u);
    }
    if (row > 7 && row % 2)
        c.memory.put(profile + 0xa4u, one);
    if (row == 7)
        c.abi.xmm[7][0] = one;
    return c;
}
} // namespace

int main(int argc, char **argv) try {
    const std::string mode = argc > 1 ? argv[1] : "literal";
    const auto count = argc > 2 ? unsigned(std::stoul(argv[2])) : 12000u;
    set_mode(mode, "HALO2_VERIFY_FOG_ACCUMULATION");
    std::mt19937 rng{0x131d38u};
    std::mt19937_64 floats{0x131ac0u};
    auto dense = fixture(rng, floats, 1);
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_FOG_ACCUMULATION", "1", 1);
        const auto result = execute(dense.memory, dense, true, native, reference);
        if (result.admitted || !result.fault.empty() || !result.stores.empty() || !result.calls.empty() ||
            !same_abi(result.abi, dense.abi) || result.cursor)
            throw std::runtime_error("Forced reference touched the guest state");
        std::puts("{\"forced_reference_refusals\":1}");
        return 0;
    }
    if (!census_mode)
        check_refusals(dense, native);
    Totals totals;
    for (unsigned row = 0; row < count; ++row) {
        const auto c = fixture(rng, floats, row);
        if (!compare(row, c, native, reference, totals))
            return 1;
    }
    FaultMemory memory(dense.memory);
    const auto original = execute(memory, dense, false, native, reference);
    if (!original.fault.empty() || original.stores.size() != 24 ||
        original.calls != std::vector<std::uint64_t>{std::uint32_t(fog::Next::patchy_fog)})
        throw std::runtime_error("Dense fog accumulation missed an access path");
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
    if (coverage != 127)
        throw std::runtime_error("Fog accumulation did not hit all seven outcomes");
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

#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/native_tu15_patchy_fog.hpp"

using namespace frame_misc_harness;
namespace fog = halo2::native_patchy_fog_detail;

#include "patchy_fog_body.inc"

namespace {
bool native(WorldState &s) { return fog::bridge(s, native_patchy_fog_reference_1342b0); }
struct FaultMemory : Memory {
    mutable unsigned accesses{};
    unsigned fail_at{};
    explicit FaultMemory(const Memory &source, unsigned fail = 0) : fail_at(fail) { pages = source.clone().pages; }
    void access() const {
        if (++accesses == fail_at)
            throw std::runtime_error("Injected patchy fog memory fault");
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

constexpr std::uint32_t one = 0x3f800000u;
Case fixture(std::mt19937 &rng, std::mt19937_64 &floats, unsigned row) {
    Case c;
    const auto stack = row % 11 == 3 ? 0xfffffff3u : 0x601ff4u + row % 7;
    const auto output = row % 7 == 1 ? stack - 1u : 0x700ff1u + row % 13;
    const auto first = row % 3 == 1 ? output + 3u : 0x710ffdu + row % 5;
    const auto second = row % 5 == 2 ? output + 8u : 0x720ff4u + row % 7;
    random_abi(rng, c, stack);
    c.abi.gp[0] = output;
    c.abi.gp[1] = second;
    c.abi.gp[2] = first;
    const auto data = [&] {
        return row < 8 ? one : row & 1 ? std::uint32_t(rng()) : special_float(floats);
    };
    for (unsigned offset = 0; offset < 48; offset += 4)
        c.memory.put(output + offset, data());
    for (unsigned offset = 0; offset < 12; offset += 4) {
        c.memory.put(first + offset, data());
        c.memory.put(second + offset, data());
    }
    for (unsigned offset = 0; offset <= 16; offset += 4)
        c.memory.put(stack + offset, data());
    c.memory.put(0x46a0d0u, row < 8 || row % 5 ? one : data());
    constexpr std::array samples{0x3f000000u, 0x40000000u, 0xbf800000u, 0x80000000u, 0x7f800000u, 0xff800000u,
                                 0x7f801234u, 0xffc01234u, 1u, 0x80000001u};
    c.abi.xmm[0][0] = row < samples.size() ? samples[row] : samples[rng() % samples.size()];
    if (row == 6) {
        c.memory.put(first, 0xffc07654u);
        c.memory.put(first + 4u, 0x7f801234u);
        c.memory.put(output, 0x7fc00002u);
    }
    return c;
}
} // namespace

int main(int argc, char **argv) try {
    const std::string mode = argc > 1 ? argv[1] : "literal";
    const auto count = argc > 2 ? unsigned(std::stoul(argv[2])) : 12000u;
    set_mode(mode, "HALO2_VERIFY_PATCHY_FOG");
    std::mt19937 rng{0x1342b0u};
    std::mt19937_64 floats{0x1343a6u};
    auto dense = fixture(rng, floats, 6);
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_PATCHY_FOG", "1", 1);
        const auto result = execute(dense.memory, dense, true, native, native_patchy_fog_reference_1342b0);
        if (result.admitted || !result.fault.empty() || !result.stores.empty() ||
            !same_abi(result.abi, dense.abi) || result.cursor)
            throw std::runtime_error("Forced reference touched the patchy fog state");
        std::puts("{\"forced_reference_refusals\":1}");
        return 0;
    }
    if (!census_mode)
        check_refusals(dense, native);
    Totals totals;
    for (unsigned row = 0; row < count; ++row) {
        const auto c = fixture(rng, floats, row);
        if (!compare(row, c, native, native_patchy_fog_reference_1342b0, totals))
            return 1;
    }
    FaultMemory memory(dense.memory);
    const auto original = execute(memory, dense, false, native, native_patchy_fog_reference_1342b0);
    if (!original.fault.empty() || original.stores.size() != 12)
        throw std::runtime_error("Dense patchy fog row missed an access path");
    for (unsigned site = 1; site <= memory.accesses; ++site) {
        FaultMemory expected_bank(dense.memory, site), actual_bank(dense.memory, site);
        const auto expected = execute(expected_bank, dense, false, native, native_patchy_fog_reference_1342b0);
        const auto actual = execute(actual_bank, dense, true, native, native_patchy_fog_reference_1342b0);
        if (expected.fault.empty() || actual.fault != expected.fault ||
            actual.mxcsr != expected.mxcsr || (!census_mode && actual.cursor != expected.cursor) ||
            actual.stores != expected.stores || !same_abi(actual.abi, expected.abi) || !actual_bank.same(expected_bank)) {
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
    if (coverage != 7)
        throw std::runtime_error("Patchy fog missed a clamp outcome");
    report(count, totals, coverage, 3, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

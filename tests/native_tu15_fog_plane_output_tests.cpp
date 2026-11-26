#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/native_tu15_fog_plane_output.hpp"

using namespace frame_misc_harness;
namespace fog = halo2::native_fog_plane_output_detail;

#include "fog_plane_output_body.inc"

namespace {
fog::Entry entry = fog::Entry::no_planes;
void reference(WorldState &s) { native_fog_plane_output_reference_133b40(s, entry); }
bool native(WorldState &s) { return fog::bridge(s, entry, native_fog_plane_output_reference_133b40); }
struct FaultMemory : Memory {
    mutable unsigned accesses{};
    unsigned fail_at{};
    explicit FaultMemory(const Memory &source, unsigned fail = 0) : fail_at(fail) { pages = source.clone().pages; }
    void access() const {
        if (++accesses == fail_at)
            throw std::runtime_error("Injected fog output access fault");
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

constexpr std::uint32_t half = 0x3f000000u, one = 0x3f800000u, two = 0x40000000u, negative = 0xbf800000u;
Case fixture(std::mt19937 &rng, std::mt19937_64 &floats, unsigned row) {
    Case c;
    const auto stack = row % 13 == 7 ? 0xffffffedu : 0x601ff5u + row % 13;
    const auto profile = 0x700fa1u + row % 15;
    const auto first_output = row % 17 == 10 ? profile + 0x40u : 0x710ff1u + row % 7;
    const auto second_output = row % 7 == 4 ? first_output + 4u : 0x720fedu + row % 13;
    const auto third_output = row % 19 == 14 ? stack + 3u : 0x730ff5u + row % 7;
    const auto source_a = row % 29 == 13 ? first_output + 3u : 0x740ff3u + row % 9;
    const auto source_b = row % 11 == 8 ? second_output : 0x750ffau + row % 11;
    const auto source_c = row % 7 == 6 ? third_output - 1u : 0x760ffdu + row % 7;
    random_abi(rng, c, stack);
    c.abi.gp[2] = profile;
    c.abi.gp[3] = second_output;
    c.abi.gp[5] = third_output;
    c.abi.gp[7] = first_output;
    const auto data = [&] {
        return row < 10 ? one : row & 1 ? std::uint32_t(rng()) : special_float(floats);
    };
    for (unsigned offset = 0; offset < 32; offset += 4) {
        c.memory.put(first_output + offset, data());
        c.memory.put(second_output + offset, data());
        c.memory.put(third_output + offset, data());
        c.memory.put(source_a + offset, data());
        c.memory.put(source_b + offset, data());
        c.memory.put(source_c + offset, data());
    }
    c.memory.put(profile + 0x48u, row % 5 == 3 ? 0u : 1u);
    c.memory.put(profile + 0x4cu, source_a);
    c.memory.put(profile + 0x50u, row % 7 == 5 ? 0xffffffffu : 1u);
    c.memory.put(profile + 0x54u, source_b);
    c.memory.put(profile + 0x58u, row % 7 == 2 ? 0u : 1u);
    c.memory.put(profile + 0x5cu, source_c);
    c.memory.put(stack, std::uint32_t(rng()));
    c.memory.put(stack + 0x34u, std::uint32_t(rng()));
    c.memory.put(stack + 0x38u, data());
    c.abi.xmm[2][0] = row < 10 ? 0u : row % 7 ? 0u : data();
    c.abi.xmm[6][0] = row < 10 || row % 9 ? one : data();
    constexpr std::array weights{half, two, negative, 0u, 0xffc04567u, 0x7f801234u};
    c.memory.put(first_output + 0x1cu, weights[row % weights.size()]);
    c.memory.put(second_output + 0x1cu, row < 10 ? one : data());
    c.memory.put(third_output + 0x1cu, weights[row % weights.size()]);
    if (row == 6) {
        c.memory.put(first_output + 0x1cu, 0xffc00123u);
        c.memory.put(second_output + 0x1cu, 0x7f801234u);
        c.memory.put(source_a, 0x7f803567u);
        c.memory.put(first_output, 0xffc089abu);
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
        !same_abi(actual.abi, expected.abi) || !actual_bank.same(expected_bank) ||
        actual_bank.accesses != expected_bank.accesses) {
        std::fprintf(stderr, "Fog output row %u, fault %u, cursors %x/%x, ABI %u, errors '%s'/'%s'\n",
            row, site, actual.cursor, expected.cursor, unsigned(same_abi(actual.abi, expected.abi)),
            actual.fault.c_str(), expected.fault.c_str());
        return false;
    }
    ++totals.compared;
    totals.faults += !expected.fault.empty();
    totals.stores += expected.stores.size();
    return true;
}
} // namespace

int main(int argc, char **argv) try {
    const std::string mode = argc > 1 ? argv[1] : "literal";
    const auto count = argc > 2 ? unsigned(std::stoul(argv[2])) : 12000u;
    set_mode(mode, "HALO2_VERIFY_FOG_PLANE_OUTPUT");
    std::mt19937 rng{0x134042u};
    std::mt19937_64 floats{0x1342a1u};
    auto dense = fixture(rng, floats, 0);
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_FOG_PLANE_OUTPUT", "1", 1);
        const auto result = execute(dense.memory, dense, true, native, reference);
        if (result.admitted || !result.fault.empty() || !result.stores.empty() ||
            !same_abi(result.abi, dense.abi) || result.cursor)
            throw std::runtime_error("Forced reference touched the fog output state");
        std::puts("{\"forced_reference_refusals\":1}");
        return 0;
    }
    if (!census_mode)
        check_refusals(dense, native);
    Totals totals;
    for (unsigned row = 0; row < count; ++row) {
        entry = row & 1 ? fog::Entry::classified : fog::Entry::no_planes;
        if (!compare_exact(row, fixture(rng, floats, row), totals))
            return 1;
    }
    for (const auto row : {0u, 1u, 2u, 3u, 5u}) {
        entry = row & 1 ? fog::Entry::classified : fog::Entry::no_planes;
        dense = fixture(rng, floats, row);
        FaultMemory memory(dense.memory);
        const auto original = execute(memory, dense, false, native, reference);
        if (!original.fault.empty() || (row < 2 && original.stores.size() != 25) ||
            original.cursor != (row == 2 ? 0x1342a1u : 0x13428cu))
            throw std::runtime_error("Fog output fixture missed its normal return");
        for (unsigned site = 1; site <= memory.accesses; ++site) {
            if (!compare_exact(row, dense, totals, site))
                return 1;
        }
    }
    const auto coverage = fog::coverage().load();
    if (coverage != 16383)
        throw std::runtime_error("Fog output missed an outcome");
    report(count, totals, coverage, 14, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}

#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/native_tu15_fog_plane_projection.hpp"

using namespace frame_misc_harness;
namespace fog = halo2::native_fog_plane_projection_detail;

#include "fog_plane_projection_body.inc"

namespace {
fog::Entry entry = fog::Entry::project;
void reference(WorldState &s) { log_call(std::uint32_t(native_fog_plane_projection_reference_133b40(s, entry))); }
bool native(WorldState &s) {
    const auto next = fog::bridge(s, entry, native_fog_plane_projection_reference_133b40);
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
            throw std::runtime_error("Injected fog projection access fault");
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

constexpr std::uint32_t half = 0x3f000000u, one = 0x3f800000u, two = 0x40000000u, four = 0x40800000u, negative = 0xbf800000u;
Case fixture(std::mt19937 &rng, std::mt19937_64 &floats, unsigned row) {
    Case c;
    const auto stack = row % 11 == 7 ? 0xfffffffbu : 0x601ff7u + row % 13;
    const auto profile = row % 13 == 9 ? stack + 5u : 0x700fcdu + row % 15;
    const auto first_output = row % 17 == 10 ? profile + 1u : 0x710ff1u + row % 7;
    const auto second_output = row % 7 == 4 ? first_output + 4u : 0x720fedu + row % 13;
    const auto third_output = row % 19 == 14 ? stack + 3u : 0x730ff5u + row % 7;
    random_abi(rng, c, stack);
    c.abi.gp[3] = second_output;
    c.abi.gp[5] = third_output;
    c.abi.gp[6] = row == 3 || row % 17 == 5 ? 0u : profile;
    c.abi.gp[7] = first_output;
    const auto data = [&] {
        return row < 12 ? one : row & 1 ? std::uint32_t(rng()) : special_float(floats);
    };
    for (unsigned offset = 0; offset < 32; offset += 4) {
        c.memory.put(first_output + offset, data());
        c.memory.put(second_output + offset, data());
        c.memory.put(third_output + offset, data());
    }
    for (unsigned offset = 0; offset <= 0x28u; offset += 4)
        c.memory.put(profile + offset, data());
    constexpr std::array scales{one, negative, four};
    for (unsigned index = 0; index < 3; ++index)
        c.memory.put(profile + 0x20u + 4u * index, row % 3 ? scales[(row + index) % 3] : data());
    c.memory.put(profile + 0x18u, row % 23 == 11 ? data() : 0u);
    c.memory.put(profile + 0x1cu, row == 2 ? 0u : row % 13 == 8 ? data() : one);
    c.memory.put(stack + 8u, row < 12 ? row % 3 : std::uint32_t(rng()));
    c.memory.put(stack + 0xcu, std::uint32_t(rng()));
    c.memory.put(stack + 0x10u, std::uint32_t(rng()));
    c.memory.put(0x46a0d0u, row % 17 == 9 ? data() : one);
    c.abi.xmm[0][0] = row < 8 ? (row == 5 ? two : row == 6 ? negative : half) : row % 23 == 9 ? half : data();
    if (row == 7) {
        c.abi.xmm[0][0] = 0xffc01234u;
        c.memory.put(profile, 0x7f800123u);
        c.memory.put(profile + 4u, 0x7fc04567u);
        c.memory.put(first_output, 0x7f802345u);
    }
    return c;
}
} // namespace

int main(int argc, char **argv) try {
    const std::string mode = argc > 1 ? argv[1] : "literal";
    const auto count = argc > 2 ? unsigned(std::stoul(argv[2])) : 12000u;
    set_mode(mode, "HALO2_VERIFY_FOG_PLANE_PROJECTION");
    std::mt19937 rng{0x133d8fu};
    std::mt19937_64 floats{0x13401au};
    auto dense = fixture(rng, floats, 0);
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_FOG_PLANE_PROJECTION", "1", 1);
        const auto result = execute(dense.memory, dense, true, native, reference);
        if (result.admitted || !result.fault.empty() || !result.stores.empty() || !result.calls.empty() ||
            !same_abi(result.abi, dense.abi) || result.cursor)
            throw std::runtime_error("Forced reference touched the fog projection state");
        std::puts("{\"forced_reference_refusals\":1}");
        return 0;
    }
    if (!census_mode)
        check_refusals(dense, native);
    Totals totals;
    for (unsigned row = 0; row < count; ++row) {
        entry = row % 13 == 8 ? fog::Entry::advance : fog::Entry::project;
        if (!compare(row, fixture(rng, floats, row), native, reference, totals))
            return 1;
    }
    entry = fog::Entry::project;
    FaultMemory memory(dense.memory);
    const auto original = execute(memory, dense, false, native, reference);
    if (!original.fault.empty() || original.stores.size() != 25 || original.calls.empty())
        throw std::runtime_error("Dense fog projection row missed an access path");
    for (unsigned site = 1; site <= memory.accesses; ++site) {
        FaultMemory expected_bank(dense.memory, site), actual_bank(dense.memory, site);
        const auto expected = execute(expected_bank, dense, false, native, reference);
        const auto actual = execute(actual_bank, dense, true, native, reference);
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
    if (coverage != 131071)
        throw std::runtime_error("Fog projection missed an outcome");
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
